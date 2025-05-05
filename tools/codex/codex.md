# Using Codex with ASCEND

You can use OpenAI's `codex` with ASCEND to help understand code or even to 
write code. These instructions are for running Codex inside a recent Ubuntu
docker, which ensures that only files within your working directory can be
modified by codex. This approach doesn't lock down network access. (You can
consider their 'docker-in-docker' approach if that's important for you.)

## Building the docker image

The Dockerfile in this directory provides a clean-room build environment for
ASCEND (although it's up to you to clear out results from earlier builds you 
might have done in your host system).

To build the docker:

docker build -t ascend .

Note that this docker includes a copy of codex, as well as all of the ASCEND
build dependencies. It also includes a copy of CUnit, built from source, which
it downloads using subversion from Sourceforge (old school!)

# Running the docker and building the codebase within the docker

To run the docker, map your working folder (here, ~/ascend/git) and set the
user/group to yourself as follows:

docker run -it -v ~/ascend/git:/app --user "$(id -u):$(id -g)" -w /app ascend

To build in the docker, note that CUNIT is installed in /usr/local and you may
need to explicitly tell SCons to look there:

scons CUNIT_PREFIX=/usr/local -j8

# Running Codex

To run Codex, create a .env file in your ASCEND working folder (ie ~/ascend/git)
containing your OpenAI API key:

OPENAI_API_KEY=sk_.......

Then run codex as follows:

codex "please explain this codebase"

and it should work. Please let us know if there are any problems with these
instructions!

-- 
John Pye
May 2025

