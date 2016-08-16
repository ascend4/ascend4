#!/bin/sh

# Just see if the parser can parse some a4c files without blowing up

mydir="$(dirname "$0")"
cd "$mydir/.."

for file in ../models/ksenija/*.a4c; do
  echo "testing $file"
  ./testTokens.byte parser < "$file" > /dev/null
done
