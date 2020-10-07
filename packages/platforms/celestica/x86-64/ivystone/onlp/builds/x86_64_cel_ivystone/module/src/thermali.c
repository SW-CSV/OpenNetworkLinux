#include <unistd.h>
#include <onlplib/mmap.h>
#include <onlplib/file.h>
#include <onlp/platformi/thermali.h>
#include <fcntl.h>

// #include "i2c_chips.h"
#include "platform.h"

/*
max31730-i2c-7-4c   >> U148 (switch remote sensor)
tmp75-i2c-7-4f >> U41(base board right inlet sensor) 
tmp75-i2c-7-4e >>U3(base board center inlet sensor)       
tmp75-i2c-7-4d >> U33(switch outlet sensor)

tmp75-i2c-31-48    >> U8 (PSU inlet left sensor)
tmp75-i2c-31-49    >> U10 (PSU inlet right sensor)

tmp75-i2c-39-48  >>U8 (fan board left sensor)
tmp75-i2c-39-49 >> U10(fan board right sensor)

tmp75-i2c-42-48  >> U26(right top Linecard sensor)        
tmp75-i2c-42-49  >> U25(left top Linecard sensor)
          
tmp75-i2c-43-48  >> U26(left bottom Linecard sensor)        
tmp75-i2c-43-49  >> U25(right bottom Linecard sensor)
*/
static onlp_thermal_info_t thermal_info[] = {
    { },
    { { ONLP_THERMAL_ID_CREATE(THERMAL_SWITCH_REMOTE_U148), "Switch_Remote_Temp_U148",    0},
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
            },
    { { ONLP_THERMAL_ID_CREATE(THERMAL_BASE_R_INLET_TEMP_U41), "Base_R_Inlet_Temp_U7", 0},
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
            },
    { { ONLP_THERMAL_ID_CREATE(THERMAL_BASE_C_INLET_TEMP_U3), "Base_C_Inlet_Temp_U3",   0},
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
            },
    { { ONLP_THERMAL_ID_CREATE(THERMAL_SWITCH_OUTLET_TEMP_U33), "Switch_Outlet_Temp_U33",   0},
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
            },
    { { ONLP_THERMAL_ID_CREATE(THERMAL_PSU_INLET_L_TEMP_U8), "Psu_Inlet_L_Temp_U8",   0},
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
            },
    { { ONLP_THERMAL_ID_CREATE(THERMAL_PSU_INLET_R_TEMP_U10), "Psu_Inlet_R_Temp_U10",   0},
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
            },
    { { ONLP_THERMAL_ID_CREATE(THERMAL_FAN_L_TEMP_U8), "Fan_L_Temp_U8",   0},
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
            },
    { { ONLP_THERMAL_ID_CREATE(THERMAL_FAN_R_TEMP_U10), "Fan_R_Temp_U10",   0},
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
            },
    { { ONLP_THERMAL_ID_CREATE(THERMAL_RT_LINC_TEMP_U26), "Rt_Linc_Temp_U26",   0},
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
            },
    { { ONLP_THERMAL_ID_CREATE(THERMAL_LT_LINC_TEMP_U25), "Lt_Linc_Temp_U25",   0},
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
            },
    { { ONLP_THERMAL_ID_CREATE(THERMAL_RB_LINC_TEMP_U26), "Rb_Linc_Temp_U26",   0},
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
            },
     { { ONLP_THERMAL_ID_CREATE(THERMAL_LB_LINC_TEMP_U25), "Lb_Linc_Temp_U25",   0},
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
    if(thermal_status){
        info_p->status = ONLP_THERMAL_STATUS_FAILED;
    }
    
    info_p->caps = ONLP_THERMAL_CAPS_GET_TEMPERATURE;

    info_p->thresholds.warning = 0;
    
    info_p->thresholds.error = 0;

    info_p->thresholds.shutdown = 0;

    //printf("thermal status = %x\n",thermal_status);
    //LM75_1 THERMAL Status
    //if(((thermal_status >> 0)&0x01)){
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
