#include <onlp/platformi/psui.h>
#include "platform.h"

static onlp_psu_info_t psu_info[] =
    {
        {},
        {
            {ONLP_PSU_ID_CREATE(PSUL_ID), "PSU-Left", 0},
            "",
            "",
            0,
            ONLP_PSU_CAPS_AC | ONLP_PSU_CAPS_VIN | ONLP_PSU_CAPS_VOUT | ONLP_PSU_CAPS_IIN | ONLP_PSU_CAPS_IOUT | ONLP_PSU_CAPS_PIN | ONLP_PSU_CAPS_POUT,
        },
        {
            {ONLP_PSU_ID_CREATE(PSUR_ID), "PSU-Right", 0},
            "",
            "",
            0,
            ONLP_PSU_CAPS_AC | ONLP_PSU_CAPS_VIN | ONLP_PSU_CAPS_VOUT | ONLP_PSU_CAPS_IIN | ONLP_PSU_CAPS_IOUT | ONLP_PSU_CAPS_PIN | ONLP_PSU_CAPS_POUT,
        },
        {
            {ONLP_PSU_ID_CREATE(PSUM_ID), "PSU-Middle", 0},
            "",
            "",
            0,
            ONLP_PSU_CAPS_AC | ONLP_PSU_CAPS_VIN | ONLP_PSU_CAPS_VOUT | ONLP_PSU_CAPS_IIN | ONLP_PSU_CAPS_IOUT | ONLP_PSU_CAPS_PIN | ONLP_PSU_CAPS_POUT,
        }};
#if 0
static const struct psu_reg_bit_mapper psu_mapper[PSU_COUNT + 1] = {
    {},
    {0xa160, 3, 7, 1},
    {0xa160, 2, 6, 0},
    {},
};
#endif

/*
* cloverstone_dp doesn't use cpld but pca9506 to read PSU status
* 0x0180's 0x01 bit0--present, bit3--ac_state, bit5--power_state, according to ipmi protocol
*/
static const struct psu_reg_bit_mapper psu_mapper[PSU_COUNT + 1] = {
    {},
    {0, 0, 3, 5},
    {0, 0, 3, 5},
    {0, 0, 3, 5},
};

//uint8_t psu_status = 0;
uint32_t psu_status = 0;

struct psuInfo_p temp_info[] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};

int onlp_psui_init(void)
{
    int ret = 0;

    ret = onlp_lpc_connect(LPC_CONNECT_CMMBMC); // recover the sys cpld lpc init status
    if(ret != 0){
        return ret;
    }
    if (is_cache_exist() < 1)
    {
        create_cache();
    }
    return ONLP_STATUS_OK;
}

int onlp_psui_info_get(onlp_oid_t id, onlp_psu_info_t *info_p)
{
    int psu_id,psu_offset=0;
    psu_id = ONLP_OID_ID_GET(id);
    *info_p = psu_info[psu_id];

    int present_status=0,ac_status=0,pow_status=0;

    if(psu_status == 0)
        psu_status = get_psu_status(psu_id);

    present_status = (psu_status >> psu_mapper[psu_id].bit_present) & 0x01;
    ac_status = (psu_status >> psu_mapper[psu_id].bit_ac_sta) & 0x01;
    pow_status = (psu_status >> psu_mapper[psu_id].bit_pow_sta) & 0x01;

    if (!present_status)
    {
        info_p->status |= ONLP_PSU_STATUS_PRESENT;
        if (!(ac_status && pow_status))
            info_p->status = ONLP_PSU_STATUS_FAILED;
    }
    else
    {
        info_p->status |= ONLP_PSU_STATUS_UNPLUGGED;
    }

    get_psu_model_sn(psu_id,info_p->model,info_p->serial);

    get_psu_info(psu_id,&(info_p->mvin),&(info_p->mvout),&(info_p->mpin),&(info_p->mpout),&(info_p->miin),&(info_p->miout));

    if((info_p->mvin == 0) && (info_p->mpin == 0) && (info_p->miin == 0)){
        info_p->status |= ONLP_PSU_STATUS_UNPLUGGED;
    }

    if(psu_id == 1){
        psu_offset = 1;
    }else if(psu_id == 2){
        psu_offset = 4; // according to one PSU's thermal sensors num, here one PSU has 3 thermal sensors
    }else if(psu_id == 3){
        psu_offset = 7;
    }

    info_p->hdr.coids[0] = ONLP_THERMAL_ID_CREATE(psu_offset + CHASSIS_THERMAL_COUNT);
    info_p->hdr.coids[1] = ONLP_THERMAL_ID_CREATE(psu_offset + 1 + CHASSIS_THERMAL_COUNT);
    info_p->hdr.coids[2] = ONLP_THERMAL_ID_CREATE(psu_offset + 2 + CHASSIS_THERMAL_COUNT); // add a new array member here if adding a new thermal to PSU.
    info_p->hdr.coids[3] = ONLP_FAN_ID_CREATE(psu_id + CHASSIS_FAN_COUNT);

    return ONLP_STATUS_OK;
}
