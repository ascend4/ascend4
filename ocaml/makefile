# This makefile is a dumb wrapper around ocamlbuild so "make" and "make clean" work.
# I put it here for documentation. IMO, its easier to just run ocamlbuild directly
# instead of using this makefile.

.PHONY: all clean

all:
	ocamlbuild -use-ocamlfind test_tokens.byte
	ocamlbuild -use-ocamlfind infer_derivatives.byte

clean:
	ocamlbuild -clean
	rm -rf tests/lexed tests/parsed tests/inferred-derivatives
