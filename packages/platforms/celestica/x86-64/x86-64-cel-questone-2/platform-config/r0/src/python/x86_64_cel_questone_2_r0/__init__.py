from onl.platform.base import *
from onl.platform.celestica import *

class OnlPlatform_x86_64_cel_questone_2_r0(OnlPlatformCelestica,
                                            OnlPlatformPortConfig_48x10_8x100):
    PLATFORM='x86-64-cel-questone-2-r0'
    MODEL="Questone-2"
    SYS_OBJECT_ID=".2060.1"

    def baseconfig(self):
        onlp_interval_time = 30  # second
        file_path = "/var/opt/interval_time.txt"
        qsfp_quantity = 6
        sfp_quantity = 48
        sfp_i2c_start_bus = 2
        print("Initialize and Install the driver here")
        self.insmod("questone2_switchboard.ko")
        self.insmod("questone2_baseboard_cpld.ko")
        self.insmod("optoe.ko")
        self.insmod("mc24lc64t.ko")

        # ###### new configuration for SDK support ########
        os.system("insmod /lib/modules/`uname -r`/kernel/net/core/pktgen.ko")
        os.system("insmod /lib/modules/`uname -r`/kernel/net/core/drop_monitor.ko")
        os.system("insmod /lib/modules/`uname -r`/kernel/net/ipv4/tcp_probe.ko")

        # eeprom driver
        self.new_i2c_device('24lc64t', 0x56, 1)
        # initialize SFP devices name
        for actual_i2c_port in range(sfp_i2c_start_bus, sfp_i2c_start_bus+(qsfp_quantity+sfp_quantity)):
            port_number = actual_i2c_port - (sfp_i2c_start_bus-1)
            if(port_number <= sfp_quantity):
                #print("echo 'QSFP{1}' > /sys/devices/i2c-{0}/{0}-0050/port_name".format(actual_i2c_port,port_number))
                os.system("echo 'SFP{1}' > /sys/devices/i2c-{0}/{0}-0050/port_name".format(actual_i2c_port,port_number))
            else:
                #print("echo 'SFP{1}' > /sys/devices/i2c-{0}/{0}-0050/port_name".format(actual_i2c_port,port_number-qsfp_quantity))
                os.system("echo 'QSFP{1}' > /sys/devices/i2c-{0}/{0}-0050/port_name".format(actual_i2c_port,port_number-sfp_quantity))
            # self.new_i2c_device('sff8436', 0x50, port)
            # self.new_i2c_device('as5912_54x_sfp%d' % port, 0x51, port+25)
        
        # Script for create interval_time cache.
        if os.path.exists(file_path):
            pass
        else:
            with open(file_path, 'w') as f:  
                f.write("{0}\r\n".format(onlp_interval_time))
            f.close()

        #initialize onlp cache files
        print("Initialize ONLP Cache files")
        os.system("ipmitool fru > /tmp/onlp-fru-cache.tmp; sync; rm -f /tmp/onlp-fru-cache.txt; mv /tmp/onlp-fru-cache.tmp /tmp/onlp-fru-cache.txt")
        os.system("ipmitool sensor list > /tmp/onlp-sensor-list-cache.tmp; sync; rm -f /tmp/onlp-sensor-list-cache.txt; mv /tmp/onlp-sensor-list-cache.tmp /tmp/onlp-sensor-list-cache.txt")
        
        return True
