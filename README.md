# Sod: a modern environment module system

[![Build Status](https://travis-ci.org/nolta/sod.svg?branch=master)](https://travis-ci.org/nolta/sod)

*Status: alpha / experimental*

Sod is an environment module system like
[modules](http://modules.sourceforge.net) or
[Lmod](https://www.tacc.utexas.edu/research-development/tacc-projects/lmod),
with the following features:

* **rich dependencies**: sod uses [libsolv][libsolv], a SAT solver designed
for linux package managers, to resolve module dependencies.

* **single-file repositories**: instead of spreading module files over a bunch
of directories, sod stores them in a single file. This cuts down on IOPS,
and speeds things up on shared filesystems.

* **declarative syntax**: since modules can be unloaded as well as loaded, sod
only supports a limited set of reversible operations.

* **no side effects**: modules can only change the environment, nothing else.

* **module versioning**

## Prerequisites

* [libsolv][libsolv]
* sqlite3
* python 2.7
* C compiler
* bash >= 3.2
* Linux or macOS

[libsolv]: https://github.com/openSUSE/libsolv

## Building

First, decide on the destination:

    $ prefix="/path/to/dir"

Next, build libsolv:

    $ git clone git://github.com/openSUSE/libsolv
    $ cd libsolv
    $ mkdir build
    $ cd build
    $ cmake -DCMAKE_INSTALL_PREFIX:PATH=$prefix ..
    $ make install

Finally, build sod:

    $ echo "prefix = $prefix" > config.mk
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

    $ sodrepo test.repo create --arch=x86_64

Next let's add a module. The command is:

    $ sodrepo FILENAME add NAME/VERSION-RELEASE SUMMARY < SCRIPT

For example:

    $ sodrepo test.repo add X/1.0-1 "X marks the spot" <<EOF
    # a comment
    @ /x/y/z
    + FOO @/bin
    = BAR @/lib
    EOF

This adds a module named 'X'; with version '1.0',
release '1', summary 'X marks the spot';
and a script that when loaded does two things:

1. prepends `/x/y/z/bin` to the environment variable `FOO`

2. sets the environment variable BAR to `/x/y/z/lib`

### The module command

Now let's load the `module` command:

    $ source module.bash
    $ module --version
    sod 0.0.0

Point sod at our repo file:

    $ module use test.repo

See what modules are available:

    $ module avail
    X/1.0-1.x86_64

Load a module:

    $ module load X
    loading X/1.0-1.x86_64@test

After loading, the environment variables `FOO` and `BAR` are changed:

    $ env | egrep 'FOO|BAR'
    FOO=/a/b/c/bin
    BAR=/x/y/z/lib

List all loaded modules:

    $ module list
    X/1.0-1.x86_64

### Dependencies

For a more complex example, let's add the following four packages
(omitting some arguments for clarity):

    $ sodrepo add gcc --provides='compiler'
    $ sodrepo add intel --provides='compiler'
    $ sodrepo add hdf5-gcc --provides='hdf5' --requires='gcc'
    $ sodrepo add hdf5-intel --provides='hdf5' --requires='intel'

Now when we load the `hdf5-intel` module the `intel` module is automatically
installed:

    $ module load hdf5-intel
    loading intel/16.0.2-1.x86_64@test
    loading hdf5-intel/1.8.11-1.x86_64@test

Likewise, if we swap out `hdf5-intel` for `hdf5-gcc`, sod automatically
unloads and loads the dependencies:

    $ module swap hdf5-gcc
    unloading hdf5-intel/1.8.11-1.x86_64@test
    unloading intel/16.0.2-1.x86_64@test
    loading gcc/5.2.0-1.x86_64@test
    loading hdf5-gcc/1.8.11-1.x86_64@test

Furthermore, if the `intel` module is already loaded, and we ask to load
`hdf5`, sod can install the right module:

    $ module load intel
    loading intel/16.0.2-1.x86_64@test
    $ module load hdf5
    loading hdf5-intel/1.8.11-1.x86_64@test

Dependencies can constrain versions, for example:

    --requires='intel >= 15.1'

These constraints can also be used with some of the module commands:

    $ module avail 'intel < 16'

## FAQ

1. *How can i save the currently loaded modules, and reload them later?*

        $ module list > my_modules.txt
        ...
        $ module load $(cat my_modules.txt)

