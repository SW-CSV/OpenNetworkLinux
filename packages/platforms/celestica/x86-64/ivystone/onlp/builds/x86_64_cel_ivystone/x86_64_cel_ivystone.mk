
###############################################################################
#
# Inclusive Makefile for the x86_64_cel_ivystone module.
#
# Autogenerated 2020-10-09 06:08:27.514045
#
###############################################################################
x86_64_cel_ivystone_BASEDIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
include $(x86_64_cel_ivystone_BASEDIR)module/make.mk
include $(x86_64_cel_ivystone_BASEDIR)module/auto/make.mk
include $(x86_64_cel_ivystone_BASEDIR)module/src/make.mk

