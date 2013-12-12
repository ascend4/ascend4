#!/bin/bash
echo "Fluids list:"
ls *.c | cut -d "." -f 1 | sed 's/\(.*\)/{{fpropsfluid|\1}},/g'
echo "Total: "
ls *.c | cut -d "." -f 1 | sed 's/\(.*\)/{{fpropsfluid|\1}},/g' | wc -l

