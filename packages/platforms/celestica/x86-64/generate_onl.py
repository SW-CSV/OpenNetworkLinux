#!/usr/bin/env python

# Copyright (c) 2018 Larry Ming
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS" by "LARR-Y" "MIN-G", WITHOUT WARRANTY OF ANY KIND, 
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

from __future__ import print_function
from os import path
import xml.etree.ElementTree as XML
import getopt
import json
import os
import re
import sys
import time

platform = ''

def gen_modules_files():
        print("\t Project Name:%s" % platform)
        os.system("mkdir -p ./x86-64-cel-%s/modules" % platform.lower())
        fp = open('./x86-64-cel-%s/modules/PKG.yml' % platform.lower(),'a')
        fp.write("!include $ONL_TEMPLATES/no-platform-modules.yml ARCH=amd64 VENDOR=celestica BASENAME=x86-64-cel-%s\n" % platform.lower())
        fp.close()
        fp = open('./x86-64-cel-%s/modules/Makefile' % platform.lower(),'a')
        fp.write("include $(ONL)/make/pkg.mk\n")
        fp.close()		

def gen_onlp_files():
        print("\t Project Name:%s" % platform)
        os.system("mkdir -p ./x86-64-cel-%s/onlp" % platform.lower())
        fp = open('./x86-64-cel-%s/onlp/PKG.yml' % platform.lower(),'a')
        fp.write("variables:\n")
        fp.write("  platform: x86-64-cel-%s-r0\n" % platform.lower())
        fp.write("  install: /lib/platform-config/${platform}/onl\n")
        fp.write("\n")
        fp.write("common:\n")
        fp.write("  version: 1.0.0\n")
        fp.write("  arch: amd64\n")
        fp.write("  copyright: Copyright 2013, 2014, 2015 Big Switch Networks\n")
        fp.write("  maintainer: support@bigswitch.com\n")
        fp.write("  comment: dummy package for ONLP on Celestica %s\n" % platform)
        fp.write("packages:\n")
        fp.write("  - name: onlp-${platform}\n")
        fp.write("    summary: ONLP Package for the ${platform} platform.\n")
        fp.write("\n")
        fp.write("    changelog: initial version\n")
        fp.write("\n")
        fp.close()
        fp = open('./x86-64-cel-%s/onlp/Makefile' % platform.lower(),'a')
        fp.write("include $(ONL)/make/pkg.mk\n")
        fp.close()		

def gen_plaform_config_files():
        print("\t Project Name:%s" % platform)
        os.system("mkdir -p ./x86-64-cel-%s/platform-config/r0" % platform.lower())
        fp = open('./x86-64-cel-%s/platform-config/r0/PKG.yml' % platform.lower(),'a')
        fp.write("!include $ONL_TEMPLATES/platform-config-platform.yml ARCH=amd64 VENDOR=celestica BASENAME=x86-64-cel-%s REVISION=r0\n" % platform.lower())
        fp.close()
        fp = open('./x86-64-cel-%s/platform-config/Makefile' % platform.lower(),'a')
        fp.write("include $(ONL)/make/pkg.mk\n")
        fp.close()
        fp = open('./x86-64-cel-%s/platform-config/r0/Makefile' % platform.lower(),'a')
        fp.write("include $(ONL)/make/pkg.mk\n")
        fp.close()		

def gen_plaform_config_src_files():
        print("\t Project Name:%s" % platform)
        #os.system("mkdir -p ./x86-64-cel-%s/platform-config/r0/src/python/" % platform.lower())
        os.system("mkdir -p ./x86-64-cel-%s/platform-config/r0/src/python/x86_64_cel_%s_r0"  % platform.lower().replace('-','_'))
        fp = open('./x86-64-cel-%s/platform-config/r0/PKG.yml' % platform.lower(),'a')
        fp.write("!include $ONL_TEMPLATES/platform-config-platform.yml ARCH=amd64 VENDOR=celestica BASENAME=x86-64-cel-%s REVISION=r0\n" % platform.lower())
        fp.close()
        fp = open('./x86-64-cel-%s/platform-config/Makefile' % platform.lower(),'a')
        fp.write("include $(ONL)/make/pkg.mk\n")
        fp.close()
        fp = open('./x86-64-cel-%s/platform-config/r0/src/Makefile' % platform.lower(),'a')
        fp.write("include $(ONL)/make/pkg.mk\n")
        fp.close()
		
