#ifndef _PLATFORM_IVYSTONE_H_
#define _PLATFORM_IVYSTONE_H_
#include <stdint.h>

#ifndef BMC_RESTFUL_API_SUPPORT
#define BMC_RESTFUL_API_SUPPORT
#endif

#define PREFIX_PATH_LEN 100
#define PSOC_CTRL_SMBUS 0x01
//FAN
#define FAN_COUNT   9
#define FAN_PRESENT 0xFB
#define CHASSIS_FAN_1 1
#define CHASSIS_FAN_2 2
#define CHASSIS_FAN_3 3
#define CHASSIS_FAN_4 4
#define CHASSIS_FAN_5 5
#define PSU_FAN_1_1 6
#define PSU_FAN_1_2 7
#define PSU_FAN_2_1 8
#define PSU_FAN_2_2 9
#define FAN1_FRU_ID 5
#define FAN2_FRU_ID 6
#define FAN3_FRU_ID 7
#define FAN4_FRU_ID 8
//PSU
#define PSU_COUNT 4
#define PSU1_ID 1
#define PSU2_ID 2
#define PSU3_ID 3
#define PSU4_ID 4
#define PSUL_IPMI 3
#define PSUR_IPMI 4
#define PSU_REGISTER 0xA15F
#define PSU_LED_REGISTER 0xA161
#define ONLP_PSU_LED_CACHE_FILE "/tmp/onlp-psu-led-cache.txt"
#define ONLP_FAN_LED_CACHE_FILE "/tmp/onlp-fan-led-cache.txt"
#define ONLP_SENSOR_CACHE_FILE "/tmp/onlp-sensor-cache.txt"
#define ONLP_PSU_CACHE_FILE "/tmp/onlp-psu-fru-cache.txt"
#define ONLP_FAN_CACHE_FILE "/tmp/onlp-fan-fru-cache.txt"
#define ONLP_SYS_CACHE_FILE "/tmp/onlp-sys-fru-cache.txt"
#define ONLP_STATUS_CACHE_FILE "/tmp/onlp-status-fru-cache.txt"

//THERMAL
#define THERMAL_COUNT 12
#define THERMAL_REGISTER 0xA176

#define  CPU_CORE_TEMPERATURE "/sys/class/thermal/thermal_zone0/temp"
#define THERMAL_SWITCH_REMOTE_U148  1
#define THERMAL_BASE_R_INLET_TEMP_U41 2
#define THERMAL_BASE_C_INLET_TEMP_U3 3
#define THERMAL_SWITCH_OUTLET_TEMP_U33 4
#define THERMAL_PSU_INLET_L_TEMP_U8 5
#define THERMAL_PSU_INLET_R_TEMP_U10 6
#define THERMAL_FAN_L_TEMP_U8 7
#define THERMAL_FAN_R_TEMP_U10 8
#define THERMAL_RT_LINC_TEMP_U26 9
#define THERMAL_LT_LINC_TEMP_U25 10
#define THERMAL_RB_LINC_TEMP_U26 11
#define THERMAL_LB_LINC_TEMP_U25 12
#define SWITCH_REMOTE_U148_NAME "max31730-i2c-7-4c"
#define BASE_R_INLET_TEMP_U41_NAME "tmp75-i2c-7-4f"
#define BASE_C_INLET_TEMP_U3_NAME "tmp75-i2c-7-4e"
#define SWITCH_OUTLET_TEMP_U33_NAME "tmp75-i2c-7-4d"
#define PSU_INLET_L_TEMP_U8_NAME "tmp75-i2c-31-48"
#define PSU_INLET_R_TEMP_U10_NAME "tmp75-i2c-31-49"
#define FAN_L_TEMP_U8_NAME "tmp75-i2c-39-48"
#define FAN_R_TEMP_U10_NAME "tmp75-i2c-39-49"
#define RT_LINC_TEMP_U26_NAME "tmp75-i2c-42-48"
#define LT_LINC_TEMP_U25_NAME "tmp75-i2c-42-49"
#define RB_LINC_TEMP_U26_NAME "tmp75-i2c-43-48"
#define LB_LINC_TEMP_U25_NAME "tmp75-i2c-43-49"

