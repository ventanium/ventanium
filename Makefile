##########
# NAMING #
##########
LIBNAME = ventanium
VERSION = 0.1

LIB_STATIC_EXT  = a
LIB_DYNAMIC_EXT = so

STATIC_LIBNAME  = lib$(LIBNAME).$(LIB_STATIC_EXT)
DYNAMIC_LIBNAME = lib$(LIBNAME).$(LIB_DYNAMIC_EXT)
DYNAMIC_LIBNAME_VERSIONED = $(DYNAMIC_LIBNAME).$(VERSION)

STATIC_LIB =  $(LIB_DIR)/$(STATIC_LIBNAME)
DYNAMIC_LIB = $(LIB_DIR)/$(DYNAMIC_LIBNAME_VERSIONED)

###############
# DIRECTORIES #
###############
SRC_DIR = src
LIB_DIR = lib
OBJ_DIR = obj/release
BIN_DIR = bin
DIST_DIR = dist
WINDOWS_DIR = windows

TEST_NAME = vtm-test
TEST_SRC_DIR = test
TEST_OBJ_DIR = $(OBJ_DIR)-test

EXAMPLES_SRC_DIR = examples
EXAMPLES_OBJ_DIR = $(OBJ_DIR)-examples

prefix ?= /usr/local
INSTALL_LIB_DIR = $(DESTDIR)$(prefix)/lib
INSTALL_INC_DIR = $(DESTDIR)$(prefix)/include

#########
# TOOLS #
#########
CC ?= cc
AR = ar rcs
RM = rm -f
INSTALL = install
WINDRES = windres

#################
# COMPILERFLAGS #
#################
CFLAGS = -std=c99 -Wall -Wsign-compare -Wpedantic -pedantic-errors \
         -fPIC -I$(SRC_DIR)
LDLIBS = -lm
STFLAGS =
STLIBS =

DYNAMIC_LIB_FLAGS = -shared -Wl,-soname,$(DYNAMIC_LIBNAME_VERSIONED)

################
# OS DETECTION #
################
ifndef SYS
	ifeq ($(OS),Windows_NT)
		SYS = Windows
	else
		SYS = $(shell uname)
		ifneq (,$(findstring BSD,$(SYS)))
			SYS = BSD
		endif
	endif
$(info Detected System: $(SYS))
endif

############
# DEFAULTS #
############
ifndef MYSQL
	MYSQL = 0
endif

###########
# OPTIONS #
###########
ifeq ($(DEBUG), 1)
    CFLAGS += -O0 -g3
    OBJ_DIR = obj/debug
endif

ifeq ($(NO_OOM_ABORT), 1)
	CFLAGS += -DVTM_OPT_NO_OOM_ABORT
endif

#######
# SYS #
#######
DIR_SYS = vtm/sys

## LINUX ##
ifeq ($(SYS), Linux)
	UNIX = 1
	SRC_SYS_FLT = %/socket_listener_kqueue.c
	CFLAGS += -DVTM_SYS_LINUX -D_XOPEN_SOURCE=700

## BSD ##
else ifeq ($(SYS), BSD)
	UNIX = 1
	SRC_SYS_FLT = %/socket_listener_epoll.c
	CFLAGS += -DVTM_SYS_BSD

## DARWIN ##
else ifeq ($(SYS), Darwin)
	UNIX = 1
	SRC_SYS_FLT = %/socket_listener_epoll.c
	CFLAGS += -DVTM_SYS_DARWIN

	LIB_DYNAMIC_EXT = dylib
	DYNAMIC_LIBNAME_VERSIONED = lib$(LIBNAME).$(VERSION).$(LIB_DYNAMIC_EXT)
	DYNAMIC_LIB_FLAGS = -dynamiclib -Wl,-install_name,$(INSTALL_LIB_DIR)/$(DYNAMIC_LIBNAME_VERSIONED)

