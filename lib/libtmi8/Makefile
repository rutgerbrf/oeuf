# Taken from:
# Open Source Security Foundation (OpenSSF), “Compiler Options Hardening Guide
# for C and C++,” OpenSSF Best Practices Working Group. Accessed: Dec. 01,
# 2023. [Online]. Available:
# https://best.openssf.org/Compiler-Hardening-Guides/Compiler-Options-Hardening-Guide-for-C-and-C++.html
CXXFLAGS=-std=c++2b -g -fno-omit-frame-pointer -Iinclude $(if $(DEVMODE),-Werror,)\
	-O2 -Wall -Wformat=2 -Wconversion -Wtrampolines -Wimplicit-fallthrough \
	-U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=3 \
	-D_GLIBCXX_ASSERTIONS \
	-fstrict-flex-arrays=3 \
	-fstack-clash-protection -fstack-protector-strong
LDFLAGS=-larrow -lparquet -Wl,-z,defs \
	-Wl,-z,nodlopen -Wl,-z,noexecstack \
	-Wl,-z,relro -Wl,-z,now
DESTDIR=/usr/local

LIBHDRS=include/tmi8/kv1_lexer.hpp include/tmi8/kv1_parser.hpp include/tmi8/kv1_types.hpp include/tmi8/kv6_parquet.hpp
LIBSRCS=src/kv1_index.cpp src/kv1_lexer.cpp src/kv1_parser.cpp src/kv1_types.cpp src/kv6_parquet.cpp
LIBOBJS=$(patsubst %.cpp,%.o,$(LIBSRCS))

.PHONY: all install libtmi8 clean
all: libtmi8

libtmi8: libtmi8.a libtmi8.so

clean:
	rm libtmi8.a libtmi8.so $(LIBOBJS)

install: libtmi8.a $(LIBHDRS)
	install -D -m644 include/tmi8/* -t $(DESTDIR)/include/tmi8
	install -D -m644 libtmi8.a -t $(DESTDIR)/lib
	install -D -m644 libtmi8.so -t $(DESTDIR)/lib

src/%.o: src/%.cpp $(LIBHDRS)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

libtmi8.a: $(LIBOBJS)
	$(AR) rcs $@ $^

libtmi8.so: $(LIBOBJS)
	$(CXX) -shared -fPIC -o $@ $^ $(CXXFLAGS) $(LDFLAGS)
