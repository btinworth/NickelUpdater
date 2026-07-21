include NickelHook/NickelHook.mk

override LIBRARY  := libnickelupdater.so
override SOURCES  += $(sort $(wildcard src/*.cc))
override MOCS     += $(sort $(shell grep -l 'Q_OBJECT' src/*.h 2>/dev/null || true))
override CFLAGS   += -Wall -Wextra -Werror
override CXXFLAGS += -Wall -Wextra -Werror -Wno-missing-field-initializers
override PKGCONF  += Qt5Network

override KOBOROOT += config/nickelupdater.conf.tmpl:/usr/local/nickelupdater/nickelupdater.conf.tmpl

include NickelHook/NickelHook.mk
