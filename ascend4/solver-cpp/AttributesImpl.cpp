extern "C" {
#include "utilities/ascConfig.h"
#include "utilities/mem.h"
}

#include "solver-cpp/Attributes.h"
#include "solver-cpp/AttributesImpl.h"

//===============================================================
/** We keep all the attributes in a union, segregated by type in
    AttributesImpl->dl. We do this so we can mass allocate.
 */
struct AttributeData {
  const char *name;
  AttributeData *next;
  union { 
    void *oPtr;
    const char *cPtr;
    real64 r8;
    real32 r4;
    int64 i8;
    int32 i4;
    int32 b4;
  } u;
};

//===============================================================
struct AttributeData *AttributesImpl::findName(struct AttributeData *ad, const char *name) {
  if (name == 0) { return 0; }
  while (ad != 0) {
    assert(ad != 0);
    if (ad->name == name || strcmp(name,ad->name) == 0) {
      return ad;
    }
    ad = ad->next;
  }
  return 0;
}


//===============================================================
//===============================================================

//===============================================================
// boolean attributes list
#define B4LIST  dl[0]
#define B4INDEX 0

// double attributes list
#define R8LIST  dl[1]
#define R8INDEX 1

// int attributes list
#define I4LIST  dl[2]
#define I4INDEX 2

// string attributes list
#define SLIST   dl[3]
#define SINDEX  3

// float attributes list
#define R4LIST  dl[4]
#define R4INDEX 4

// long long attributes list
#define I8LIST  dl[5]
#define I8INDEX 5

// object attributes list
#define OLIST   dl[6]
#define OINDEX  6

static const char *dlName[7] = {
  "BOOL", "DOUBLE8", "INT4", "STRING", "FLOAT4", "LONG8", "VOIDPTR"
};


//===============================================================
/** 
  We recycle Attributes to avoid bombing malloc/free with small requests.
  The pool is filled whenever AttributeData are needed and emptied
  when the last AttributesImpl is deleted, so we don't have to provide
  init and destroy functions.
 */
static mem_store_t g_AttributeData_pool = 0;

static int g_AttributesImplCount = 0;

#define LENMAGIC 10
#define WIDTHMAGIC 2048

//===============================================================

AttributesImpl::AttributesImpl() {
  if (g_AttributeData_pool == 0) {
    g_AttributeData_pool = 
      mem_create_store(LENMAGIC, WIDTHMAGIC/sizeof(struct AttributeData) - 1,
                       sizeof(struct AttributeData),LENMAGIC,WIDTHMAGIC);
  }
  dl[0] = dl[1] = dl[2] = dl[3] = dl[4] = dl[5] = dl[6] = 0;
  g_AttributesImplCount++;
}

//===============================================================

AttributesImpl::~AttributesImpl() {
  int i;
  for (i = 0; i < AttributesImpl::kindCount; i++) {
    AttributeData *old;
    while (dl[i] != 0) {
      old = dl[i];
      dl[i] = old->next;
      old->next = 0;
      old->name = 0;
      old->u.r8 = 0;
      mem_free_element(g_AttributeData_pool,(void *)old);
    }
  }

  if (list != 0) { free(list); list = 0; }

  g_AttributesImplCount--;
  if (g_AttributesImplCount <= 0 && g_AttributeData_pool != 0) {
    assert(g_AttributesImplCount == 0);
    mem_destroy_store(g_AttributeData_pool);
    g_AttributeData_pool = 0;
    g_AttributesImplCount = 0;
  }
}

//===============================================================
#define SETVALUE(NAME,VALUE,LIST,UNAME) \
  struct AttributeData *ad; \
  if (NAME == 0) { return; } \
  if ((ad = findName(LIST,NAME)) == 0) { \
    ad = (struct AttributeData *)mem_get_element(g_AttributeData_pool); \
    ad->name = (NAME); \
    ad->u.##UNAME = (VALUE); \
    ad->next = LIST; \
    LIST = ad; \
  } else { \
    ad->u.##UNAME = (VALUE); \
  } \
  return

//===============================================================
void AttributesImpl::set(const char *name, void * ovalue) {
  SETVALUE(name,ovalue,OLIST,oPtr);
}
  

//===============================================================
void AttributesImpl::set(const char *name, int32 ivalue) {
  SETVALUE(name,ivalue,I4LIST,i4);
}

//===============================================================
void AttributesImpl::set(const char *name, int64 lvalue) {
  SETVALUE(name,lvalue,I8LIST,i8);
}

//===============================================================
void AttributesImpl::set(const char *name, real64 rvalue) {
  SETVALUE(name,rvalue,R8LIST,r8);
}

//===============================================================
void AttributesImpl::set(const char *name, real32 rvalue) {
  SETVALUE(name,rvalue,R4LIST,r4);
}

//===============================================================
void AttributesImpl::set(const char *name, const char *svalue) {
  SETVALUE(name,svalue,SLIST,cPtr);
}

//===============================================================
void AttributesImpl::set(int boolValue, const char *name) {
  SETVALUE(name,(boolValue?1:0),B4LIST,b4);
}