def gen_lib_x86_64_platform_yml():
        print("\t Project Name:%s" % platform)
        os.system("mkdir -p ./x86-64-cel-%s/platform-config/r0/src/lib/" % platform.lower())
        fp = open('./x86-64-cel-%s/platform-config/r0/src/lib/x86-64-cel-%s-r0.yml' % (platform.lower(), platform.lower()),'a')
        fp.write("---\n")
        fp.write("\n")
        fp.write("######################################################################\n")
        fp.write("#\n")
        fp.write("# platform-config for Celestica %s\n" % platform)
        fp.write("#\n")
        fp.write("#\n")
        fp.write("######################################################################\n")
        fp.write("\n")
        fp.write("x86-64-cel-%s-r0:\n"% platform.lower())
        fp.write("\n")
        fp.write("  grub:\n")
        fp.write("\n")
        fp.write("    serial: >-\n")
        fp.write("      --port=0x3f8\n")
        fp.write("      --speed=115200\n")
        fp.write("      --word=8\n")
        fp.write("      --parity=0\n")
        fp.write("      --stop=1\n")
        fp.write("\n")
        fp.write("    kernel:\n")
        fp.write("      <<: *kernel-4-14\n")
        fp.write("\n")
        fp.write("    args: >-\n")
        fp.write("      nopat\n")
        fp.write("      console=ttyS0,115200n8\n")
        fp.write("\n")
        fp.write("  ##network\n")
        fp.write("  ##  interfaces:\n")
        fp.write("  ##    ma1:\n")
        fp.write("  ##      name: ~\n")
        fp.write("  ##      syspath: pci0000:00/0000:00:14.0\n")
        fp.write("\n")
        fp.close()

def gen_init_py():
        print("\t Project Name:%s" % platform)
        os.system("mkdir -p ./x86-64-cel-%s/platform-config/r0/src/python/x86_64_cel_%s_r0" %(platform.lower(), platform.lower().replace('-','_')))
        fp = open('./x86-64-cel-%s/platform-config/r0/src/python/x86_64_cel_%s_r0/__init__.py' %(platform.lower(), platform.lower().replace('-','_')),'a')
        fp.write("from onl.platform.base import *\n")
        fp.write("from onl.platform.celestica import *\n")
        fp.write("\n")
        fp.write("class OnlPlatform_x86_64_cel_%s_r0(OnlPlatformCelestica,\n" % platform.lower().replace('-','_'))
        fp.write("                                            OnlPlatformPortConfig_48x10_6x40):\n")
        fp.write("    PLATFORM='x86-64-cel-%s-r0'\n" % platform.lower())
        fp.write("    MODEL=\"%s\"\n" % platform)
        fp.write("    SYS_OBJECT_ID=\".2060.1\"\n")
        fp.close()
		
def gen_main_makefile():
        print("\t Project Name:%s" % platform)
        fp = open('./x86-64-cel-%s/Makefile' % platform.lower(),'a')
        fp.write("include $(ONL)/make/pkg.mk\n")
        fp.close()
		
def help():
    print('generate.py [options]\n')
    print('options:')
    print('        -h                            show this help message')
    print('        -p <platform>                 identifier to be platform')
    print('        -v <kernel version>           kernel version')

def main():
    global platform


    if 1 == len(sys.argv):
        help()
        sys.exit(1)

    # TODO Add options for outputting header or source files
    options, arguments = getopt.getopt(sys.argv[1:], 'hp:s:v:fg')
    for opt, arg in options:
        if opt in ('-h'):
            help()
            sys.exit(0)
        elif opt in ('-p'):
            platform = arg
            print('platform:', arg)
        elif opt in ('-s'):
            service = arg
            print('service:', arg)
        elif opt in ('-v'):
            name_end = str(arg).find(':')
            variable = Variable(arg[0:name_end], arg[name_end + 1:].split(';'))
            variables.append(variable)
        elif opt in ('-f'):
            functions_only = True
        elif opt in ('-g'):
            stub_guards_on = True
    os.system("rm -rf ./x86-64-cel-%s/" % platform.lower())
    os.system("mkdir -p ./x86-64-cel-%s/" % platform.lower())
    gen_main_makefile()
    gen_modules_files()
    gen_onlp_files()
    gen_plaform_config_files()
    gen_lib_x86_64_platform_yml()
    gen_init_py()

if __name__ == '__main__':
    main()