//ALARM
#define ALARM_REGISTER 0xA163
//LED
#ifndef BMC_RESTFUL_API_SUPPORT
#define LED_COUNT   8
#else
#define LED_COUNT   3
#endif
#define LED_SYSTEM_H  1
#define LED_PSU_REGISTER 0xA161
#define LED_SYSTEM_REGISTER 0xA162
#define LED_FAN_REGISTER    0xA163
#define LED_SYSTEM_BOTH 3
#define LED_SYSTEM_GREEN 1
#define LED_SYSTEM_YELLOW 2
#define LED_SYSTEM_OFF 3
#define LED_SYSTEM_4_HZ 2
#define LED_SYSTEM_1_HZ 1
#define LED_SYS_ENUM 1
#define LED_BLINK_ENUM 0
#define LED_GREEN_ENUM 1
#define LED_YELLOW_ENUM 2
#define LED_OFF_ENUM 3 

#define QSFP_FIRST 2
#define NELEMS(x)  (sizeof(x) / sizeof((x)[0]))


#define ONLP_SENSOR_FRU_CACHE_SHARED "/onlp-sensor-fru-cache-shared"
#define ONLP_PSU_FRU_CACHE_SHARED "/onlp-psu-fru-cache-shared"
#define ONLP_FAN_FRU_CACHE_SHARED "/onlp-fan-fru-cache-shared"
#define ONLP_SYS_FRU_CACHE_SHARED "/onlp-sys-fru-cache-shared"
#define ONLP_STATUS_FRU_CACHE_SHARED "/onlp-status-fru-cache-shared"
#define ONLP_PSU_LED_CACHE_SHARED "/onlp-psu-led-cache-shared"
#define ONLP_FAN_LED_CACHE_SHARED "/onlp-fan-led-cache-shared"

#define ONLP_SENSOR_FRU_CACHE_SEM "/onlp-sensor-fru-cache-sem"
#define ONLP_PSU_FRU_CACHE_SEM "/onlp-psu-fru-cache-sem"
#define ONLP_FAN_FRU_CACHE_SEM "/onlp-fan-fru-cache-sem"
#define ONLP_SYS_FRU_CACHE_SEM "/onlp-sys-fru-cache-sem"
#define ONLP_STATUS_FRU_CACHE_SEM "/onlp-status-fru-cache-sem"
#define ONLP_PSU_LED_CACHE_SEM "/onlp-psu-led-cache-sem"
#define ONLP_FAN_LED_CACHE_SEM "/onlp-fan-led-cache-sem"


struct shm_map_data{
    char data[16384]; 
    int size;
}; 

struct device_info{
	char serial_number[256];
	char model[256];
};

struct fan_config_p
{
	uint16_t front_spd_reg;
	uint16_t rear_spd_reg;
    uint16_t pwm_reg;
	uint16_t led_ctrl_reg;
	uint16_t ctrl_sta_reg;
};

struct led_reg_mapper{
    char *name;
    uint16_t device;
    uint16_t dev_reg;
};

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

#define SYS_CPLD_PATH "/sys/devices/platform/ivystone.cpldb/"
#define SYS_BASE_CPLD_VERSION_PATH "/sys/devices/platform/ivystone.cpldb/version"
#define SYS_BASE_CPLD_VERSION_LEN 4
#define PLATFORM_PATH "/sys/devices/platform/ivystone.switchboard/"
#define PREFIX_PATH_ON_SYS_EEPROM "/sys/bus/i2c/devices/i2c-0/0-0056/eeprom"
#define FAN_CPLD_PATH "/sys/devices/platform/ivystone.switchboard/FAN_CPLD/"

//Cache file name
#define PSU_CACHE_PATH "/tmp/onlp-psu-fru-cache.txt"

int write_to_dump(uint8_t dev_reg);
int get_fan_present_status(int id);
int get_fan_board_sn(int id, char *sn);
int get_fan_board_md(int id, char *md);
uint8_t read_dump(uint16_t dev_reg);
int get_rear_fan_per(int id, int *speed);
int get_rear_fan_pwm(int id, int *speed);
uint8_t get_psu_status(int id);
uint8_t getThermalStatus(int id);
uint8_t get_led_status(int id);
uint8_t get_fan_led_status(int id);
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
int read_cpu_temp(int *temperature);
int get_psu_item_content(int id, char *item, char *content);
int read_psu_inout(const char *adapter, const char *name, const char *item, char *content);
int search_current_val(char *amper_c, int *amper_i);
int search_voltage_val(char *volt_c, int *volt_i);
int search_power_val(char *watt_c, int *watt_i);
int get_rear_fan_rpm(int id, int *rpm);
int search_rpm_val(char *rpm_c, int *rpm_i);
int dump_shared_memory(const char *shm_path, const char *sem_path, struct shm_map_data *shared_mem);
int fill_shared_memory(const char *shm_path, const char *sem_path, const char *cache_path);\
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

#define USE_SHM_METHOD 0

#endif /* _PLATFORM_IVYSTONE_H_ */
