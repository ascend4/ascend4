
#include <emscripten.h>

extern "C" {

// Not using size_t for array indices as the values used by the javascript code are signed.

EM_JS(void, array_bounds_check_error, (size_t idx, size_t size), {
  throw 'Array index ' + idx + ' out of bounds: [0,' + size + ')';
});

void array_bounds_check(const int array_size, const int array_idx) {
  if (array_idx < 0 || array_idx >= array_size) {
    array_bounds_check_error(array_idx, array_size);
  }
}

// VoidPtr

void EMSCRIPTEN_KEEPALIVE emscripten_bind_VoidPtr___destroy___0(void** self) {
  delete self;
}

// Bar

Bar* EMSCRIPTEN_KEEPALIVE emscripten_bind_Bar_Bar_0() {
  return new Bar();
}

int EMSCRIPTEN_KEEPALIVE emscripten_bind_Bar_getf_0(Bar* self) {
  return self->getf();
}

void EMSCRIPTEN_KEEPALIVE emscripten_bind_Bar_setf_1(Bar* self, int i) {
  self->setf(i);
}

void EMSCRIPTEN_KEEPALIVE emscripten_bind_Bar___destroy___0(Bar* self) {
  delete self;
}

// Foo

Foo* EMSCRIPTEN_KEEPALIVE emscripten_bind_Foo_Foo_0() {
  return new Foo();
}

int EMSCRIPTEN_KEEPALIVE emscripten_bind_Foo_getf_0(Foo* self) {
  return self->getf();
}

void EMSCRIPTEN_KEEPALIVE emscripten_bind_Foo_setf_1(Foo* self, int i) {
  self->setf(i);
}

void EMSCRIPTEN_KEEPALIVE emscripten_bind_Foo___destroy___0(Foo* self) {
  delete self;
}

}

