
libsolv        = $(prefix)
libsolv_inc    = $(libsolv)/include
libsolv_lib    = $(libsolv)/lib64
libsqlite3     = /usr
libsqlite3_inc = $(libsqlite3)/include
libsqlite3_lib = $(libsqlite3)/lib64
prefix         = /usr/local
uname          = $(shell uname)

CFLAGS         = -g -Wall -std=gnu99 -I$(libsolv_inc) -I$(libsqlite3_inc)
LDFLAGS        = -L$(libsolv_lib) -lsolvext -lsolv \
		 -L$(libsqlite3_lib) -lsqlite3

ifeq ($(uname), Darwin)
  libsqlite3_lib = $(libsqlite3)/lib
else
  rpath = $(shell realpath $(libsolv_lib))
  LDFLAGS += -Wl,-rpath=$(rpath)
endif

sod: sod.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

.PHONY: check test
check test:
	bash test_module.bash
	bash test_sod

