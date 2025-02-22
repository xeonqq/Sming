#######################
#
# This file is invoked as a separate MAKE instance to build each Component.
# (The idea was borrowed from Espressif's IDF, but is much simpler.)
#
# By default, a Component builds a single library.
# The working directory (CURDIR) is set to the Component build directory.
# All required submodules are pulled in and patched before this makefile is invoked.
# See `building.rst` for further details.
#
#######################

.PHONY: all
all:
	$(error Internal makefile) 

include $(SMING_HOME)/build.mk
include $(PROJECT_DIR)/$(OUT_BASE)/hwconfig.mk

# Makefile runs in the build directory
COMPONENT_BUILD_DIR := $(CURDIR)

CPPFLAGS		:= $(CPPFLAGS) $(GLOBAL_CFLAGS) -DCOMPONENT_PATH=\"$(COMPONENT_PATH)\"

#
CUSTOM_BUILD		:=
COMPONENT_TARGETS	:=
EXTRA_OBJ		:=
COMPONENT_CFLAGS	:=
COMPONENT_CPPFLAGS	:=
COMPONENT_CXXFLAGS	:=
COMPONENT_VARS		:=
COMPONENT_RELINK_VARS :=
COMPONENT_LIBNAME	:= $(COMPONENT_NAME)

ifeq (,$(wildcard $(COMPONENT_PATH)/Makefile-user.mk))
# Regular Component
EXTRA_INCDIR		:=
ifeq (App,$(COMPONENT_NAME))
COMPONENT_SRCDIRS	:= app
COMPONENT_INCDIRS	:= include
else
COMPONENT_SRCDIRS	:= . src
endif
COMPONENT_SRCFILES	:=
else
# Legacy project
MODULES			:= app
EXTRA_INCDIR		:= include
EXTRA_SRCFILES		:=
include $(COMPONENT_PATH)/Makefile-user.mk
COMPONENT_SRCDIRS	:= $(MODULES)
COMPONENT_SRCFILES	:= $(EXTRA_SRCFILES)
endif

ifeq (App,$(COMPONENT_NAME))
CPPFLAGS		+= $(APP_CFLAGS)
else ifneq ($(STRICT),1)
# Enforce strictest building for regular Components and treat as errors
CPPFLAGS		:= $(filter-out -Wno-sign-compare -Wno-strict-aliasing,$(CPPFLAGS)) -Werror
CXXFLAGS		:= $(filter-out -Wno-reorder,$(CXXFLAGS))
endif

INCDIR			= $(EXTRA_INCDIR) $(COMPONENTS_EXTRA_INCDIR)

# Build a Component target using MAKE
# The makefile should accept TARGET and BUILD_DIR variables
# $1 -> path to makefile, relative to Component path
# $2 -> parameters
define MakeTarget
$(Q) mkdir -p $(COMPONENT_BUILD_DIR)/$(basename $(@F)) $(@D)
+$(Q) $(MAKE) --no-print-directory -C $(dir $(COMPONENT_PATH)/$1) -f $(notdir $1) \
	TARGET=$@ BUILD_DIR=$(COMPONENT_BUILD_DIR)/$(basename $(@F)) V=$(V) $2
endef

# Define variables required for custom builds
COMPONENT_VARIANT := $(COMPONENT_LIBNAME)$(if $(COMPONENT_LIBHASH),-$(COMPONENT_LIBHASH))
COMPONENT_LIBPATH := $(COMPONENT_LIBDIR)/$(CLIB_PREFIX)$(COMPONENT_VARIANT).a

# component.mk is optional
-include $(COMPONENT_PATH)/component.mk

COMPONENT_VARS += $(COMPONENT_RELINK_VARS)

ifeq (App,$(COMPONENT_NAME))
COMPONENT_SRCDIRS += $(APPCODE)
EXTRA_INCDIR += $(COMPONENT_INCDIRS)
endif

EXTRA_INCDIR := $(call AbsoluteSourcePath,$(COMPONENT_PATH),$(EXTRA_INCDIR))

# COMPONENT_LIBNAME gets undefined if Component doesn't create a library
ifneq (,$(COMPONENT_LIBNAME))

COMPONENT_TARGETS += $(COMPONENT_LIBPATH)

# List of directories containing object files
BUILD_DIRS :=

ifeq (App,$(COMPONENT_NAME))
$(eval $(call WriteConfig,$(COMPONENT_BUILD_DIR)/build.cfg,$(CONFIG_VARS)))
else ifneq (,$(COMPONENT_VARS))
$(eval $(call WriteConfig,$(COMPONENT_BUILD_DIR)/build.cfg,$(COMPONENT_VARS)))
endif

# Custom build means we don't need any of the regular build logic, that's all be done in component.mk
ifeq (,$(CUSTOM_BUILD))

CFLAGS		+= $(COMPONENT_CFLAGS)
CPPFLAGS	+= $(COMPONENT_CPPFLAGS)
CXXFLAGS	+= $(COMPONENT_CXXFLAGS)

# GCC 10 escapes ':' in path names which breaks GNU make for Windows so filter them
ifeq ($(UNAME),Windows)
OUTPUT_DEPS := | sed "s/\\\\:/:/g" > $$@
else
OUTPUT_DEPS := -MF $$@
endif

# Additional flags to pass to clang
CLANG_FLAG_EXTRA ?=

# Flags which clang doesn't recognise
CLANG_FLAG_UNKNOWN := \
	-fstrict-volatile-bitfields

# $1 -> absolute source directory, no trailing path separator
# $2 -> relative output build directory, with trailing path separator
define GenerateCompileTargets
BUILD_DIRS += $2
ifneq (,$(filter $1/%.s,$(SOURCE_FILES)))
$2%.o: $1/%.s
	$(vecho) "AS $$<"
	$(Q) $(AS) $(addprefix -I,$(INCDIR)) $(CPPFLAGS) $(CFLAGS) -c $$< -o $$@
