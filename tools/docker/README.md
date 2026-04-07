A docker for testing ASCEND on 22.04:

docker build -f Dockerfile-22.04 -t ascend:22.04 .

docker run --rm -it \
    -v "$HOME/ascend:/mnt/ascend:ro" \
    -v ascend-work-22.04:/work \
    ascend:22.04
    
inside that shell:

scons GCOV=1 CUNIT_PREFIX=$HOME/.local MALLOC_DEBUG=1 ascend models solvers ascxx pygtk test a4 -j2

or simpler/faster:

scons DEBUG=1 CUNIT_PREFIX=$HOME/.local MALLOC_DEBUG=1 test -j2

for a clean rebuild:

docker volume rm ascend-work-22.04

to resync files from ~/ascend into the docker, just exit the docker shell and restart it.