## WINDOWS ##
else ifeq ($(SYS), Windows)
	SRC_SYS  = $(shell find $(SRC_DIR)/$(DIR_SYS)/windows -name "*.c")
	SRC_SYS += $(shell find $(SRC_DIR)/$(DIR_SYS)/base/net -name "*.c")
	CFLAGS += -DVTM_SYS_WINDOWS -D_WIN32_WINNT=0x0600
	LDLIBS += -lWs2_32 -lwsock32

	LIB_STATIC_EXT  = lib
	LIB_DYNAMIC_EXT = dll

	DYNAMIC_LIBNAME = $(LIBNAME).$(LIB_DYNAMIC_EXT)
	DYNAMIC_LIBNAME_VERSIONED = $(DYNAMIC_LIBNAME)
	DYNAMIC_LIB_FLAGS = -shared -Wl,--out-implib,$(LIB_DIR)/$(LIBNAME).lib

	DYN_OBS += $(OBJ_DIR)/$(LIBNAME).res

## UNSUPPORTED ##
else
$(error SYS=$(SYS) not supported)
endif

## UNIX ##
ifeq ($(UNIX), 1)
	SRC_SYS  = $(shell find $(SRC_DIR)/$(DIR_SYS)/unix -name "*.c")
	SRC_SYS += $(shell find $(SRC_DIR)/$(DIR_SYS)/base/net -name "*.c")
	CFLAGS += -DVTM_SYS_UNIX -DVTM_HAVE_POSIX
endif

########
# LIBS #
########

## OPENSSL ##
ifeq ($(NO_OPENSSL),)
	CFLAGS += -DVTM_LIB_OPENSSL -DVTM_MODULE_CRYPTO
	LDLIBS += -lssl -lcrypto
	STLIBS += -ldl
	SRC_SYS_FLT += %/socket_tls_unsupported.c
else
	SRC_SYS_FLT += %/socket_tls_openssl.c
endif

## PTHREAD ##
ifeq ($(UNIX), 1)
	LDLIBS += -lpthread
endif

###########
# MODULES #
###########

## SYS ##
SRC_FLT = $(shell find $(SRC_DIR)/$(DIR_SYS) -name "*.c")

## MYSQL ##
ifeq ($(MARIADB), 1)
	MYSQL = 1
	MYSQL_CONFIG = mariadb_config
endif

ifeq ($(MYSQL), 1)
	MYSQL_CONFIG ?= mysql_config
	CFLAGS += -DVTM_MODULE_MYSQL
	CFLAGS += $(shell $(MYSQL_CONFIG) --include)
	LDLIBS += $(shell $(MYSQL_CONFIG) --libs)
	TEST_SQL = 1
else
	DIR_MYSQL = vtm/sql/mysql
	SRC_FLT += $(shell find $(SRC_DIR)/$(DIR_MYSQL) -name "*.c")
	HEADERS_FLT += $(shell find $(SRC_DIR)/$(DIR_MYSQL) -name "*.h")

	SRC_TEST_FLT += $(shell find $(TEST_SRC_DIR)/$(DIR_MYSQL) -name "*.c")
	SRC_EXAMPLES_FLT += $(shell find $(EXAMPLES_SRC_DIR) -name "mysql_*.c")
endif

## SQLITE ##
ifeq ($(SQLITE), 1)
	CFLAGS += -DVTM_MODULE_SQLITE
	LDLIBS += -lsqlite3
	TEST_SQL = 1
else
	DIR_SQLITE = vtm/sql/sqlite
	SRC_FLT += $(shell find $(SRC_DIR)/$(DIR_SQLITE) -name "*.c")
	HEADERS_FLT += $(shell find $(SRC_DIR)/$(DIR_SQLITE) -name "*.h")

	SRC_TEST_FLT += $(shell find $(TEST_SRC_DIR)/$(DIR_SQLITE) -name "*.c")
	SRC_EXAMPLES_FLT += $(shell find $(EXAMPLES_SRC_DIR) -name "sqlite_*.c")
