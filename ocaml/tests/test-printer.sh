#!/bin/sh

# This test checks if the AST pretty printer can "round trip" and 
# produce a token stream equivalent to the one we get from the lexer.

mydir="$(dirname "$0")"
cd "$mydir"

mkdir -p ./lexed
mkdir -p ./parsed
rm -rf ./lexed/* ./parsed/*

ok=1
for file in ../../models/ksenija/*.a4c; do
  name="$(basename "$file")"
  ../testTokens.byte lexer  < "$file" > "./lexed/$name"
  ../testTokens.byte parser < "$file" > "./parsed/$name"
  if ! diff "./lexed/$name" "./parsed/$name" > /dev/null; then
    echo "Mismatch found in $name"
    ok=0
  fi
done

if [ "$ok" = 1 ]; then
  echo "Everything looks fine!"
fi
