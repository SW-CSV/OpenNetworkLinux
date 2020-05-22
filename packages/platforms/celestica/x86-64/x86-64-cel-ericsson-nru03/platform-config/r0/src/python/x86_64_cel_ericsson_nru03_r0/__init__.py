from onl.platform.base import *
from onl.platform.celestica import *

class OnlPlatform_x86_64_cel_ericsson_nru03_r0(OnlPlatformCelestica,
                                            OnlPlatformPortConfig_48x10_6x40):
    PLATFORM='x86-64-cel-ericsson-nru03-r0'
    MODEL="ericsson-nru03"
    SYS_OBJECT_ID=".2060.1"
    def baseconfig(self):
        os.system("echo '3' > /proc/sys/kernel/printk")
        return True
