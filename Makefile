
libsolv        = $(prefix)
libsolv_inc    = $(libsolv)/include
libsolv_lib    = $(libsolv)/lib64
libsqlite3     = /usr
libsqlite3_inc = $(libsqlite3)/include
libsqlite3_lib = $(libsqlite3)/lib64
prefix         = /usr/local
rpath          = $(shell realpath $(libsolv_lib))

CFLAGS         = -g -std=gnu99 -I$(libsolv_inc) -I$(libsqlite3_inc)
LDFLAGS        = -L$(libsolv_lib) -lsolvext -lsolv \
		 -L$(libsqlite3_lib) -lsqlite3 \
		 -Wl,-rpath=$(rpath)

sod: sod.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

.PHONY: check test
check test:
	bash test_module.bash
	bash test_sod

