#include "foo.h"

Foo::Foo()
{
    f = 1;
}

int Foo::getf()
{
    return f;
}

void Foo::setf(int i)
{
    f = i;
}