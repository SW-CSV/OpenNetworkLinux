#include <onlp/platformi/psui.h>

// #include "i2c_chips.h"
#include "platform.h"

static
onlp_psu_info_t psu_info[] =
{   
    { },/* Not used */
    {
            { ONLP_PSU_ID_CREATE(PSUL_ID), "PSU-Left", 0 },
    },
    {
            { ONLP_PSU_ID_CREATE(PSUR_ID), "PSU-Right", 0 },
    }
};

struct psuInfo_p temp_info[]={
    {0,0,0,0,0,0,0,0,0,0,0,0,0}
};

int
onlp_psui_init(void)
{
    return ONLP_STATUS_OK;
}

int
onlp_psui_info_get(onlp_oid_t id, onlp_psu_info_t* info_p)
{
    int psu_id;
    //struct psuInfo psu;
    //bool psu_status;
    psu_id = ONLP_OID_ID_GET(id);
    *info_p = psu_info[psu_id];

    uint8_t psu_stat;
    int absent_status = 1;

    psu_stat = getPsuStatus(psu_id);

    if(psu_stat != 0xFF){

        if(psu_id == PSUL_ID){
            absent_status = (psu_stat >> 5) &0x1;
            if(!absent_status)
                info_p->status |= ONLP_PSU_STATUS_PRESENT;
        }else{
            absent_status = (psu_stat >> 4) &0x1;
            if(!absent_status)
                info_p->status |= ONLP_PSU_STATUS_PRESENT;
        }

    }else{
       return ONLP_STATUS_E_MISSING;
    }

    psu_get_model_sn(psu_id,info_p->model,info_p->serial);
    
    psu_get_info(psu_id,&(info_p->mvin),&(info_p->mvout),&(info_p->mpin),&(info_p->mpout),&(info_p->miin),&(info_p->miout));

    if ((info_p->mpin) < 0 || (info_p->mvin)<0)
        info_p->status |= ONLP_PSU_STATUS_UNPLUGGED;

    return ONLP_STATUS_OK;
}
