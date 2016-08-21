#!/bin/sh

# This test checks if the AST pretty printer can "round trip" and 
# produce a token stream equivalent to the one we get from the lexer.

mydir="$(dirname "$0")"
cd "$mydir"

mkdir -p ./lexed
mkdir -p ./parsed
rm -rf ./lexed/* ./parsed/*

ok=1
for file in $(find ../../models -iname "*.a4c"); do
  flatname=$(echo "$file" | sed -e 's#^\.\./\.\./models/##' -e 's#/#-#g')
  
  if ! ../test_tokens.byte lexer < "$file" > "./lexed/$flatname"; then
    echo "Lexer error in file $file"
    echo
    ok=0
    continue
  fi

  if ! ../test_tokens.byte parser < "$file" > "./parsed/$flatname"; then
    echo "Parser error in file $file"
    echo
    ok=0
    continue
  fi
  
  if ! diff "./lexed/$flatname" "./parsed/$flatname" > /dev/null; then
    echo "Mismatch found in $file"
    echo
    ok=0
    continue
  fi
done

if [ "$ok" = 1 ]; then
  echo "Everything looks fine!"
fi
