# sod

## Prerequisites

* [libsolv](https://github.com/openSUSE/libsolv)
* sqlite3
* python 2.7

## Building

    $ echo 'prefix = /path/to/dir' > config.mk
    $ make
    $ make test

The build system assumes that `libsolv` is installed in `$prefix`.

