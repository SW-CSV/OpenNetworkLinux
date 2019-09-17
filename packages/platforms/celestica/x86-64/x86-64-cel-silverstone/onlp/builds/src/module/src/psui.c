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
        }};

static const struct psu_reg_bit_mapper psu_mapper[PSU_COUNT + 1] = {
    {},
    {0xa160, 3, 7, 1},
    {0xa160, 2, 6, 0},
};

uint8_t psu_status = 0;

struct psuInfo_p temp_info[] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};

int onlp_psui_init(void)
{
    return ONLP_STATUS_OK;
}

int onlp_psui_info_get(onlp_oid_t id, onlp_psu_info_t *info_p)
{
    int psu_id;
    psu_id = ONLP_OID_ID_GET(id);
    *info_p = psu_info[psu_id];

    int present_status = 0, ac_status = 0, pow_status = 0;

    if (psu_status == 0)
        psu_status = getPsuStatus_sysfs_cpld(psu_id);

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

    psu_get_model_sn(psu_id, info_p->model, info_p->serial);

    psu_get_info(psu_id, &(info_p->mvin), &(info_p->mvout), &(info_p->mpin), &(info_p->mpout), &(info_p->miin), &(info_p->miout));

    return ONLP_STATUS_OK;
}
