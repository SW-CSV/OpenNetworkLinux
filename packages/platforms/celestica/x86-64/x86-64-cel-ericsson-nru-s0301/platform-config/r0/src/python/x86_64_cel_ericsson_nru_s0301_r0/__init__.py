from onl.platform.base import *
from onl.platform.celestica import *

class OnlPlatform_x86_64_cel_ericsson_nru_s0301_r0(OnlPlatformCelestica,
                                            OnlPlatformPortConfig_48x10_6x40):
    PLATFORM='x86-64-cel-ericsson-nru-s0301-r0'
    MODEL="ericsson-nru-s0301"
    SYS_OBJECT_ID=".2060.1"
    def baseconfig(self):
        os.system("echo '3' > /proc/sys/kernel/printk")
        #initialize onlp cache files
        print("Initialize ONLP Cache files")
        os.system("ipmitool sdr > /tmp/onlp-sensor-cache.txt")
        os.system("ipmitool fru > /tmp/onlp-fru-cache.txt")
        os.system("ipmitool sensor list > /tmp/onlp-sensor-list-cache.txt")
        return True
