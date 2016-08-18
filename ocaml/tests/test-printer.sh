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
  ../testTokens.byte lexer  < "$file" > "./lexed/$flatname"
  ../testTokens.byte parser < "$file" > "./parsed/$flatname"
  if ! diff "./lexed/$flatname" "./parsed/$flatname" > /dev/null; then
    echo "Mismatch found in $file"
    ok=0
  fi
done

if [ "$ok" = 1 ]; then
  echo "Everything looks fine!"
fi
