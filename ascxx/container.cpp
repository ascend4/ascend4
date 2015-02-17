#include "container.h"
#include <iostream>

ASCVector::ASCVector()
{

}

ASCVector::~ASCVector()
{
	clear();
}

int ASCVector::push(PyObject * o)
{
	//cout<<(char *)o->ob_type->tp_name<<endl;
	//type check
	if(_tn.length() == 0)
		_tn = (char *)o->ob_type->tp_name;
	else if(_tn.compare((char *)o->ob_type->tp_name) != 0)
	{
		return -1;
	}
	Py_INCREF(o);
	_v.push_back(o);
	return 0;
}

void ASCVector::pop()
{
	if(_v.size() <= 0) return;
	Py_DECREF(_v[_v.size()-1]);
	_v.pop_back();
}

int ASCVector::size()
{
	return _v.size();
}
void ASCVector::clear()
{
	for(int i = 0 ; i < _v.size() ; i++)
	{
		Py_DECREF(_v[_v.size()-1]);
		_v.pop_back();
	}
}
PyObject * ASCVector::at(int index)
{
	if(index >= size()) return NULL;
	return _v.at(index);
}

PyObject * ASCVector::__getitem__(int i) const
{
	return _v.at(i);
}

void ASCVector::__setitem__(int i, PyObject * o)
{
	_v[i] = o;
}

///SET
ASCSet::ASCSet()
{
}
ASCSet::~ASCSet()
{
	clear();
}
int ASCSet::size()
{
	return _s.size();
}
int ASCSet::insert(PyObject * o)
{
	//cout<<(char *)o->ob_type->tp_name<<endl;
	//type check
	if(_tn.length() == 0)
		_tn = (char *)o->ob_type->tp_name;
	else if(_tn.compare((char *)o->ob_type->tp_name) != 0)
	{
		return -1;
	}
	Py_INCREF(o);
	_s.insert(o);
	return 0;
}
int ASCSet::erase(PyObject * e)
{
	if(_s.find(e) != _s.end()){
		Py_DECREF(e);
		_s.erase(e);
		return 0;
	}
	return -1;// nothing is to be erased 
}
void ASCSet::clear()
{
	set< PyObject * >::iterator it;
	for(it = _s.begin(); it != _s.end(); it++)
	{
		Py_DECREF(*it);
		_s.erase(*it);
	}
}
int ASCSet::count(PyObject * e)
{
	return _s.count(e);
}
PyObject * ASCSet::at(int i)
{
	int k = 0;
	set< PyObject * >::iterator it;
	for(it = _s.begin(); it != _s.end() && k <= i; it++)
	{
		if(k == i) 
			return (*it);
	}
	return NULL;
}
PyObject * ASCSet::__getitem__(int i)
{
	return at(i);
}
PyObject * ASCSet::operator[](unsigned long &index)
{
	return at(index);
}

///MAP
ASCMap::ASCMap()
{}
ASCMap::~ASCMap()
{
	clear();
}
int ASCMap::count(PyObject * key)
{
	return _m.count(key);
}
bool ASCMap::empty()
{
	return _m.empty();
}
int ASCMap::erase(PyObject * key)
{
	if(_m.find(key) != _m.end()){
		Py_DECREF(key);
		Py_DECREF(_m[key]);
		_m.erase(key);
		return 0;
	}
	return -1;
}
bool ASCMap::find(PyObject * key)
{
	if(key == NULL) return false;
	return _m.find(key) != _m.end();
}
int ASCMap::insert(PyObject * key, PyObject * value)
{
	//cout<<(char *)o->ob_type->tp_name<<endl;
	if(key == NULL || value == NULL) return -1;
	string kt = (char *)key->ob_type->tp_name;
	string vt = (char *)value->ob_type->tp_name;
	//type check
	if(_tn_key.length() == 0)
	{
		_tn_key = kt;
		_tn_value = vt;
	}
	else if(_tn_key.compare(kt) != 0 || _tn_value.compare(vt) != 0)
	{
		return -1;
	}
	Py_INCREF(key);
	Py_INCREF(value);
	_m.insert(pair< PyObject *, PyObject* >(key,value));
	return 0;
}
int ASCMap::size()
{
	return _m.size();
}
void ASCMap::clear()
{
	map< PyObject *, PyObject*, pyobjectCmp >::iterator it;
	for(it = _m.begin(); it != _m.end(); it++)
	{
		Py_DECREF((*it).first);
		Py_DECREF((*it).second);
		_m.erase((*it).first);
	}
}
PyObject * ASCMap::__getitem__(PyObject* key)
{
	return key == NULL ? NULL:_m[key];
}
PyObject * ASCMap::operator[](PyObject* key)
{
	return key == NULL ? NULL:_m[key];
}
		
