###############################################################################
#
# 
#
###############################################################################
THIS_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
x86_64_cel_ericsson_nru03_INCLUDES := -I $(THIS_DIR)inc
x86_64_cel_ericsson_nru03_INTERNAL_INCLUDES := -I $(THIS_DIR)src
x86_64_cel_ericsson_nru03_DEPENDMODULE_ENTRIES := init:x86_64_cel_ericsson_nru03 ucli:x86_64_cel_ericsson_nru03

