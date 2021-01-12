###############################################################################
#
# 
#
###############################################################################
THIS_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
x86_64_cel_cloverstone_dp_INCLUDES := -I $(THIS_DIR)inc
x86_64_cel_cloverstone_dp_INTERNAL_INCLUDES := -I $(THIS_DIR)src
x86_64_cel_cloverstone_dp_DEPENDMODULE_ENTRIES := init:x86_64_cel_cloverstone_dp ucli:x86_64_cel_cloverstone_dp

