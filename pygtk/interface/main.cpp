#include <unistd.h>

#include "library.h"
#include "type.h"
#include "instance.h"
#include "symchar.h"
#include "reporter.h"

void esc_seq(char *s="0",FILE *f=stderr){
	fprintf(f,"%c[%sm",char(27),s);
}

int red_error(ERROR_REPORTER_CALLBACK_ARGS){
	bool red=false;
	if((int)sev >= (int)ASC_PROG_NOTE) red=true;

	if(red)esc_seq("31;2");
	vfprintf(stderr,fmt,args);
	if(red)esc_seq();
}

int main(void){
	Reporter *reporter = Reporter::Instance();
	reporter->setErrorCallback(red_error);

	Library *l;
	l = new Library();
	l->load("simple_fs.a4c");
	Type t;
	t = l->findType("test_controller");
	Instanc i = t.getInstance("tc"); // speling is deliberate
	//i.solve();
}

