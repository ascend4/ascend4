#include "annotation.h"

extern "C"{
#include <ascend/compiler/notate.h>
#include <ascend/compiler/notequery.h>
#include <ascend/utilities/error.h>
#include <ascend/general/panic.h>
}

#include <stdexcept>
#include <iostream>

using namespace std;

AnnotationDatabase::AnnotationDatabase(const SymChar &dbid){
	this->dbid = dbid.getInternalType();
}

/**
	This is a C++ wrapper for the GetNotes function from notate.h.
	It doesn't allow access to the full range of functionality from GetNotes
	though.
*/
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

	gl_destroy(res);
	
	return v;
}

/**
	Return the note corresponding to a variable within a type. This function
	is cogniscent of the type hierarchy; it will locate the note on this
	variable in the most refined parent type of 'type' for which it is present.
	This makes it possible to define a note in a base-type and then access it
	from a more refined type.
*/
const char *
AnnotationDatabase::getNoteForVariable(const Type &type
		, const SymChar *varname
		, const SymChar *lang
){
	return notes_get_for_variable(this->dbid, type.getInternalType(), varname->getInternalType(), lang->getInternalType());
}

/**
	Return most-refined note corresponding all variables within a type. Only
	notes having 'language' lang are returned.
*/
vector<Annotation> 
AnnotationDatabase::getTypeRefinedNotesLang(const Type &type
		, const SymChar *lang
){
	struct gl_list_t *l = notes_refined_for_type_with_lang(
		this->dbid
		, type.getInternalType()
		, lang->getInternalType()
	);

	vector<Annotation> v;
	if(l != NULL){
		for(unsigned i=1; i<=gl_length(l); ++i){
			v.push_back(Annotation((struct Note *)gl_fetch(l,i)));
		}
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
