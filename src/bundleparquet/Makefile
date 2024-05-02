# Taken from:
# Open Source Security Foundation (OpenSSF), “Compiler Options Hardening Guide
# for C and C++,” OpenSSF Best Practices Working Group. Accessed: Dec. 01,
# 2023. [Online]. Available:
# https://best.openssf.org/Compiler-Hardening-Guides/Compiler-Options-Hardening-Guide-for-C-and-C++.html
CXXFLAGS=-std=c++2b -g -fno-omit-frame-pointer $(if $(DEVMODE),-Werror,)\
	-O2 -Wall -Wformat=2 -Wconversion -Wtrampolines -Wimplicit-fallthrough \
	-U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=3 \
	-D_GLIBCXX_ASSERTIONS \
	-fstrict-flex-arrays=3 \
	-fstack-clash-protection -fstack-protector-strong
LDFLAGS=-larrow -lcurl -lparquet -lprometheus-cpp-push -lprometheus-cpp-core -lz -ltmi8 -Wl,-z,defs \
	-Wl,-z,nodlopen -Wl,-z,noexecstack \
	-Wl,-z,relro -Wl,-z,now

bundleparquet: main.cpp spliturl.cpp
	$(CXX) -fPIE -pie -o $@ $^ $(CXXFLAGS) $(LDFLAGS)

.PHONY: clean
clean:
	rm bundleparquet
