include NickelHook/NickelHook.mk

override LIBRARY  := libnickelupdater.so
override SOURCES  += $(sort $(wildcard src/*.cc))
override MOCS     += $(sort $(shell grep -l 'Q_OBJECT' src/*.h 2>/dev/null || true))
override CFLAGS   += -Wall -Wextra -Werror -Wformat=2
override CXXFLAGS += -Wall -Wextra -Werror -Wno-missing-field-initializers

# C++ Core Guidelines diagnostics; -Wshadow, -Wold-style-cast, -Wuseless-cast and
# -Wzero-as-null-pointer-constant are excluded as Qt 5.2 headers and moc output trip them
override CXXFLAGS += -Wformat=2 -Wnon-virtual-dtor -Woverloaded-virtual -Wcast-qual
override CXXFLAGS += -Wredundant-decls -Wlogical-op -Wfloat-equal -Wundef
override PKGCONF  += Qt5Network

override KOBOROOT += config/nickelupdater.conf.tmpl:/usr/local/nickelupdater/nickelupdater.conf.tmpl

include NickelHook/NickelHook.mk
