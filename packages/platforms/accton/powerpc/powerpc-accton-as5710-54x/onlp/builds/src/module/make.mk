###############################################################################
#
# 
#
###############################################################################
THIS_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
powerpc_accton_as5710_54x_INCLUDES := -I $(THIS_DIR)inc
powerpc_accton_as5710_54x_INTERNAL_INCLUDES := -I $(THIS_DIR)src
powerpc_accton_as5710_54x_DEPENDMODULE_ENTRIES := init:powerpc_accton_as5710_54x ucli:powerpc_accton_as5710_54x

