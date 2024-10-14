#include "foo.h"
#include "bar.h"

int main() {
    Foo f;
    Bar b;
    int x = f.getf();
    b.setf(x);
    return 0;
}