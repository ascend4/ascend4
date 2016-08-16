What is this code?
==================

In this folder we have an alternate parser for the ASCEND grammar, written in
Ocaml. The motivation for this parser is that it should be simpler than the C
one, since all it just builds a syntax tree without doing any additional
processing and because Ocaml is better suited for playing around with trees
than C is (algebraic data types and pattern matching help a lot).

So far, the parser can parse a4c files and build an AST from them and we can
convert this AST back to an a4c source (although an unreadable one because we
don't try to pretty print it). The original plan was to also create some
AST-to-AST transformations to experiment with new language feature without
needing to hack in the C code for ASCEND but unfortunately, that didn't happen
yet.

How to build
============

Building instructions can be found in the makefile. However, we depend on some
libraries that have to be installed with Opam, the Ocaml package manager. To
install Opam, follow the instructions on http://opam.ocaml.org/. After that,
install the dependencies with

    opam install menhir containers sequence

Menhir a bottom up parser generator. Its amazing and is much better than
the parser generator that comes with the Ocaml standard library.

"containers" is a (lightweight) overlay over the Ocaml standard library. The
Ocaml standard library is kind of small and inconsistent and "containers"
improves that.

The "sequence" data type has an efficient concatenation operation, which we need
for the ast-to-token-stream functionality.



