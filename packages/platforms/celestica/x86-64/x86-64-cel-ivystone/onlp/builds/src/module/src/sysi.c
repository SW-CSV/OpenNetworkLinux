/*
 * Copyright
 */
#include <unistd.h>
#include <fcntl.h>

#include <onlplib/file.h>
#include <onlp/platformi/thermali.h>
#include <onlp/platformi/fani.h>
#include <onlp/platformi/sysi.h>
#include <x86_64_cel_ivystone/x86_64_cel_ivystone_config.h>

#include "x86_64_cel_ivystone_int.h"
#include "x86_64_cel_ivystone_log.h"
#include "platform.h"
#include <sys/stat.h>
#include <time.h>
#include <sys/mman.h>
#include <semaphore.h>

static int is_cache_exist(){
    const char *path="/tmp/onlp-sensor-cache.txt";
    const char *time_setting_path="/var/opt/interval_time.txt";
    time_t current_time;
    double diff_time;
    int interval_time = 30; //set default to 30 sec
    struct stat fst;
    bzero(&fst,sizeof(fst));

    //Read setting
    if(access(time_setting_path, F_OK) == -1){ //Setting not exist
        return -1;
    }else{
        FILE *fp;
        
        fp = fopen(time_setting_path, "r"); // read setting
        
        if (fp == NULL)
        {
            perror("Error while opening the file.\n");
            exit(EXIT_FAILURE);
        }

        fscanf(fp,"%d", &interval_time);

        fclose(fp);
    }

    if (access(path, F_OK) == -1){ //Cache not exist
        return -1;
    }else{ //Cache exist
        current_time = time(NULL);
        if (stat(path,&fst) != 0) { printf("stat() failed"); exit(-1); }

        diff_time = difftime(current_time,fst.st_mtime);

        if(diff_time > interval_time){
            return -1;
        }
        return 1;
    }
}

static void update_shm_mem(void)
{
    (void)fill_shared_memory(ONLP_SENSOR_FRU_CACHE_SHARED, ONLP_SENSOR_FRU_CACHE_SEM, ONLP_SENSOR_CACHE_FILE);
    (void)fill_shared_memory(ONLP_PSU_FRU_CACHE_SHARED, ONLP_PSU_FRU_CACHE_SEM, ONLP_PSU_CACHE_FILE);
    (void)fill_shared_memory(ONLP_FAN_FRU_CACHE_SHARED, ONLP_FAN_FRU_CACHE_SEM, ONLP_FAN_CACHE_FILE);
    (void)fill_shared_memory(ONLP_SYS_FRU_CACHE_SHARED, ONLP_SYS_FRU_CACHE_SEM, ONLP_SYS_CACHE_FILE);
    (void)fill_shared_memory(ONLP_STATUS_FRU_CACHE_SHARED, ONLP_STATUS_FRU_CACHE_SEM, ONLP_STATUS_CACHE_FILE);
    //(void)fill_shared_memory(ONLP_PSU_LED_CACHE_SHARED, ONLP_PSU_LED_CACHE_SEM, ONLP_PSU_LED_CACHE_FILE);
    //(void)fill_shared_memory(ONLP_FAN_LED_CACHE_SHARED, ONLP_FAN_LED_CACHE_SEM, ONLP_PSU_LED_CACHE_FILE);
}

