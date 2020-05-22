from onl.platform.base import *
from onl.platform.celestica import *

class OnlPlatform_x86_64_cel_common_r0(OnlPlatformCelestica,
                                            OnlPlatformPortConfig_48x10_6x40):
    PLATFORM='x86-64-cel-common-r0'
    MODEL="common"
    SYS_OBJECT_ID=".2060.1"
