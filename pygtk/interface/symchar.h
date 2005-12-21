#ifndef ASCXX_SYMCHAR_H
#define ASCXX_SYMCHAR_H 

#include <iostream>
#include <string>

extern "C"{
#include <utilities/ascConfig.h>
#include <compiler/compiler.h>
#include <compiler/symtab.h>
#include <compiler/instance_name.h>
}

/**
	A container for ASCEND strings, used to ensure
	that two identical strings created via the SymChar interface
	will only be stored once in memory, and can be tested for
	equality using pointer comparison alone.

	 @TODO @bug FIXME There is a problem with the calling conventions of char * and symchar*
	 -- C++ can't tell which one you're trying to use.
*/
class SymChar{
private:
	const symchar *sc;
public:
	//SymChar();
	SymChar(const std::string &name);
	SymChar(const char *name);
	SymChar(const symchar *name); // never gets used
	SymChar(const SymChar &);
	SymChar(const struct InstanceName &);
	bool operator==(const SymChar &) const;

	const char *toString() const;
	const symchar *getInternalType() const;
};

std::ostream& operator << (std::ostream& os, const SymChar& s);

#endif
