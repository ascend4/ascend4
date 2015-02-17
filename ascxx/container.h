#include <Python.h>
#include <vector>
#include <string>
#include <map>
#include <set>
using namespace std;

class ASCVector
{
	public:
		ASCVector();
		~ASCVector();
		int push(PyObject * o);
		void pop();
		int  size();
		void clear();
		PyObject * at(int index);
		PyObject * __getitem__(int i) const;
		void __setitem__(int i, PyObject * o);
	private:
		vector<PyObject *> _v;
		string _tn;//type name
};
class pyobjectCmp {
	public:
  	bool operator() (const PyObject * k1, const PyObject * k2) const
  	{
		return k1 < k2;//only consider their address in mem. Could implement a callback function of python layer to do real compare work
  	}
};
class ASCSet
{
	public:
		ASCSet();
		~ASCSet();
		int size();
		int insert(PyObject * e);
		int erase(PyObject * e);
		void clear();
		int count(PyObject * e);
		PyObject * at(int i);
		PyObject * __getitem__(int i);
		PyObject * operator[](unsigned long &index);
	private:
		set<PyObject *, pyobjectCmp > _s;
		
		string _tn;//type name
};


class ASCMap
{
	public:
		ASCMap();
		~ASCMap();
		int count(PyObject * key);
		bool empty();
		int erase(PyObject * key);
		bool find(PyObject * key);
		int insert(PyObject * key, PyObject * value);
		int size();
		void clear();
		PyObject * __getitem__(PyObject* key);
		PyObject * operator[](PyObject* key);
		
	private:
		map< PyObject *, PyObject*, pyobjectCmp > _m;
		string _tn_key;//type name 
		string _tn_value;//type name
};
