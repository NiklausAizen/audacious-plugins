PLUGIN = unix-io${PLUGIN_SUFFIX}

SRCS = unix-io.c

include ../../buildsys.mk
include ../../extra.mk

plugindir := ${plugindir}/${TRANSPORT_PLUGIN_DIR}

CPPFLAGS += -I../..
CFLAGS += ${PLUGIN_CFLAGS}
