
###############################################################################
#
# Inclusive Makefile for the x86_64_cel_silverstone module.
#
# Autogenerated 2017-12-10 05:13:18.533898
#
###############################################################################
x86_64_cel_silverstone_BASEDIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
include $(x86_64_cel_silverstone_BASEDIR)module/make.mk
include $(x86_64_cel_silverstone_BASEDIR)module/src/make.mk
include $(x86_64_cel_silverstone_BASEDIR)module/auto/make.mk