endif

## SQL ##
ifneq ($(TEST_SQL), 1)
	SRC_TEST_FLT += $(TEST_SRC_DIR)/vtm/sql/test_sql.c
endif

##########
# STATIC #
##########
ifeq ($(STATIC), 1)
	STFLAGS = -static
	LDLIBS += $(STLIBS)
endif

#########
# BUILD #
#########
SRCS_ALL = $(shell find $(SRC_DIR) -name "*.c")
SRCS  = $(filter-out $(SRC_FLT), $(SRCS_ALL))
SRCS += $(filter-out $(SRC_SYS_FLT), $(SRC_SYS))
OBJS = $(patsubst %.c,$(OBJ_DIR)/%.o,$(SRCS))

.PHONY: all clean distclean

all: $(DYNAMIC_LIB) $(STATIC_LIB)

$(DYNAMIC_LIB): $(LIB_DIR) $(OBJS) $(DYN_OBS)
	$(CC) $(CFLAGS) $(DYNAMIC_LIB_FLAGS) $(LDFLAGS) -o $(DYNAMIC_LIB) $(OBJS) $(DYN_OBS) $(LDLIBS)

$(STATIC_LIB): $(LIB_DIR) $(OBJS)
	$(AR) $(STATIC_LIB) $(OBJS)

$(OBJ_DIR)/$(LIBNAME).res: $(WINDOWS_DIR)/$(LIBNAME).rc
	$(WINDRES) $< -O coff -o $@

$(OBJ_DIR)/%.o: %.c
	@test -d $(@D) || mkdir -pm 775 $(@D)
	$(CC) $(CFLAGS) $(LDFLAGS) -c $< -o $@

$(LIB_DIR):
	@test -d $(LIB_DIR) || mkdir -pm 775 $(LIB_DIR)

$(BIN_DIR):
	@test -d $(BIN_DIR) || mkdir -pm 775 $(BIN_DIR)

depend: .depend

.depend: $(SRCS)
	$(RM) ./.depend
	$(CC) $(CFLAGS) -MM -MG $^>>./.depend;

