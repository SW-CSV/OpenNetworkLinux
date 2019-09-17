#include <onlp/platformi/ledi.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include "platform.h"


uint8_t psu_status_l = 0;
uint8_t psu_led_result = 0xFF;

enum onlp_led_id
{
    LED_RESERVED = 0,
    LED_SYSTEM,
    LED_FAN_1,
    LED_FAN_2,
    LED_FAN_3,
    LED_FAN_4,
    LED_FAN_5,
    LED_FAN_6,
    LED_FAN_7,
    LED_ALARM,
    LED_PSU

};

/*
 * Get the information for the given LED OID.
 */
static onlp_led_info_t led_info[] =
{
    { },
    {
        { ONLP_LED_ID_CREATE(LED_SYSTEM), "System LED (Front)", 0 },
        ONLP_LED_STATUS_PRESENT,
        ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_YELLOW | ONLP_LED_CAPS_YELLOW_BLINKING | ONLP_LED_CAPS_GREEN | 
        ONLP_LED_CAPS_GREEN_BLINKING | ONLP_LED_CAPS_AUTO,
    },
    {
        { ONLP_LED_ID_CREATE(LED_FAN_1), "FAN(1) LED", 0 },
        ONLP_LED_STATUS_PRESENT,
        ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_RED |  ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_AUTO,
    },
    {
        { ONLP_LED_ID_CREATE(LED_FAN_2), "FAN(2) LED", 0 },
        ONLP_LED_STATUS_PRESENT,
        ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_RED |  ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_AUTO,
    },
    {
        { ONLP_LED_ID_CREATE(LED_FAN_3), "FAN(3) LED", 0 },
        ONLP_LED_STATUS_PRESENT,
        ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_RED |  ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_AUTO,
    },
    {
        { ONLP_LED_ID_CREATE(LED_FAN_4), "FAN(4) LED", 0 },
        ONLP_LED_STATUS_PRESENT,
        ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_RED |  ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_AUTO,
    },
    {
        { ONLP_LED_ID_CREATE(LED_FAN_5), "FAN(5) LED", 0 },
        ONLP_LED_STATUS_PRESENT,
        ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_RED |  ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_AUTO,
    },
    {
        { ONLP_LED_ID_CREATE(LED_FAN_6), "FAN(6) LED", 0 },
        ONLP_LED_STATUS_PRESENT,
        ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_RED |  ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_AUTO,
    },
    {
        { ONLP_LED_ID_CREATE(LED_FAN_7), "FAN(7) LED", 0 },
        ONLP_LED_STATUS_PRESENT,
        ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_RED |  ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_AUTO,
    },
    {
        { ONLP_LED_ID_CREATE(LED_ALARM), "Alert LED (Front)", 0 },
        ONLP_LED_STATUS_PRESENT,
        ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_YELLOW | ONLP_LED_CAPS_YELLOW_BLINKING | ONLP_LED_CAPS_GREEN | 
        ONLP_LED_CAPS_GREEN_BLINKING,
    },
    {
        { ONLP_LED_ID_CREATE(LED_PSU), "PSU LED (Front)", 0 },
        ONLP_LED_STATUS_PRESENT,
        ONLP_LED_CAPS_AUTO | ONLP_LED_CAPS_ORANGE | ONLP_LED_CAPS_GREEN,
    },
};

int
onlp_ledi_init(void)
{
    return ONLP_STATUS_OK;
}

int
onlp_ledi_info_get(onlp_oid_t id, onlp_led_info_t* info_p)
{
    int led_id;
    uint8_t led_color = 0;
    uint8_t blink_status = 0;
    uint8_t hw_control_status = 0;
    uint8_t result = 0;

    led_id = ONLP_OID_ID_GET(id);
    *info_p = led_info[led_id];
    switch(led_id){
        case LED_SYSTEM:
        case LED_ALARM:
        case LED_FAN_1:
        case LED_FAN_2:
        case LED_FAN_3:
        case LED_FAN_4:
        case LED_FAN_5:
        case LED_FAN_6:
        case LED_FAN_7:
            result = getLEDStatus(led_id);
            break;
        
        case LED_PSU:
            if(psu_led_result == 0xFF)
                psu_led_result = getLEDStatus(led_id);
            break;

    }
    

    if(result != 0xFF)
        info_p->status |= ONLP_LED_STATUS_ON;
    
    int psu_id = 1;

    if(psu_status_l == 0)
        psu_status_l = getPsuStatus_sysfs_cpld(psu_id);

    switch(led_id){
        case LED_SYSTEM:
        case LED_ALARM:

            led_color = (result >> 4)&0x3;
            if(led_color == 0){
                if(led_id==LED_SYSTEM)
                    info_p->mode |= ONLP_LED_MODE_AUTO;
                if(led_id==LED_ALARM)
                    info_p->mode |= ONLP_LED_MODE_OFF;
            }
            if(led_color == 1){
                info_p->mode |= ONLP_LED_MODE_GREEN;
            }
            if(led_color == 2){
                info_p->mode |= ONLP_LED_MODE_YELLOW;
            }
            if(led_color == 3){
                info_p->mode |= ONLP_LED_MODE_OFF;
                break;
            }

            blink_status = result & 0x3;
            if(blink_status == 1 || blink_status == 2){
                int current_mode = info_p->mode;
                info_p->mode = current_mode+1;
            }

            break;
        case LED_FAN_1:
        case LED_FAN_2:
        case LED_FAN_3:
        case LED_FAN_4:
        case LED_FAN_5:
        case LED_FAN_6:
        case LED_FAN_7:

            led_color = result & 0x3;

            if(led_color == 3){
                info_p->mode |= ONLP_LED_MODE_OFF;
            }else if(led_color == 1){
                info_p->mode |= ONLP_LED_MODE_GREEN;
            }else if(led_color == 2){
                info_p->mode |= ONLP_LED_MODE_RED;
            }else if(led_color == 0){
                info_p->mode |= ONLP_LED_MODE_AUTO;
            }
            break;
        case LED_PSU:
            hw_control_status = (psu_led_result >> 4) & 0x1;
            led_color = psu_led_result & 0x1;
            if(!hw_control_status)
            {
                if(led_color == 1){
                    info_p->mode = ONLP_LED_MODE_ORANGE;
                }else{
                    info_p->mode = ONLP_LED_MODE_GREEN;
                }
            }else{
                info_p->mode = ONLP_LED_MODE_AUTO;
            }
            break;
    }
    return ONLP_STATUS_OK;
}
