###############################################################################
#
# 
#
###############################################################################
THIS_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
x86_64_cel_ericsson_nru_s0301_INCLUDES := -I $(THIS_DIR)inc
x86_64_cel_ericsson_nru_s0301_INTERNAL_INCLUDES := -I $(THIS_DIR)src
x86_64_cel_ericsson_nru_s0301_DEPENDMODULE_ENTRIES := init:x86_64_cel_ericsson_nru_s0301 ucli:x86_64_cel_ericsson_nru_s0301

