
prefix     = /usr/local
bindir     = $(prefix)/bin
incdirs    = $(prefix)/include
libdirs    = $(prefix)/lib64 $(prefix)/lib
rpaths     = '$$ORIGIN/../lib64' '$$ORIGIN/../lib'

CFLAGS     = -Wall -std=gnu99
CPPFLAGS   = $(addprefix -I,$(incdirs))
LDFLAGS    = $(addprefix -L,$(libdirs)) -lsolvext -lsolv -lsqlite3

-include config.mk

ifneq ($(shell uname), Darwin)
  , := ,
  space :=
  space +=
  LDFLAGS += -Wl,-z,origin $(addprefix -Wl$(,)-rpath$(,),$(rpaths))
  SOD_ENV = LD_LIBRARY_PATH="$(subst $(space),:,$(libdirs)):$$LD_LIBRARY_PATH"
endif

sod: sod.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $^ $(LDFLAGS)

.PHONY: check clean install test

check test: sod
	bash test_module.bash
	$(SOD_ENV) bash test_sod

clean:
	-rm sod test.sodrepo

install: sod
	install sod $(bindir)

