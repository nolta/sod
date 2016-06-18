# sod

## Prerequisites

* [libsolv](https://github.com/openSUSE/libsolv)
* sqlite3
* python 2.7
* C compiler
* bash

## Building

    $ echo 'prefix = /path/to/dir' > config.mk
    $ make
    $ make test

The build system assumes that `libsolv` is installed in `$prefix`.

## Usage

All modifications to repos are done through `sodrepo`.

### Creating a repo

    $ sodrepo -r FILENAME create

### Adding a module

    $ sodrepo -r FILENAME add NAME VERSION RELEASE ARCH SUMMARY