static int create_cache(){
    //curl -g http://240.1.1.1:8080/api/sys/fruid/status |python -m json.tool Present status PSU,FAN
    //curl -g http://240.1.1.1:8080/api/sys/fruid/psu |python -m json.tool PSU FRU data
    //curl -g http://240.1.1.1:8080/api/sys/fruid/fan |python -m json.tool FAN FRU data
    //curl -g http://240.1.1.1:8080/api/sys/fruid/sys |python -m json.tool Board FRU
    // (void)system("curl -g http://240.1.1.1:8080/api/sys/sensors |python -m json.tool > /tmp/onlp-sensor-cache.txt");
    // (void)system("curl -g http://240.1.1.1:8080/api/sys/fruid/psu | python -m json.tool > /tmp/onlp-psu-fru-cache.txt");
    // (void)system("curl -g http://240.1.1.1:8080/api/sys/fruid/fan | python -m json.tool > /tmp/onlp-fan-fru-cache.txt");
    // (void)system("curl -g http://240.1.1.1:8080/api/sys/fruid/sys | python -m json.tool > /tmp/onlp-sys-fru-cache.txt");
    // (void)system("curl -g http://240.1.1.1:8080/api/sys/fruid/status | python -m json.tool > /tmp/onlp-status-fru-cache.txt");
    // (void)system("curl -d \'{\"data\":\"cat /sys/bus/i2c/devices/i2c-0/0-000d/psu_led_ctrl_en 2>/dev/null | head -n 1\"}\' http://240.1.1.1:8080/api/sys/raw | python -m json.tool > /tmp/onlp-psu-led-cache.txt");
    // (void)system("curl -d \'{\"data\":\"cat /sys/bus/i2c/devices/i2c-0/0-000d/fan_led_ctrl_en 2>/dev/null | head -n 1\"}\' http://240.1.1.1:8080/api/sys/raw | python -m json.tool > /tmp/onlp-fan-led-cache.txt");
    (void)system("curl -g http://240.1.1.1:8080/api/sys/sensors |python -m json.tool > /tmp/onlp-sensor-cache.tmp; sync; rm -f /tmp/onlp-sensor-cache.txt; mv /tmp/onlp-sensor-cache.tmp /tmp/onlp-sensor-cache.txt");
    (void)system("curl -g http://240.1.1.1:8080/api/sys/fruid/psu | python -m json.tool > /tmp/onlp-psu-fru-cache.tmp; sync; rm -f /tmp/onlp-psu-fru-cache.txt; mv /tmp/onlp-psu-fru-cache.tmp /tmp/onlp-psu-fru-cache.txt");
    (void)system("curl -g http://240.1.1.1:8080/api/sys/fruid/fan | python -m json.tool > /tmp/onlp-fan-fru-cache.tmp; sync; rm -f /tmp/onlp-fan-fru-cache.txt; mv /tmp/onlp-fan-fru-cache.tmp /tmp/onlp-fan-fru-cache.txt");
    (void)system("curl -g http://240.1.1.1:8080/api/sys/fruid/sys | python -m json.tool > /tmp/onlp-sys-fru-cache.tmp; sync; rm -f /tmp/onlp-sys-fru-cache.txt; mv /tmp/onlp-sys-fru-cache.tmp /tmp/onlp-sys-fru-cache.txt");
    (void)system("curl -g http://240.1.1.1:8080/api/sys/fruid/status | python -m json.tool > /tmp/onlp-status-fru-cache.tmp; sync; rm -f /tmp/onlp-status-fru-cache.txt; mv /tmp/onlp-status-fru-cache.tmp /tmp/onlp-status-fru-cache.txt");
    //(void)system("curl -d \'{\"data\":\"cat /sys/bus/i2c/devices/i2c-0/0-000d/psu_led_ctrl_en 2>/dev/null | head -n 1\"}\' http://240.1.1.1:8080/api/sys/raw | python -m json.tool > /tmp/onlp-psu-led-cache.tmp; sync; rm -f /tmp/onlp-psu-led-cache.txt; mv /tmp/onlp-psu-led-cache.tmp /tmp/onlp-psu-led-cache.txt");
    //(void)system("curl -d \'{\"data\":\"cat /sys/bus/i2c/devices/i2c-0/0-000d/fan_led_ctrl_en 2>/dev/null | head -n 1\"}\' http://240.1.1.1:8080/api/sys/raw | python -m json.tool > /tmp/onlp-fan-led-cache.tmp; sync; rm -f /tmp/onlp-fan-led-cache.txt; mv /tmp/onlp-fan-led-cache.tmp /tmp/onlp-fan-led-cache.txt");
    
    if(USE_SHM_METHOD){
        update_shm_mem();
    }
    return 1;
}

