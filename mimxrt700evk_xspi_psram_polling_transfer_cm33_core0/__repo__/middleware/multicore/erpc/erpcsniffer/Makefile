#-------------------------------------------------------------------------------
# Copyright (C) 2014-2016 Freescale Semiconductor, Inc.
# Copyright 2016 NXP
# All rights reserved.
#
# THIS SOFTWARE IS PROVIDED BY FREESCALE "AS IS" AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
# SHALL FREESCALE BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
# OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
# IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
# OF SUCH DAMAGE.
#-------------------------------------------------------------------------------

this_makefile := $(firstword $(MAKEFILE_LIST))
ERPC_ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))../)
include $(ERPC_ROOT)/mk/erpc_common.mk
TARGET_OUTPUT_ROOT = $(OUTPUT_ROOT)/$(DEBUG_OR_RELEASE)/$(os_name)/$(APP_NAME)
#-----------------------------------------------
# setup variables
# ----------------------------------------------

APP_NAME = erpcsniffer
LIB_NAME = erpc

PYTH_SCRIPT = erpcgen/bin/txt_to_c.py
ERPC_LIB_OUTPUT_DIR = $(OUTPUT_ROOT)/$(DEBUG_OR_RELEASE)/$(os_name)/$(LIB_NAME)/lib

#-----------------------------------------------
# Include path. Add the include paths like this:
# INCLUDES += ./include/
#-----------------------------------------------
INCLUDES += $(ERPC_ROOT) \
			$(ERPC_ROOT)/erpcgen/src \
			$(ERPC_ROOT)/erpcgen/src/cpptemplate \
			$(ERPC_ROOT)/erpcgen/src/types \
			$(ERPC_ROOT)/erpc_c/infra \
			$(ERPC_ROOT)/erpc_c/port \
			$(ERPC_ROOT)/erpc_c/setup \
			$(ERPC_ROOT)/erpc_c/transports \
			$(ERPC_ROOT)/erpcsniffer/src \
			$(OBJS_ROOT)

ifeq "$(is_mingw)" "1"
INCLUDES += $(VISUAL_STUDIO_ROOT)
endif

SOURCES += 	$(OBJS_ROOT)/erpcgen_parser.tab.cpp \
			$(OBJS_ROOT)/erpcgen_lexer.cpp \
			$(ERPC_ROOT)/erpcgen/src/format_string.cpp \
			$(ERPC_ROOT)/erpcgen/src/HexValues.cpp \
			$(ERPC_ROOT)/erpcgen/src/Logging.cpp \
			$(ERPC_ROOT)/erpcgen/src/options.cpp \
			$(ERPC_ROOT)/erpcgen/src/SearchPath.cpp \
			$(ERPC_ROOT)/erpcgen/src/AstNode.cpp \
			$(ERPC_ROOT)/erpcgen/src/AstWalker.cpp \
			$(ERPC_ROOT)/erpcgen/src/UniqueIdChecker.cpp \
			$(ERPC_ROOT)/erpcgen/src/ErpcLexer.cpp \
			$(ERPC_ROOT)/erpcgen/src/InterfaceDefinition.cpp \
			$(ERPC_ROOT)/erpcgen/src/SymbolScanner.cpp \
			$(ERPC_ROOT)/erpcgen/src/Token.cpp \
			$(ERPC_ROOT)/erpcgen/src/ParseErrors.cpp \
			$(ERPC_ROOT)/erpcgen/src/Utils.cpp \
			$(ERPC_ROOT)/erpcgen/src/types/Type.cpp \
			$(ERPC_ROOT)/erpcsniffer/src/erpcsniffer.cpp \
			$(ERPC_ROOT)/erpcsniffer/src/Sniffer.cpp

# Prevent make from deleting these temp files.
.SECONDARY: $(OBJS_ROOT)/erpcgen_parser.tab.cpp \
            $(OBJS_ROOT)/erpcgen_lexer.cpp

OBJECT_DEP := $(OBJS_ROOT)/erpcgen_lexer.cpp

include $(ERPC_ROOT)/mk/targets.mk

# Release should be alwas static. Customers don't need install things.
ifeq "$(build)" "release"
    # Except Darwin.
    ifeq "$(is_darwin)" ""
        LIBRARIES += -static
    endif
endif

LIBRARIES := -L$(ERPC_LIB_OUTPUT_DIR) -lerpc $(LIBRARIES)

#$(info $$var is [${LIBRARIES}])

# Run flex
$(OBJS_ROOT)/erpcgen_lexer.cpp: $(ERPC_ROOT)/erpcgen/src/erpcgen_lexer.l $(OBJS_ROOT)/erpcgen_parser.tab.hpp | $(OBJECTS_DIRS)
	@$(call printmessage,orange,Generating, $(subst $(ERPC_ROOT)/,,$<))
	$(at)$(FLEX) $(LLFLAGS) -o $(OBJS_ROOT)/erpcgen_lexer.cpp $<

# Run bison
$(OBJS_ROOT)/erpcgen_parser.tab.cpp: $(OBJS_ROOT)/erpcgen_parser.tab.hpp

$(OBJS_ROOT)/erpcgen_parser.tab.hpp: $(ERPC_ROOT)/erpcgen/src/erpcgen_parser.y | $(OBJECTS_DIRS)
	@$(call printmessage,orange,Generating, $(subst $(ERPC_ROOT)/,,$<))
	$(at)$(BISON) $(YYFLAGS) -o $(OBJS_ROOT)/erpcgen_parser.tab.cpp $<

.PHONY: install
install: $(MAKE_TARGET)
	@$(call printmessage,c,Installing, erpcsniffer in $(BIN_INSTALL_DIR))
	$(at)$(mkdirc) -p $(BIN_INSTALL_DIR)
	$(at)install $(MAKE_TARGET) $(BIN_INSTALL_DIR)

clean::
	$(at)$(rmc) $(OBJS_ROOT)/*.cpp $(OBJS_ROOT)/*.hpp $(OBJS_ROOT)/*.c