clean: cleantest cleanexamples
	$(RM) $(OBJS)
	$(RM) $(LIB_DIR)/*

distclean: clean
	$(RM) $(DIST_DIR)/*
	$(RM) .depend

-include .depend

########
# INFO #
########
.PHONY: ldinfo

ldinfo:
	@echo "Use following libs to build your program:"
	@echo -l$(LIBNAME) $(LDLIBS)

###########
# RELEASE #
###########
.PHONY: dist

dist:
	@test -d $(DIST_DIR) || mkdir -pm 775 $(DIST_DIR)
	git archive --format=tar --prefix=$(LIBNAME)-$(VERSION)/ HEAD | gzip -9 > $(DIST_DIR)/$(LIBNAME)-$(VERSION).tar.gz

###########
# INSTALL #
###########
HEADERS_ALL  = $(shell find $(SRC_DIR) -name "*.h")
HEADERS_FLT += $(shell find $(SRC_DIR) -name "*_intl.h")
HEADERS_MIN  = $(filter-out $(HEADERS_FLT), $(HEADERS_ALL))
HEADERS_RAW  = $(patsubst src/%,%,$(HEADERS_MIN))
HEADERS_INC  = $(addprefix $(INSTALL_INC_DIR)/,$(HEADERS_RAW))

HEADERS_ROOTDIRS     = $(shell find $(SRC_DIR) -mindepth 1 -maxdepth 1 -type d)
HEADERS_ROOTDIRS_RAW = $(patsubst src/%,%,$(HEADERS_ROOTDIRS))

HEADERS_INC_ROOTDIRS = $(addprefix $(INSTALL_INC_DIR)/,$(HEADERS_ROOTDIRS_RAW))
HEADERS_INC_DIRS = $(sort $(dir $(HEADERS_INC)))

.PHONY: install uninstall

install: all $(HEADERS_INC_DIRS) $(HEADERS_INC)
	$(INSTALL) -d $(INSTALL_LIB_DIR)
	$(INSTALL) $(STATIC_LIB) $(INSTALL_LIB_DIR)
	$(INSTALL) $(DYNAMIC_LIB) $(INSTALL_LIB_DIR)
	cd $(INSTALL_LIB_DIR) && ln -sf $(DYNAMIC_LIBNAME_VERSIONED) $(DYNAMIC_LIBNAME)

uninstall:
	$(RM) $(INSTALL_LIB_DIR)/$(STATIC_LIBNAME)
	$(RM) $(INSTALL_LIB_DIR)/$(DYNAMIC_LIBNAME)
	$(RM) $(INSTALL_LIB_DIR)/$(DYNAMIC_LIBNAME_VERSIONED)
	$(RM) $(HEADERS_INC)
	$(RM) -r $(HEADERS_INC_ROOTDIRS)

$(HEADERS_INC_DIRS):
	$(INSTALL) -d $@

$(INSTALL_INC_DIR)/%.h: $(SRC_DIR)/%.h
	$(INSTALL) $^ $@

########
# TEST #
########
CFLAGS_TEST = $(CFLAGS) -I$(TEST_SRC_DIR)

SRCS_TEST_ALL = $(shell find $(TEST_SRC_DIR) -name "*.c")
SRCS_TEST = $(filter-out $(SRC_TEST_FLT),$(SRCS_TEST_ALL))
OBJS_TEST = $(patsubst %.c,$(TEST_OBJ_DIR)/%.o,$(SRCS_TEST))

.PHONY: runtest compiletest cleantest

runtest: compiletest
	$(BIN_DIR)/$(TEST_NAME)

compiletest: all cleantest $(OBJS_TEST) $(BIN_DIR)
	$(CC) $(CFLAGS_TEST) $(LDFLAGS) $(STFLAGS) $(OBJS_TEST) -o $(BIN_DIR)/$(TEST_NAME) $(STATIC_LIB) $(LDLIBS)

cleantest:
	$(RM) $(BIN_DIR)/$(TEST_NAME)
	$(RM) $(OBJS_TEST)

$(TEST_OBJ_DIR)/%.o: %.c
	@test -d $(@D) || mkdir -pm 775 $(@D)
	$(CC) $(CFLAGS_TEST) $(LDFLAGS) -c $< -o $@

############
# EXAMPLES #
############
SRCS_EXAMPLES_ALL = $(shell find $(EXAMPLES_SRC_DIR) -name "*.c")
SRCS_EXAMPLES = $(filter-out $(SRC_EXAMPLES_FLT),$(SRCS_EXAMPLES_ALL))
OBJS_EXAMPLES = $(patsubst %.c,%.o,$(addprefix $(EXAMPLES_OBJ_DIR)/,$(notdir $(SRCS_EXAMPLES))))
BINS_EXAMPLES = $(addprefix $(BIN_DIR)/,$(notdir $(basename $(SRCS_EXAMPLES))))

.PHONY: examples cleanexamples

examples: all $(OBJS_EXAMPLES) $(BIN_DIR) $(BINS_EXAMPLES)

cleanexamples:
	$(RM) $(OBJS_EXAMPLES)
	$(RM) $(BINS_EXAMPLES)

$(EXAMPLES_OBJ_DIR)/%.o: $(EXAMPLES_SRC_DIR)/%.c
	@test -d $(@D) || mkdir -pm 775 $(@D)
	$(CC) $(CFLAGS) $(LDFLAGS) -c $< -o $@

$(BIN_DIR)/%: $(EXAMPLES_OBJ_DIR)/%.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(STFLAGS) $< -o $@ $(STATIC_LIB) $(LDLIBS)
