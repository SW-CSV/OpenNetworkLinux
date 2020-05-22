from onl.platform.base import *
from onl.platform.celestica import *

class OnlPlatform_x86_64_cel_dx032_r0(OnlPlatformCelestica,
                                            OnlPlatformPortConfig_48x10_6x40):
    PLATFORM='x86-64-cel-dx032-r0'
    MODEL="DX032"
    SYS_OBJECT_ID=".2060.1"
    def baseconfig(self):
        os.system("echo '3' > /proc/sys/kernel/printk")
        return True

