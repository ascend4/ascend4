#include <Python.h>
#include "set.h"
#include "symchar.h"
using namespace std;

class SetInt: public ASCXX_Set<long>
{
public:
	SetInt();
	~SetInt();
	unsigned long length() const;
	inline const long at(const unsigned long &index) const;
	long __getitem__(int i);
};

class SetString: public ASCXX_Set<SymChar>
{
public:
	SetString();
	~SetString();
	unsigned long length() const;
	inline const SymChar at(const unsigned long &index) const;
	SymChar __getitem__(int i);
};
