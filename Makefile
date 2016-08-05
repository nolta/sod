
prefix     = /usr/local
bindir     = $(prefix)/bin
incdirs    = $(prefix)/include
libdirs    = $(prefix)/lib64 $(prefix)/lib
rpaths     = '$$ORIGIN/../lib64' '$$ORIGIN/../lib'

CFLAGS     = -Wall -std=gnu99
CPPFLAGS   = $(addprefix -I,$(incdirs))
LDFLAGS    = $(addprefix -L,$(libdirs)) -lsolvext -lsolv -lsqlite3 -lz

-include config.mk

TEST_ENV = PATH="$$PWD:$$PATH"
ifneq ($(shell uname), Darwin)
  , := ,
  space :=
  space +=
  LDFLAGS += -Wl,-z,origin $(addprefix -Wl$(,)-rpath$(,),$(rpaths))
  TEST_ENV += LD_LIBRARY_PATH="$(subst $(space),:,$(libdirs)):$$LD_LIBRARY_PATH"
endif

sod: sod.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $^ $(LDFLAGS)

.PHONY: check clean install test

check test: sod
	@$(TEST_ENV) bash test_sod
	@$(TEST_ENV) bash test_module.bash

clean:
	-rm sod test.sodrepo

install: sod
	install sod $(bindir)