//===============================================================
#define GETVALUE(NAME,VALUE,LIST,UNAME) \
  struct AttributeData *ad; \
  if (name == 0) { return -1; } \
  ad = findName(LIST,NAME); \
  if (ad != 0) { \
    VALUE = ad->u.##UNAME; \
    return 0; \
  } \
  VALUE = 0; \
  return -1

//===============================================================
int AttributesImpl::get(const char *name, void *&ovalue) {
  GETVALUE(name,ovalue,OLIST,oPtr);
}

//===============================================================
int AttributesImpl::get(const char *name, int32 & ivalue) {
  GETVALUE(name,ivalue,I4LIST,i4);
}

//===============================================================
int AttributesImpl::get(const char *name, int64 & lvalue) {
  GETVALUE(name,lvalue,I8LIST,i8);
}

//===============================================================
int AttributesImpl::get(const char *name, real64 & rvalue) {
  GETVALUE(name,rvalue,R8LIST,r8);
}

//===============================================================
int AttributesImpl::get(const char *name, real32 & fvalue) {
  GETVALUE(name,fvalue,R4LIST,r4);
}

//===============================================================
int AttributesImpl::get(const char *name, const char *& svalue) {
  GETVALUE(name,svalue,SLIST,cPtr);
}

//===============================================================
int AttributesImpl::get(int & boolValue, const char *name) {
  GETVALUE(name,boolValue,B4LIST,b4);
}

//===============================================================
void **AttributesImpl::getList(int32 & nAttr) {
  int nelts, offset;
  AttributeData *elt;

  if (list != 0) { 
    free(list); 
    list = 0;
  }

  nelts = 0;
  for (int i = 0; i < AttributesImpl::kindCount; i++) {
    elt = dl[i];
    while (elt != 0) {
      nelts++;
      elt = elt->next;
    }
  }

  list = (void **)malloc(sizeof(void *)*(3*nelts+1));
  if (list == 0) { return 0; }
  list[3*nelts] = 0; // NULL terminated
  offset = 0;

  for (int i = 0; i < AttributesImpl::kindCount; i++) {
    elt = dl[i];
    while (elt != 0) {
      list[offset] = (void *)dlName[i];
      list[offset+1] = (void *) (elt->name);
      switch (i) {
      case OINDEX:
        list[offset+2] = elt->u.oPtr;
        break;
      case SINDEX:
        list[offset+2] = (void *)elt->u.cPtr;
        break;
      case R8INDEX:
        list[offset+2] = (void *)(&(elt->u.r8));
        break;
      case R4INDEX:
        list[offset+2] = (void *)(&(elt->u.r4));
        break;
      case I8INDEX:
        list[offset+2] = (void *)(&(elt->u.i8));
        break;
      case I4INDEX:
        list[offset+2] = (void *)(&(elt->u.i4));
        break;
      case B4INDEX:
        list[offset+2] = (void *)(&(elt->u.b4));
        break;
      default:
        assert(0);
        list[offset+2] = 0;
      }
      offset += 3;
      elt = elt->next;
    }
  }
  nAttr = nelts;
  return list; 
}

//===============================================================
void AttributesImpl::unset(const char *name) {
  AttributeData *die, **handle;
  if (name == 0) { return; }
  int dumped = 0;
  for (int i = 0; i < AttributesImpl::kindCount; i++) {
    handle = &(dl[i]);
    die = dl[i];
    while (die != 0) {
      if (die->name == name || strcmp(name,die->name) == 0) {
        *handle = die->next; // stick next into previous link.
        die->u.r8 = 0;
        die->next = 0;
        die->name = 0;
        mem_free_element(g_AttributeData_pool,(void *)die); // dump element.
        dumped++;
      } else {
        handle = &(die->next); // advance link to this element's.
        die = die->next; // advance to next element.
      }
    }
  }
  if (list != 0 && dumped) {
    free(list);
    list = 0;
  }
}


//===============================================================
#ifdef Main_AttributesImpl
int main() {
  AttributesImpl a;
  a.set("fred8",8.0); // r8
  a.set("fred4",(float)4.0); // r4
  a.set("barney4",3); // i4
  a.set("barney8size",(int)sizeof(long long)); //i8?
  a.set("barney8",3000000000000LL); //i8
  a.set("SELF",&a); // obj
  a.set("string","freds"); // str
  a.set(2,"boolean 2"); // bool
  a.set("fred4",(float)4.1); // r4

  int32 len;
  void **list = a.getList(len);
  for (int i = 0; i < len; i++) {
    printf("%s {%s} ",(char *)list[0], (char *)list[1]);
    switch (((char *)list[0])[0]) {
    case 'V':
      printf("%p\n",list[2]);
      break;
    case 'S':
      printf("%s\n",(char *)list[2]);
      break;
    case 'D':
      printf("%21.17g\n",*((double *)list[2]));
      break;
    case 'F':
      printf("%21.17f\n",*((float *)list[2]));
      break;
    case 'I':
      printf("%d\n",*((int *)list[2]));
      break;
    case 'L':
      printf("%qd\n",*((long long *)list[2]));
      break;
    case 'B':
      printf("%d\n",*((int *)list[2]));
      break;
    default:
      printf("BOGOSITY\n");
      break;
    }
    list += 3;
  }
  exit(0);
}
#endif
