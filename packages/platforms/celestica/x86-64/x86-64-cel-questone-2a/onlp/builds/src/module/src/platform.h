#ifndef _PLATFORM_QUESTONE_H_
#define _PLATFORM_QUESTONE_H_
#include <stdint.h>

#define PREFIX_PATH_LEN 100
#define PSOC_CTRL_SMBUS 0x01
//FAN
#define FAN_COUNT   4
#define FAN_PRESENT 0xFB
#define FAN1_FRU_ID 5
#define FAN2_FRU_ID 6
#define FAN3_FRU_ID 7
#define FAN4_FRU_ID 8
//PSU
#define PSU_COUNT 2
#define PSUL_IPMI 3
#define PSUR_IPMI 4
#define PSU_REGISTER 0xA160
#define PSU_LED_REGISTER 0xA161
//THERMAL
#define THERMAL_COUNT 13
#define THERMAL_REGISTER 0xA176
//ALARM
#define ALARM_REGISTER 0xA163
//LED
#define LED_COUNT   8

#define LED_SYSTEM_H  1
#define LED_SYSTEM_REGISTER 0xA162
#define LED_SYSTEM_BOTH 3
#define LED_SYSTEM_GREEN 1
#define LED_SYSTEM_YELLOW 2
#define LED_SYSTEM_OFF 3
#define LED_SYSTEM_4_HZ 2
#define LED_SYSTEM_1_HZ 1

#define LED_FAN_1_H   2
#define LED_FAN_2_H   3
#define LED_FAN_4_H   4
#define LED_FAN_5_H   5
#define LED_ALARM_H   6
#define LED_PSU_1_H   7
#define LED_PSU_2_H   8
#define QSFP_FIRST 2
#define NELEMS(x)  (sizeof(x) / sizeof((x)[0]))

#define PSUL_ID 1
#define PSUR_ID 2

struct device_info{
	char serial_number[256];
	char model[256];
};

struct fan_config_p
{
    uint16_t pwm_reg;
	uint16_t ctrl_sta_reg;
	uint16_t rear_spd_reg;
    uint16_t front_spd_reg;
};

struct led_reg_mapper{
    char *name;
    uint16_t device;
    uint16_t dev_reg;
};



// struct psuInfo_p
// {
//     unsigned int vin;
// 	unsigned int iin;
// 	unsigned int vout;
// 	unsigned int iout;
// 	unsigned int pout;
// 	unsigned int pin;
// 	unsigned int temp;
// };
struct search_psu_sdr_info_mapper{
	char* keyword;
	char unit;
};

struct search_psu_fru_info_mapper{
	char* keyword;
	int start_index;
	int end_index;
};

typedef struct psuInfo_p
{
    unsigned int lvin;
	unsigned int liin;
	unsigned int lvout;
	unsigned int liout;
	unsigned int lpout;
	unsigned int lpin;
	unsigned int ltemp;
	
	unsigned int rvin;
	unsigned int riin;
	unsigned int rvout;
	unsigned int riout;
	unsigned int rpout;
	unsigned int rpin;
	unsigned int rtemp;
}psuInfo_p;

#define SYS_CPLD_PATH "/sys/devices/platform/sys_cpld/"
#define PLATFORM_PATH "/sys/devices/platform/questone2/"
#define PREFIX_PATH_ON_SYS_EEPROM "/sys/bus/i2c/devices/i2c-1/1-0056/eeprom"

int write_to_dump(uint8_t dev_reg);
int getFanPresent_tmp(int id);
uint8_t read_dump(uint16_t dev_reg);
int getFanSpeed_Percentage(int id, int *speed);
int getFanAirflow_tmp(int id);
int getFanSpeed_PWM(int id, int *speed);
uint8_t getPsuStatus(int id);
uint8_t getThermalStatus(int id);
uint8_t getLEDStatus(int id);
int led_present(int id,uint8_t value);
int led_mask(int start,uint8_t value);
int led_translate(int id, uint8_t value);
int fanSpeedSet(int id, unsigned short speed);
int psu_get_model_sn(int id,char* model,char* serial_number);
int psu_get_info(int id,int *mvin,int *mvout,int *mpin,int *mpout,int *miin,int *miout);
char* read_psu_fru(int id);
char* read_psu_sdr(int id);
char* read_ipmi(char* cmd);
int keyword_match(char* a,char *b);
void append(char* s, char c);
char* trim(char* a);
int getFaninfo(int id,char* model,char* serial);
int getThermalStatus_Ipmi(int id,int *tempc);
int deviceNodeReadBinary(char *filename, char *buffer, int buf_size, int data_len);
int deviceNodeReadString(char *filename, char *buffer, int buf_size, int data_len);
// #define PSU_FAN         2
// #define THERMAL_COUNT   6
// #define LED_COUNT       11

// #define THERMAL_MAIN_BOARD_REAR     0
// #define THERMAL_BCM                 1
// #define THERMAL_CPU                 2
// #define THERMAL_MAIN_BOARD_FRONT    3
// #define THERMAL_PSU1                4
// #define THERMAL_PSU2                5
#define DEBUG_MODE 0

#if (DEBUG_MODE == 1)
    #define DEBUG_PRINT(format, ...)   printf(format, __VA_ARGS__)
#else
    #define DEBUG_PRINT(format, ...)
#endif

#endif /* _PLATFORM_SEASTONE_H_ */
