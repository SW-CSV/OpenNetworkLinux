###############################################################################
#
# 
#
###############################################################################
THIS_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
x86_64_cel_dx032_INCLUDES := -I $(THIS_DIR)inc
x86_64_cel_dx032_INTERNAL_INCLUDES := -I $(THIS_DIR)src
x86_64_cel_dx032_DEPENDMODULE_ENTRIES := init:x86_64_cel_dx032 ucli:x86_64_cel_dx032

