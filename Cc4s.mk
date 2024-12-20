# IMPORTANT NOTE:
# ===============
# Do not edit this file, if you want to override a variable
# do it in your configuration file before you include this file
#
CC4S_EXTERN_BUILD = $(abspath ./extern/build/${CONFIG})
CC4S_EXTERN_SRC   = $(abspath ./extern/src/)
CC4S_INTERN_SRC   = $(abspath ./src/)

# Cyclops Tensor Framework ====================================================
CTF_COMMIT         ?= 826b3ad6de6cebdaaf72b2bcbee4d83fe8167f8c
CTF_BUILD_PATH     ?= $(CC4S_EXTERN_BUILD)/ctf/$(CTF_COMMIT)
CTF_SRC_PATH       ?= $(CC4S_EXTERN_SRC)/ctf/$(CTF_COMMIT)
CTF_LDFLAGS        ?= -L${CTF_BUILD_PATH}/lib -lctf
CTF_INCLUDE        ?= -I${CTF_BUILD_PATH}/include
CTF_GIT_REPOSITORY ?= https://gitlab.cc4s.org/cc4s/ctf.git

# yaml-cpp ====================================================================
YAML_COMMIT         ?= c9460110e072df84b7dee3eb651f2ec5df75fb18
YAML_BUILD_PATH     ?= $(CC4S_EXTERN_BUILD)/yaml-cpp/$(YAML_COMMIT)
YAML_SRC_PATH       ?= $(CC4S_EXTERN_SRC)/yaml-cpp/$(YAML_COMMIT)
YAML_LDFLAGS        ?= -L${YAML_BUILD_PATH} -lyaml-cpp
YAML_INCLUDE        ?= -I${YAML_BUILD_PATH}/include
YAML_GIT_REPOSITORY ?= https://gitlab.cc4s.org/cc4s/yaml-cpp.git

# BLAS ========================================================================
BLAS_INCLUDE ?= -I${BLAS_PATH}/include
BLAS_LDFLAGS ?= -L${BLAS_PATH}/lib -lopenblas

# ScaLAPACK ===================================================================
SCALAPACK_LDFLAGS ?= -L${SCALAPACK_PATH}/lib -lscalapack

# ATRIP =======================================================================
ATRIP_COMMIT         ?= bbbfb30
ATRIP_SRC_PATH       ?= $(CC4S_INTERN_SRC)/atrip/$(ATRIP_COMMIT)
ATRIP_GIT_REPOSITORY ?= git@gitlab.cc4s.org:gallo/atrip
ATRIP                ?= no
CXXFLAGS             += -DATRIP_COMMIT=$(ATRIP_COMMIT)
CXXFLAGS             += -DATRIP_NO_OUTPUT

# General settings ============================================================

# Default CFLAGS
INCLUDE_FLAGS += -Isrc/main

# destination path for installation
CC4S_INSTALL = ~/bin/cc4s/$(CONFIG)

# main target
CC4S_TARGET = Cc4s

# default CXXFLAGS
CXXFLAGS  +=                  \
-D_POSIX_C_SOURCE=200112L     \
-D__STDC_LIMIT_MACROS         \
-DFTN_UNDERSCORE=1            \
-DCC4S_VERSION=\"${VERSION}\" \
"-DCC4S_DATE=\"${DATE}\""     \
"-DCOMPILER_VERSION=\"${COMPILER_VERSION}\"" \
"-DCC4S_MPI_VERSION=\"${CC4S_MPI_VERSION}\""