endif
ifneq (,$(filter $1/%.S,$(SOURCE_FILES)))
$2%.o: $1/%.S
	$(vecho) "AS $$<"
	$(Q) $(AS) $(addprefix -I,$(INCDIR)) $(CPPFLAGS) $(CFLAGS) -c $$< -o $$@
endif
ifneq (,$(filter $1/%.c,$(SOURCE_FILES)))
ifdef CLANG_TIDY
$2%.o: $1/%.c
	$(vecho) "TIDY $$<"
	$(Q) $(CLANG_TIDY) $$< -- $(addprefix -I,$(INCDIR)) $$(filter-out $(CLANG_FLAG_UNKNOWN),$(CPPFLAGS) $(CFLAGS)) $(CLANG_FLAG_EXTRA)
else
$2%.o: $1/%.c $2%.c.d
	$(vecho) "CC $$<"
	$(Q) $(CC) $(addprefix -I,$(INCDIR)) $(CPPFLAGS) $(CFLAGS) -c $$< -o $$@
$2%.c.d: $1/%.c
	$(Q) $(CC) $(addprefix -I,$(INCDIR)) $(CPPFLAGS) $(CFLAGS) -MM -MT $2$$*.o $$< $(OUTPUT_DEPS)
.PRECIOUS: $2%.c.d
endif
endif
ifneq (,$(filter $1/%.cpp,$(SOURCE_FILES)))
ifdef CLANG_TIDY
$2%.o: $1/%.cpp
	$(vecho) "TIDY $$<"
	$(Q) $(CLANG_TIDY) $$< -- $(addprefix -I,$(INCDIR)) $$(filter-out $(CLANG_FLAG_UNKNOWN),$(CPPFLAGS) $(CXXFLAGS)) $(CLANG_FLAG_EXTRA)
else
$2%.o: $1/%.cpp $2%.cpp.d
	$(vecho) "C+ $$<"
	$(Q) $(CXX) $(addprefix -I,$(INCDIR)) $(CPPFLAGS) $(CXXFLAGS) -c $$< -o $$@
$2%.cpp.d: $1/%.cpp
	$(Q) $(CXX) $(addprefix -I,$(INCDIR)) $(CPPFLAGS) $(CXXFLAGS) -MM -MT $2$$*.o $$< $(OUTPUT_DEPS)
.PRECIOUS: $2%.cpp.d
endif
endif
endef

# Resolve a source path to the corresponding build output object file
# $1 -> source root directory
# $2 -> file path(s)
define ResolveObjPath
$(foreach f,$2,$(patsubst /%,%,$(patsubst $(SMING_HOME)/%,%,$(patsubst $1/%,%,$f))))
endef

# All source files, absolute paths
SOURCE_FILES := $(call AbsoluteSourcePath,$(COMPONENT_PATH),$(COMPONENT_SRCFILES)) \
	$(foreach d,$(call AbsoluteSourcePath,$(COMPONENT_PATH),$(COMPONENT_SRCDIRS)),$(wildcard $d/*.s $d/*.S $d/*.c $d/*.cpp))
# All unique source directories, absolute paths
SOURCE_DIRS := $(sort $(patsubst %/,%,$(dir $(SOURCE_FILES))))
# Output object files
OBJ := $(call ResolveObjPath,$(COMPONENT_PATH),$(SOURCE_FILES))
OBJ := $(OBJ:.s=.o)
OBJ := $(OBJ:.S=.o)
OBJ := $(OBJ:.c=.o)
OBJ := $(OBJ:.cpp=.o)
# Create implicit rules for each source directory, and update BUILD_DIRS
$(foreach d,$(SOURCE_DIRS),$(eval $(call GenerateCompileTargets,$d,$(call ResolveObjPath,$(COMPONENT_PATH),$d/))))
BUILD_DIRS := $(sort $(BUILD_DIRS:/=))
# Include any generated dependency files (these won't exist on first build)
ABS_BUILD_DIRS := $(sort $(COMPONENT_BUILD_DIR) $(BUILD_DIRS))
include $(wildcard $(ABS_BUILD_DIRS:=/*.c.d))
include $(wildcard $(ABS_BUILD_DIRS:=/*.cpp.d))

# Provide a target unless Component is custom built, in which case the component.mk will have defined this already
$(COMPONENT_LIBPATH): build.ar
ifndef CLANG_TIDY
	$(vecho) "AR $@"
	$(Q) test ! -f $@ || rm $@
	$(Q) $(AR) -M < build.ar
	$(Q) mv $(notdir $(COMPONENT_LIBPATH)) $(COMPONENT_LIBPATH)
endif

define addmod
	@echo ADDMOD $2 >> $1

endef

build.ar: $(OBJ) $(EXTRA_OBJ)
	@echo CREATE $(notdir $(COMPONENT_LIBPATH)) > $@
	$(foreach o,$(OBJ) $(EXTRA_OBJ),$(call addmod,$@,$o))
	@echo SAVE >> $@
	@echo END >> $@


endif # ifeq (,$(CUSTOM_BUILD))
endif # ifneq (,$(COMPONENT_LIBNAME))

TARGET_DIRS := $(sort $(patsubst %/,%,$(dir $(COMPONENT_TARGETS))))

# Targets
.PHONY: build
build: checkdirs $(COMPONENT_TARGETS)

checkdirs: | $(BUILD_DIRS) $(TARGET_DIRS)

$(BUILD_DIRS) $(TARGET_DIRS):
	$(Q) mkdir -p $@
