###############################################################################
#
# 
#
###############################################################################
THIS_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
x86_64_cel_ivystone_INCLUDES := -I $(THIS_DIR)inc
x86_64_cel_ivystone_INTERNAL_INCLUDES := -I $(THIS_DIR)src
x86_64_cel_ivystone_DEPENDMODULE_ENTRIES := init:x86_64_cel_ivystone ucli:x86_64_cel_ivystone

