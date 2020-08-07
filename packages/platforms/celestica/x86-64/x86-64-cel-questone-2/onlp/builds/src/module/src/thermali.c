#include <unistd.h>
#include <onlplib/mmap.h>
#include <onlplib/file.h>
#include <onlp/platformi/thermali.h>
#include <fcntl.h>

// #include "i2c_chips.h"
#include "platform.h"

static onlp_thermal_info_t thermal_info[] = {
    { },
    { { ONLP_THERMAL_ID_CREATE(1), "Base_Temp_U5",    0},
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
            },
    { { ONLP_THERMAL_ID_CREATE(2), "Base_Temp_U7", 0},
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
            },
    { { ONLP_THERMAL_ID_CREATE(3), "CPU_Temp",   0},
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
            },
    { { ONLP_THERMAL_ID_CREATE(3), "Switch_Temp_U31",   0},
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
            },
    { { ONLP_THERMAL_ID_CREATE(3), "Switch_Temp_U30",   0},
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
            },
    { { ONLP_THERMAL_ID_CREATE(3), "Switch_Temp_U28",   0},
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
            },
    { { ONLP_THERMAL_ID_CREATE(3), "Switch_Temp_U29",   0},
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
            },
    { { ONLP_THERMAL_ID_CREATE(3), "PSUL_Temp1",   0},
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
            },
    { { ONLP_THERMAL_ID_CREATE(3), "PSUL_Temp2",   0},
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
            },
    { { ONLP_THERMAL_ID_CREATE(3), "PSUR_Temp1",   0},
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
            },
    { { ONLP_THERMAL_ID_CREATE(3), "PSUR_Temp2",   0},
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
            },
    { { ONLP_THERMAL_ID_CREATE(3), "Switch_U21_Temp",   0},
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
            },
    { { ONLP_THERMAL_ID_CREATE(3), "Switch_U33_Temp",   0},
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
            }
    
};

int
onlp_thermali_init(void)
{
    return ONLP_STATUS_OK;
}

int
onlp_thermali_info_get(onlp_oid_t id, onlp_thermal_info_t* info_p)
{
    int thermal_id;
    thermal_id = ONLP_OID_ID_GET(id);
    *info_p = thermal_info[thermal_id];
    int thermal_status = getThermalStatus_Ipmi(thermal_id,&(info_p->mcelsius));
    if(!(thermal_status)){
        info_p->status = ONLP_THERMAL_STATUS_FAILED;
    }
    //printf("thermal status = %x\n",thermal_status);
    //LM75_1 THERMAL Status
    // if(((thermal_status >> 0)&0x01)){
    //     info_p[0].status = ONLP_THERMAL_STATUS_PRESENT;
    // }else{
    //     info_p[0].status = ONLP_THERMAL_STATUS_FAILED;
    // }

    // //LM75_2 THERMAL Status
    // if(((thermal_status >> 1)&0x01)){
    //     info_p[1].status = ONLP_THERMAL_STATUS_PRESENT;
    // }else{
    //     info_p[1].status = ONLP_THERMAL_STATUS_FAILED;
    // }

    // //LM75 THERMAL Status
    // if(((thermal_status >> 2)&0x01)){
    //     info_p[2].status = ONLP_THERMAL_STATUS_PRESENT;
    // }else{
    //     info_p[2].status = ONLP_THERMAL_STATUS_FAILED;
    // }

    return ONLP_STATUS_OK;
}
