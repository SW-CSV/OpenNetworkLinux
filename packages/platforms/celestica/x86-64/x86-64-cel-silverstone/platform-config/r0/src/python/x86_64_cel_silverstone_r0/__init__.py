from onl.platform.base import *
from onl.platform.celestica import *

class OnlPlatform_x86_64_cel_silverstone_r0(OnlPlatformCelestica,
                                            OnlPlatformPortConfig_32x100):
    PLATFORM='x86-64-cel-silverstone-r0'
    MODEL="silverstone"
    SYS_OBJECT_ID=".2060.1"

    def baseconfig(self):
        onlp_interval_time = 30 #second
        file_path = "/var/opt/interval_time.txt"

        print("Initialize Silverstone driver")

        qsfp_qty = 32
        sfp_qty = 2
        qsfp_offset = 9

        self.insmod("i2c-ocores.ko")
        self.insmod("i2c-cls.ko")
        self.insmod("cls-switchboard.ko")
        #self.insmod("baseboard.ko")
        self.insmod("cpld_b.ko")
        #self.insmod("switchboard.ko")
        self.insmod("switchboard-diag.ko")
        self.insmod("mc24lc64t.ko")
        self.insmod("optoe.ko")
        self.insmod("xcvr-cls.ko")

        ###### new configuration for SDK support ########
        os.system("insmod /lib/modules/`uname -r`/kernel/net/core/pktgen.ko")
        os.system("insmod /lib/modules/`uname -r`/kernel/net/core/drop_monitor.ko")
        os.system("insmod /lib/modules/`uname -r`/kernel/net/ipv4/tcp_probe.ko")

        #tlv eeprom device
        self.new_i2c_device('24lc64t', 0x56, 0)

        #transceiver device
        for x in range(sfp_qty):
            self.new_i2c_device('optoe2',0x50,x+1)
        
        for y in range(qsfp_qty):
            self.new_i2c_device('optoe1',0x50,qsfp_offset+y+1)
            
        os.system("echo '3' > /proc/sys/kernel/printk")

        if os.path.exists(file_path):
            pass
        else:
            with open(file_path, 'w') as f:  
                f.write("{0}\r\n".format(onlp_interval_time))
            f.close()
        
        #initialize onlp cache files
        print("Initialize ONLP Cache files")
        os.system("ipmitool sdr > /tmp/onlp-sensor-cache.txt")
        os.system("ipmitool fru > /tmp/onlp-fru-cache.txt")
        os.system("ipmitool sensor list > /tmp/onlp-sensor-list-cache.txt")

        return True
