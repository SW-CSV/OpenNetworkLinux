from onl.platform.base import *
from onl.platform.celestica import *

class OnlPlatform_x86_64_cel_silverstone_2_r0(OnlPlatformCelestica,
                                            OnlPlatformPortConfig_2x10_32x400):
    PLATFORM='x86-64-cel-silverstone-2-r0'
    MODEL="silverstone-2"
    SYS_OBJECT_ID=".2060.1"
