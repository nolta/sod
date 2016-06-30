# Sod

[![Build Status](https://travis-ci.org/nolta/sod.svg?branch=master)](https://travis-ci.org/nolta/sod)

*Status: alpha / experimental*

Sod is an environment module system like
[modules](http://modules.sourceforge.net) or
[Lmod](https://www.tacc.utexas.edu/research-development/tacc-projects/lmod),
with the following features:

* **rich dependencies**: sod uses [libsolv][libsolv], a SAT solver designed
for linux package managers, to resolve module dependencies.

* **single-file repositories**: instead of spreading module files over a bunch
of directories, sod stores them in a single file. This allows logging changes,
and cuts down on IOPS.

* **declarative syntax**

## Prerequisites

* [libsolv][libsolv]
* sqlite3
* python 2.7
* C compiler
* bash >= 3.2
* Linux or macOS

[libsolv]: https://github.com/openSUSE/libsolv

## Building

    $ echo 'prefix = /path/to/dir' > config.mk
    $ make
    $ make test
    $ make install

The build system assumes that libsolv and the other dependencies are
installed either in `$prefix` or system wide.

## Quick start

### Creating a repo

A *repo* is a file containing a set of modules.
All modifications to repos are done via the `sodrepo` command.
First, let's create an empty repo named `test.repo`:

    $ sodrepo -r test.repo create

Next let's add a module. The command is:

    $ sodrepo -r FILENAME add NAME VERSION RELEASE ARCH SUMMARY < SCRIPT

For example:

    $ sodrepo -r test.repo add X 1.0 1 x86 "X marks the spot" <<EOF
    + FOO /a/b/c
    = BAR /x/y/z
    EOF

This adds a module named 'X'; with version '1.0',
release '1', architecture 'x86', summary 'X marks the spot';
and a script that when loaded does two things:

1. prepends '/a/b/c' to the environment variable 'FOO'

2. sets the environment variable BAR to '/x/y/z'

### The module command

Now let's load the `module` command:

    $ source module.bash
    $ module --version
    sod 0.0.0

Point sod at our repo file:

    $ module use test.repo

Let's see what modules are available:

    $ module avail
    X-1.0-1.x86

Load a module:

    $ module load X
    loading X-1.0-1.x86@test

After loading, the environment variables `FOO` and `BAR` are changed:

    $ env | egrep 'FOO|BAR'
    FOO=/a/b/c
    BAR=/x/y/z

List all loaded modules:

    $ module list
    X-1.0-1.x86

### Dependencies

For a more complex example, let's add the following four packages
(omitting irrelevant details):

    $ sodrepo add gcc --provides='compiler'
    $ sodrepo add intel --provides='compiler'
    $ sodrepo add hdf5-gcc --provides='hdf5' --requires='gcc'
    $ sodrepo add hdf5-intel --provides='hdf5' --requires='intel'

Now when we load the `hdf5-intel` module the `intel` module is automatically
installed:

    $ module load hdf5-intel
    loading intel-16.0.2-1.x86_64@test
    loading hdf5-intel-1.8.11-1.x86_64@test

Likewise, if we swap out `hdf5-intel` for `hdf5-gcc`, sod automatically
unloads and loads the dependencies:

    $ module swap hdf5-gcc
    unloading hdf5-intel-1.8.11-1.x86_64@test
    unloading intel-16.0.2-1.x86_64@test
    loading gcc-5.2.0-1.x86_64@test
    loading hdf5-gcc-1.8.11-1.x86_64@test

