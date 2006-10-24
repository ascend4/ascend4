#include "annotation.h"

extern "C"{
#include <compiler/notate.h>
#include <utilities/error.h>
#include <utilities/ascPanic.h>
}

#include <stdexcept>
#include <iostream>

using namespace std;

AnnotationDatabase::AnnotationDatabase(const SymChar &dbid){
	this->dbid = dbid.getInternalType();
}

vector<Annotation>
AnnotationDatabase::getNotes(const Type &type, const SymChar *lang
	, const SymChar *id, const SymChar *method, const int datatypeint
){
	struct gl_list_t *res;
	if(datatypeint < (int)nd_wild && datatypeint > (int)nd_anonymous){
		throw runtime_error("Invalid datatype flag");
	}

	symchar *lang1, *id1, *method1;

	if(lang==NULL)lang1 = NOTESWILD;
	else lang1 = lang->getInternalType();

	if(id==NULL)id1 = NOTESWILD;
	else id1 = id->getInternalType();

	if(method==NULL)method1 = NOTESWILD;
	else method1 = method->getInternalType();
	
	res = GetNotes(dbid, type.getName().getInternalType(), lang1, id1, method1, (enum NoteData)datatypeint);

	if(res==NULL){
		throw runtime_error("NULL from GetNotes");
	}

	vector<Annotation> v;
	for(unsigned i=1; i<=gl_length(res); ++i){
		v.push_back(Annotation((struct Note *)gl_fetch(res,i)));
	}
	
	return v;
}

Annotation::Annotation(){
	n = NULL;
	cerr << "Created Annotation with NULL Note" << endl;
}

Annotation::~Annotation(){
	// do nothing
}

Annotation::Annotation(struct Note *n){
	this->n = n;
}

const char *
Annotation::getId() const{
	symchar *m = GetNoteId(n);
	if(m==NULL){
		return NULL;
	}
	return SCP(m);
}

/**
	@return the name of the method to which this note was assigned, or NULL if the note was not assigned to a method.
*/
const char *
Annotation::getMethod() const{
	symchar *m = GetNoteMethod(n);
	if(m==NULL){
		return NULL;
	}
	return SCP(m);
}

/**
	@return the name of the type to which this note is assigned
*/
const SymChar
Annotation::getType() const{
	asc_assert(GetNoteType(n)!=NULL);
	return SymChar(SCP(GetNoteType(n)));
}

/**
	Get the 'language' of a note. This roughly corresponds to the *purpose*
	that the note exists for: informing the end-user, providing parameters to
	the solver, giving additional data such as icons for a dreamt-of flowsheet
	GUI, etc.

	@return the 'language' of the note
*/
const SymChar
Annotation::getLanguage() const{
	asc_assert(GetNoteLanguage(n)!=NULL);
	return SymChar(SCP(GetNoteLanguage(n)));
}

/**
	@return the filename to which this note is assigned (where it was declared)
*/
const char *
Annotation::getFilename() const{
	return GetNoteFilename(n);
}

/**
	@return the line number where this note was assigned
*/
const int 
Annotation::getLineNumber() const{
	return GetNoteLineNum(n);
}

/**
	@return the text content of this note (might be quite big, possibly)
*/
const char *
Annotation::getText() const{
	return BraceCharString(GetNoteText(n));
}

/**
	@return the data type code for this note. Note sure what can be done with
	this just yet.
*/
const int
Annotation::getDataType() const{
	return (int)GetNoteEnum(n);
}
