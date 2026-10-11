VERSION  := 1.1.0
CXX      ?= g++
PREFIX   ?= /usr/local
PKG_CONFIG ?= pkg-config

CXXFLAGS ?= -O2
CXXFLAGS += -std=c++17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow \
            -fstack-protector-strong -fstack-clash-protection -fPIE \
            -D_FORTIFY_SOURCE=3 -D_GLIBCXX_ASSERTIONS \
            -DPHOTOSCRYPTO_VERSION='"$(VERSION)"' \
            $(shell $(PKG_CONFIG) --cflags libcrypto zlib)
LDFLAGS  += -pie -Wl,-z,relro,-z,now -Wl,-z,noexecstack
LDLIBS   += $(shell $(PKG_CONFIG) --libs libcrypto zlib)

GUI_CXXFLAGS = $(CXXFLAGS) $(shell $(PKG_CONFIG) --cflags gtk4 libadwaita-1)
GUI_LDLIBS   = $(LDLIBS) $(shell $(PKG_CONFIG) --libs gtk4 libadwaita-1)

CORE_SRC := $(wildcard src/core/*.cpp)
CLI_SRC  := $(wildcard src/cli/*.cpp)
GUI_SRC  := $(wildcard src/gui/*.cpp)

all: photoscrypto-cli photoscrypto-gui

photoscrypto-cli: $(CORE_SRC) $(CLI_SRC)
	$(CXX) $(CXXFLAGS) $(CORE_SRC) $(CLI_SRC) -o $@ $(LDFLAGS) $(LDLIBS)

photoscrypto-gui: $(CORE_SRC) $(GUI_SRC)
	$(CXX) $(GUI_CXXFLAGS) $(CORE_SRC) $(GUI_SRC) -o $@ $(LDFLAGS) $(GUI_LDLIBS)

test: photoscrypto-cli
	./tests/run_tests.sh ./photoscrypto-cli

install: photoscrypto-cli photoscrypto-gui
	install -Dm755 photoscrypto-cli $(DESTDIR)$(PREFIX)/bin/photoscrypto-cli
	install -Dm755 photoscrypto-gui $(DESTDIR)$(PREFIX)/bin/photoscrypto
	install -Dm644 data/io.github.hunte_av.PhotosCrypto.svg $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/io.github.hunte_av.PhotosCrypto.svg
	install -Dm644 io.github.hunte_av.PhotosCrypto.desktop $(DESTDIR)$(PREFIX)/share/applications/io.github.hunte_av.PhotosCrypto.desktop
	install -Dm644 io.github.hunte_av.PhotosCrypto.metainfo.xml $(DESTDIR)$(PREFIX)/share/metainfo/io.github.hunte_av.PhotosCrypto.metainfo.xml

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/photoscrypto-cli
	rm -f $(DESTDIR)$(PREFIX)/bin/photoscrypto
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/io.github.hunte_av.PhotosCrypto.svg
	rm -f $(DESTDIR)$(PREFIX)/share/applications/io.github.hunte_av.PhotosCrypto.desktop
	rm -f $(DESTDIR)$(PREFIX)/share/metainfo/io.github.hunte_av.PhotosCrypto.metainfo.xml

clean:
	rm -f photoscrypto-cli photoscrypto-gui

.PHONY: all test install uninstall clean
