OS_NAME = $(shell uname -s)
#darwin or  linux
LC_OS_NAME = $(shell echo $(OS_NAME) | tr '[A-Z]' '[a-z]')

MAKE_PLATFORM = pwd
ifeq ($(LC_OS_NAME), darwin)
	COMPILE_BUILD = clang
else
	COMPILE_BUILD = gcc
endif


all:
	$(COMPILE_BUILD) -D_FORTIFY_SOURCE=2 -fPIC -c crypto_scrypt-nosse.c sha256.c crypto-mcf.c b64.c crypto-scrypt-saltgen.c crypto_scrypt-check.c crypto_scrypt-hash.c slowequals.c
	ar rcs libscrypt.a crypto_scrypt-nosse.o sha256.o crypto-mcf.o b64.o crypto-scrypt-saltgen.o crypto_scrypt-check.o crypto_scrypt-hash.o slowequals.o
	
clean:
	rm -rf *.o *.a 