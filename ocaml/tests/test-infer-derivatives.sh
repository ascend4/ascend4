#!/bin/sh

# This tests that the derivative inferrer doesn't crash on any inputs
# You can then run ascend on the generated files to see if they work
# as expected

mydir="$(dirname "$0")"
cd "$mydir"

mkdir -p ./inferred-derivatives

for file in $(find ../../models -iname "*.a4c"); do
  flatname=$(echo "$file" | sed -e 's#^\.\./\.\./models/##' -e 's#/#-#g')
  if ! ../infer_derivatives.byte < "$file" > "./inferred-derivatives/$flatname"; then
    echo "Error in file $file"
    echo
  fi
done
