###############################################################################
#
#
#
###############################################################################

LIBRARY := x86_64_cel_silverstone2
$(LIBRARY)_SUBDIR := $(dir $(lastword $(MAKEFILE_LIST)))
include $(BUILDER)/lib.mk