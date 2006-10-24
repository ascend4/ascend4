#ifndef ASCXX_ANNOTATION_H
#define ASCXX_ANNOTATION_H

#include "symchar.h"
#include "type.h"

#include <vector>

class Annotation{
private:
	struct Note *n;
public:
	Annotation();
	Annotation(struct Note *);
	~Annotation();
	
	const char *getId() const;
	const char *getMethod() const;
	const SymChar getType() const;
	const char *getFilename() const;
	const int getLineNumber() const;
	const char *getText() const;
	const int getDataType() const;
	//const void *getUserData(const int &datatype) const;
};

class AnnotationDatabase{
private:
	const symchar *dbid;
public:
	AnnotationDatabase(const SymChar &dbid);
	std::vector<Annotation> getNotes(const Type &type, const SymChar *lang=NULL
		, const SymChar *id=NULL, const SymChar *method=NULL, const int flag=0
	);
};

#endif
