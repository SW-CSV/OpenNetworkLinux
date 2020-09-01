#! /usr/bin/python

import os, sys, subprocess, string

def new_device(driver, addr, bus, devdir):
        if not os.path.exists(os.path.join(bus, devdir)):
                try:
                        with open("%s/new_device" % bus, "w") as f:
                                f.write("%s 0x%x\n" % (driver, addr))
                except Exception, e:
                        print "Unexpected error initialize device %s:0x%x:%s: %s" % (driver, addr, bus, e)
        else:
                print("Device %s:%x:%s already exists." % (driver, addr, bus))


def new_i2c_device(driver, addr, bus_number):
        bus = '/sys/bus/i2c/devices/i2c-%d' % bus_number
        devdir = "%d-%4.4x" % (bus_number, addr)
        return new_device(driver, addr, bus, devdir)

def baseconfig():
        print("Initialize Dell EMC Z9332F D1508 driver")
        #tlv eeprom device
        new_i2c_device('24lc64t', 0x52, 0)
        for y in range(40):
            new_i2c_device('optoe1',0x50,15+y+1)
def main():
        baseconfig()


if __name__ == "__main__":
        sys.exit(main())