const char*
onlp_sysi_platform_get(void)
{
    return "x86-64-cel-ivystone-r0";
}

int
onlp_sysi_init(void)
{
    return ONLP_STATUS_OK;
}

int
onlp_sysi_onie_data_get(uint8_t** data, int* size)
{
    uint8_t* rdata = aim_zmalloc(256);
    if(onlp_file_read(rdata, 256, size, PREFIX_PATH_ON_SYS_EEPROM) == ONLP_STATUS_OK) {
        if(*size == 256) {
            *data = rdata;
            return ONLP_STATUS_OK;
        }
    }
    aim_free(rdata);
    rdata = NULL;
    DEBUG_PRINT("[Debug][%s][%d][Can't get onie data]\n", __FUNCTION__, __LINE__);
    return ONLP_STATUS_E_INTERNAL;
}

void
onlp_sysi_onie_data_free(uint8_t* data)
{
    aim_free(data);
}

int onlp_sysi_platform_manage_init(void)
{
    //printf("Check the sequence from onlp_sysi_platform_manage_init\n");
    if(is_cache_exist()<1){
        create_cache();
    }
    return ONLP_STATUS_OK;
}

int onlp_sysi_platform_manage_fans(void){
    //printf("Check the sequence from onlp_sysi_platform_manage_fans\n");
    if(is_cache_exist()<1){
        create_cache();
    }
    return ONLP_STATUS_OK;
}

int onlp_sysi_platform_manage_leds(void){
    //printf("Check the sequence from onlp_sysi_platform_manage_leds\n");
    if(is_cache_exist()<1){
        create_cache();
    }
    return ONLP_STATUS_OK;
}

int
onlp_sysi_oids_get(onlp_oid_t* table, int max)
{
    int i;
    onlp_oid_t* e = table;

    memset(table, 0, max*sizeof(onlp_oid_t));
    
    /* 4 PSUs */
    for (i=1; i<=PSU_COUNT; i++)
        *e++ = ONLP_PSU_ID_CREATE(i);

    /* LEDs Item */
    for (i=1; i<=LED_COUNT; i++)
        *e++ = ONLP_LED_ID_CREATE(i);

    /* THERMALs Item */
    for (i=1; i<=THERMAL_COUNT; i++)
        *e++ = ONLP_THERMAL_ID_CREATE(i);

    /* Fans Item */
    for (i=1; i<=FAN_COUNT; i++)
        *e++ = ONLP_FAN_ID_CREATE(i);

    return ONLP_STATUS_OK;
}

int 
onlp_sysi_platform_info_get(onlp_platform_info_t* platform_info)
{
    const char *tmp = onlp_sysi_platform_get();
    int size = 0;

    if(!platform_info)
	return ONLP_STATUS_E_PARAM;

    if(!platform_info->other_versions){
        platform_info->other_versions = aim_zmalloc(strlen(tmp) + 1);
        strncpy(platform_info->other_versions, tmp, strlen(tmp));
    }
    
    if(!platform_info->cpld_versions){
	uint8_t *cpld_versions = aim_zmalloc(SYS_BASE_CPLD_VERSION_LEN + 1);
        if(onlp_file_read(cpld_versions, SYS_BASE_CPLD_VERSION_LEN, &size,SYS_BASE_CPLD_VERSION_PATH) == ONLP_STATUS_OK)
	{
	    if(size == SYS_BASE_CPLD_VERSION_LEN)
	    {
		platform_info->cpld_versions = (char *)cpld_versions;
		return ONLP_STATUS_OK;
	    }
	}
        
	aim_free(cpld_versions);
    cpld_versions = NULL;
	return ONLP_STATUS_E_INTERNAL;
    }
  
    return ONLP_STATUS_OK;
}

void onlp_sysi_platform_info_free(onlp_platform_info_t *pi)
{
    aim_free(pi->cpld_versions);
    aim_free(pi->other_versions);
}
