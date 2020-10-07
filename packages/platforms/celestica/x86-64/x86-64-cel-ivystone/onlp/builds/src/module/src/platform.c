//////////////////////////////////////////////////////////////
//   PLATFORM FUNCTION TO INTERACT WITH SYS_CPLD AND BMC    //
//////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/io.h>
#include <cjson/cJSON.h>
#include <cjson_util/cjson_util.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <errno.h>
#include "platform.h"

char command[256];
FILE *fp;
static char *sdr_value = NULL;
//static char *temp_sdr_value = NULL;

static struct device_info fan_information[FAN_COUNT + 1] = {
    {"unknown", "unknown"}, //check
    {}, //Fan 1
    {}, //Fan 2
    {}, //Fan 3
    {}, //Fan 4
};

static struct device_info psu_information[PSU_COUNT + 1] = {
    {"unknown", "unknown"}, //check
    {}, //PSU 1
    {}, //PSU 2
};

static const struct fan_config_p fan_sys_reg[FAN_COUNT + 1] = {
    /*	uint16_t front_spd_reg;
	uint16_t rear_spd_reg;
    uint16_t pwm_reg;
	uint16_t led_ctrl_reg;
	uint16_t ctrl_sta_reg;*/
    {},
    {0x20, 0x21, 0x22, 0x24,0x26}, //Fan 1
    {0x40, 0x41, 0x42, 0x44,0x46}, //Fan 2
    {0x60, 0x60, 0x62, 0x64,0x66}, //Fan 3
    {0x80, 0x81, 0x82, 0x84,0x86}, //Fan 4
    {0xA0, 0xA1, 0xA2, 0xA4,0xA6}  //Fan 5
};
#ifndef BMC_RESTFUL_API_SUPPORT
static const struct led_reg_mapper led_mapper[LED_COUNT + 1] = {
    /*char *name;
    uint16_t device;
    uint16_t dev_reg;*/
    {},
    {"LED_SYSTEM", 1, LED_SYSTEM_REGISTER},
    {"LED_FAN_1", 2, 0x24},
    {"LED_FAN_2", 3, 0x44},
    {"LED_FAN_3", 4, 0x64},
    {"LED_FAN_4", 5, 0x84},
    {"LED_FAN_5", 6, 0xA4},
    {"LED_ALARM", 7, ALARM_REGISTER},
    {"LED_PSU", 8, PSU_LED_REGISTER}
};
#else
static const struct led_reg_mapper led_mapper[LED_COUNT + 1] = {
    /*char *name;
    uint16_t device;
    uint16_t dev_reg;*/
    {},
    {"LED_SYSTEM", 1, LED_SYSTEM_REGISTER},
    {"LED_PSU", 2, LED_PSU_REGISTER},
    {"LED_FAN", 3, LED_FAN_REGISTER}
};
#endif
static const struct search_psu_sdr_info_mapper search_psu_sdr_info[12] = {
    {"PSUL_VIn", 'V'},
    {"PSUL_CIn", 'A'},
    {"PSUL_PIn", 'W'},
    {"PSUL_VOut", 'V'},
    {"PSUL_COut", 'A'},
    {"PSUL_POut", 'W'},
    {"PSUR_VIn", 'V'},
    {"PSUR_CIn", 'A'},
    {"PSUR_PIn", 'W'},
    {"PSUR_VOut", 'V'},
    {"PSUR_COut", 'A'},
    {"PSUR_POut", 'W'},
};

char *Thermal_sensor_name[THERMAL_COUNT] = {
    "Switch_Remote_Temp_U148", "Base_R_Inlet_Temp_U7", "Base_C_Inlet_Temp_U3", "Switch_Outlet_Temp_U33",
    "Psu_Inlet_L_Temp_U8", "Psu_Inlet_R_Temp_U10", "Fan_L_Temp_U8", "Fan_R_Temp_U10",
    "Rt_Linc_Temp_U26", "Lt_Linc_Temp_U25", "Rb_Linc_Temp_U26","Lb_Linc_Temp_U25"};

int write_to_dump(uint8_t dev_reg)
{
    uint8_t ret;

    sprintf(command, "echo 0x%x> " SYS_CPLD_PATH "getreg", dev_reg);
    ret = system(command);
    return ret;
}

int is_shm_mem_ready(){

    if(USE_SHM_METHOD){
        const char *sdr_cache_path="/run/shm/onlp-sensor-fru-cache-shared";
        const char *fru_cache_path="/run/shm/onlp-status-fru-cache-shared";

        if(access(fru_cache_path, F_OK) == -1 || access(sdr_cache_path, F_OK) == -1 ){ //Shared cache files not exist
            return 0;
        }

    return 1;
    }
    
    return 0;
}

char *read_tmp_cache(char *cmd, char *cache_file_path)
{
    FILE* pFd = NULL;
    char *str = NULL;
    int round = 1;

    for(round = 1;round <= 10;round++){
        pFd = fopen(cache_file_path, "r");
        if(pFd != NULL ){

            struct stat st;

            stat(cache_file_path, &st);

            int size = st.st_size;
            str = (char *)malloc(size + 1);
            
            memset (str, 0, size+1);

            fread(str, size+1, 1, pFd);

            fclose(pFd);
            break;
        }else{
            usleep(5000); //Sleep for 5 Microsec for waiting the file operation complete
        }
    }

    if(round >= 10 && str == NULL){
        str = (char *)malloc(1);
        memset (str, 0, 1);
    }
    
    return str;
}

/* Parse text to JSON, then render back to text, and print! */
void doit(char *text)
{
    char *out = NULL;
    cJSON *json = NULL;

    json = cJSON_Parse(text);
    if (!json)
    {
        printf("Error before: [%s]\n", cJSON_GetErrorPtr());
    }
    else
    {
        out = cJSON_Print(json);
        cJSON_Delete(json);
        printf("%s\n", out);
        free(out);
    }
}

int fill_shared_memory(const char *shm_path, const char *sem_path, const char *cache_path)
{
    int seg_size = 0;    
    int shm_fd = -1;   
    struct shm_map_data * shm_map_ptr = (struct shm_map_data *)NULL;

    if(!shm_path || !sem_path || !cache_path){
	return -1;
    }

    seg_size = sizeof(struct shm_map_data);

    shm_fd = shm_open(shm_path, O_CREAT|O_RDWR, S_IRWXU|S_IRWXG);
    if(shm_fd < 0){
        
	printf("\nshm_path:%s. errno:%d\n", shm_path, errno);
        return -1;
    }   
 
    ftruncate(shm_fd, seg_size);

    shm_map_ptr = (struct shm_map_data *)mmap(NULL, seg_size, PROT_READ|PROT_WRITE, MAP_SHARED, shm_fd, 0); 
    if(shm_map_ptr == MAP_FAILED){
	printf("\nMAP_FAILED. errno:%d.\n", errno);
    	close(shm_fd);
        return -1;
    }

    if(access(cache_path, F_OK) == -1)
    {
        munmap(shm_map_ptr, seg_size);
	close(shm_fd);
	return -1;
    }
 
    struct stat sta;
    stat(cache_path, &sta);
    int st_size = sta.st_size;
    if(st_size == 0){
        munmap(shm_map_ptr, seg_size);
	close(shm_fd);
	return -1;
    }

    char *cache_buffer = (char *)malloc(st_size); 
    if(!cache_buffer){ 
        munmap(shm_map_ptr, seg_size);
	close(shm_fd);
        return -1;
    }

    memset(cache_buffer, 0, st_size);
 
    FILE *cache_fp = fopen(cache_path, "r");
    if(!cache_fp)
    {
        free(cache_buffer);   
        munmap(shm_map_ptr, seg_size);
	close(shm_fd);
        return -1;
    }

    int cache_len = fread(cache_buffer, 1, st_size, cache_fp);
    if(st_size != cache_len)
    {
        munmap(shm_map_ptr, seg_size);
	close(shm_fd);
 	free(cache_buffer);
	fclose(cache_fp);
	return -1;
    }

    sem_t * sem_id = sem_open(sem_path, O_CREAT, S_IRUSR | S_IWUSR, 1);
    if(sem_id == SEM_FAILED){
        munmap(shm_map_ptr, seg_size);
	close(shm_fd);
        free(cache_buffer);
        fclose(cache_fp);
        return -1;
    }    

    sem_wait(sem_id);

    memcpy(shm_map_ptr->data, cache_buffer, st_size); 
    
    shm_map_ptr->size = st_size;
 
    sem_post(sem_id);

    (void)free(cache_buffer);

    //shm_unlink(shm_path);
    
    sem_close(sem_id);

    //sem_unlink(sem_path);

    munmap(shm_map_ptr, seg_size);
   
    close(shm_fd);

    return 0; 
} 

int dump_shared_memory(const char *shm_path, const char *sem_path, struct shm_map_data *shared_mem)
{
    sem_t *sem_id = (sem_t *)NULL;
    struct shm_map_data *map_ptr = (struct shm_map_data *)NULL;
    int seg_size = 0;
    int shm_fd = -1;

    if(!shm_path || !sem_path || !shared_mem){
	return -1;
    }

    seg_size = sizeof(struct shm_map_data);

    shm_fd = shm_open(shm_path, O_RDONLY, 0666);
    if(shm_fd < 0){
   	printf("\ndump shm_path:%s. errno:%d\n", shm_path, errno);
        return -1; 
    }

    map_ptr = (struct shm_map_data *)mmap(NULL, seg_size, PROT_READ, MAP_SHARED, shm_fd, 0);
    if(map_ptr == MAP_FAILED){
	printf("\ndump mmap failed. errno:%d.\n", errno);
	close(shm_fd);
	return -1;
    }   
 
    sem_id = sem_open(sem_path, 0);
    if(SEM_FAILED == sem_id){
	printf("\nsem open failed. errno:%d.\n", errno);
	munmap(map_ptr, seg_size);
	close(shm_fd);
	return -1;
    }

    sem_wait(sem_id);
    
    memcpy(shared_mem, map_ptr, sizeof(struct shm_map_data));
   
    sem_post(sem_id);
    
    //shm_unlink(shm_path);

    sem_close(sem_id);
 
    //sem_unlink(sem_path);
    
    munmap(map_ptr, seg_size);
    close(shm_fd);

    return 0;
}

uint8_t read_register(uint16_t dev_reg)
{
    int status;
    //printf("Input of read_register = %u\n",dev_reg);
    //sprintf(command,"echo %x",dev_reg);// > "SYSCPLD_PATH"dump"
    sprintf(command, "echo 0x%x >  %sgetreg", dev_reg, SYS_CPLD_PATH);
    //printf("Command %s\n",command);
    fp = popen(command, "r");
    if (!fp)
    {
        printf("Failed : Can't specify CPLD register\n");
        return -1;
    }
    pclose(fp);
    fp = popen("cat " SYS_CPLD_PATH "getreg", "r");
    if (!fp)
    {
        printf("Failed : Can't open sysfs\n");
        return -1;
    }
    fscanf(fp, "%x", &status);
    pclose(fp);

    return status;
}

uint8_t read_fan_register(uint16_t dev_reg)
{
    int status;

    //printf("Input of read_register = %u\n",dev_reg);
    //sprintf(command,"echo %x",dev_reg);// > "SYSCPLD_PATH"dump"
    sprintf(command, "echo 0x%x >  %sgetreg", dev_reg, FAN_CPLD_PATH);
    //printf("Command %s\n",command);
    fp = popen(command, "r");
    if (!fp)
    {
        printf("Failed : Can't specify CPLD register\n");
        return -1;
    }
    pclose(fp);
    fp = popen("cat " SYS_CPLD_PATH "getreg", "r");
    if (!fp)
    {
        printf("Failed : Can't open sysfs\n");
        return -1;
    }
    fscanf(fp, "%x", &status);
    pclose(fp);

    return status;
}

uint8_t write_register(uint16_t dev_reg, uint16_t write_data)
{

    int status;
    //printf("Input of read_register = %u",dev_reg);
    //sprintf(command,"echo %x",dev_reg);// > "SYSCPLD_PATH"dump"
    //sprintf(command,"echo %x >  %sgetreg",dev_reg,SYS_CPLD_PATH);
    //printf("Command %s\n",command);
    sprintf(command, "echo 0x%x %x>  %ssetreg", dev_reg, write_data, SYS_CPLD_PATH);
    fp = popen(command, "r");
    if (!fp)
    {
        printf("Failed : Can't specify CPLD register\n");
        return -1;
    }
    fscanf(fp, "%x", &status);
    pclose(fp);

    return status;
}
#ifdef BMC_RESTFUL_API_SUPPORT

int phrase_json_buffer(const char *shm_path, const char *sem_path, char **cache_data, int *cache_size)
{
    int res = -1;
    char *tmp_ptr = (char *)NULL;
    struct shm_map_data shm_map_tmp;

    memset(&shm_map_tmp, 0, sizeof(struct shm_map_data));

    res = dump_shared_memory(shm_path, sem_path, &shm_map_tmp);
    if(!res){
	tmp_ptr = malloc(shm_map_tmp.size);
        if(!tmp_ptr){
	    res = -1;
	    return res;
	}	

	memset(tmp_ptr, 0, shm_map_tmp.size);

        memcpy(tmp_ptr, shm_map_tmp.data, shm_map_tmp.size);

        *cache_data = tmp_ptr;

        *cache_size = shm_map_tmp.size;
    }

    return res; 
}

int phrase_json_key_word(char *content, char *key, char *sub, int id, cJSON **output)
{
    cJSON *root = (cJSON *)NULL;
    int res = -1;
    int i = 0;
    cJSON *tmp = (cJSON *)NULL;

    if (!content)
        return res;

    root = cJSON_Parse(content);
    if (!root)
        return res;

    cJSON *item = cJSON_GetObjectItem(root, key);
    if (!item)
    {
        cJSON_Delete(root);
        return res;
    }

    if (id > cJSON_GetArraySize(item))
    {
        cJSON_Delete(root);
        return res;
    }

    for (i = 0; i < cJSON_GetArraySize(item); i++)
    {
        cJSON *subitem = cJSON_GetArrayItem(item, i);
        if (!subitem)
        {
            continue;
        }

        tmp = cJSON_GetObjectItem(subitem, sub);
        if (!tmp)
        {
            continue;
        }

        cJSON *object = (cJSON *)malloc(sizeof(cJSON));
        if (!object)
        {
            cJSON_Delete(root);
            return res;
        }

        if (tmp->valuestring)
        {
            object->valuestring = malloc(strlen(tmp->valuestring));
            memset(object->valuestring, 0, strlen(tmp->valuestring));
            memcpy(object->valuestring, tmp->valuestring, strlen(tmp->valuestring));
        }

        object->valueint = tmp->valueint;
        object->valuedouble = tmp->valuedouble;
        *output = object;

        break;
    }

    /* may be memory leak, i mean the root */
    cJSON_Delete(root);
    res = 0;
    return res;
}

int get_fan_present_status(int id)
{
    int ret = -1;
    char subitem[8] = {0};

    if (id <= (FAN_COUNT))
    {
        uint8_t result = 0;
        char *tmp = NULL;
        cJSON *present = (cJSON *)NULL; 
	    int len = 0;
    
       	if(is_shm_mem_ready()){
            result = phrase_json_buffer(ONLP_STATUS_FRU_CACHE_SHARED, ONLP_STATUS_FRU_CACHE_SEM, &tmp, &len);
            if(ret < 0 || !tmp){
                return ret;
                }
        }else{
            sprintf(command, "cat %s",ONLP_STATUS_CACHE_FILE);
            tmp = read_tmp_cache(command,ONLP_STATUS_CACHE_FILE);
        }

        if(result){
            if(tmp){
                (void)free(tmp);
            tmp = (char *)NULL;
            }
            return ret;
        }

        memset(subitem, 0, sizeof(subitem));
        (void)snprintf(subitem, 8, "Fan%d", id);
        
        result += phrase_json_key_word(tmp, "Information", subitem, id - 1, &present);
        if(!present){
	        ret = -1;
	        printf("\nPresent Nothing.\n");
	    }
        else{
            if(!strncmp(present->valuestring, " Present", strlen(" Present"))){
                ret = 0;
                //printf("\nMatch\n");
            }
            else{
                ret = -1;
                printf("\nMismatch\n");	
 	        }
        
            if(tmp){
                (void)free(tmp);
                tmp = (char *)NULL;
            }

        free(present->valuestring);
	    free(present);
	    }
    }

    return ret;
}

int phrase_fan_array(cJSON *information, int id, const char *item, char *content)
{
    int ret = -1;
    char buf[64] = {0};
    int size = 0;
    char *array = (char *)NULL;

    cJSON *info = information ? information->child : 0;

    memset(buf, 0, sizeof(buf));

    (void)snprintf(buf, 64, "Fantray%d", id);

    while (info)
    {
        char *tmp = cJSON_Print(info);

        if (tmp)
        {
            size = strlen(tmp) + 1;
            array = (char *)malloc(size);
            if (array)
            {
                memset(array, 0, size);
                (void)strncpy(array, tmp, strlen(tmp));
                if (!strstr(array, buf))
                {
                    
                    (void)free(array);
                    array = (char *)NULL;
                    (void)free(tmp);
                    tmp = (char *)NULL;

                    info = info->next;
                    continue;
                }

                char *token = (char *)NULL;
                char *p = (char *)NULL;
                char *ptr = array;
                token = strtok_r(ptr, ",", &p);
                while (token != NULL)
                {
                    token = strtok_r(NULL, ",", &p);
                    if (token)
                    {
                        char *item_p = strstr(token, item);
                        char *target = NULL;
                        if (item_p)
                        {
                            item_p = strtok_r(token, ":", &target);
                            if (item_p)
                            {
                                if (target)
                                {
                                    /* last char " delete */
                                    (void)strncpy(content, target, strlen(target) - 1);
                                    ret = 0;
                                }
                                else
                                {
                                    (void)strncpy(content, "Unkown", strlen("Unkown"));
                                    ret = 0;
                                }
                            }
                        }
                    }
                }
                if(ptr){
                    ptr = (char *)NULL;
                }
                if(p){
                    p = (char *)NULL;
                }
                (void)free(array);
                array = (char *)NULL;
            }

            (void)free(tmp);
            tmp = (char *)NULL;
        }

        info = info->next;
    }

    return ret;
}

int get_fan_board_md(int id, char *md)
{
    int ret = -1;
    cJSON *root = (cJSON *)NULL;
    char model[64] = {0};
    char subitem[32] = {0};

    if (id <= (FAN_COUNT))
    {
        uint8_t result = 0;
        char *tmp = NULL;
        int len = 0;

        if(is_shm_mem_ready()){
            result = phrase_json_buffer(ONLP_FAN_FRU_CACHE_SHARED, ONLP_FAN_FRU_CACHE_SEM, &tmp, &len);
            if(ret < 0 || !tmp){
                return ret;
                }
        }else{
            sprintf(command, "cat %s",ONLP_FAN_CACHE_FILE);
            tmp = read_tmp_cache(command,ONLP_FAN_CACHE_FILE);
        }
        if(result){
	        return ret;
	    }

        root = cJSON_Parse(tmp);
        if(!root){
            if(tmp){
                (void)free(tmp);
                tmp = (char *)NULL;
            }
            return ret;
        }

        cJSON *information = cJSON_GetObjectItem(root, "Information");
        if(!information){
            cJSON_Delete(root);
            if(tmp){
    	        (void)free(tmp);
                tmp = (char *)NULL;
            }
            return ret;
        }

        memset(subitem, 0, sizeof(subitem));
        (void)snprintf(subitem, 32, "Product Part Number");

        result = phrase_fan_array(information, id, subitem, model);
        if(result){
            ret = -1;
        }
        else{
   	    strncpy(md, model, strlen(model)); 
            ret = 0;
        }

        if(strlen(md)<=0){
            (void)snprintf(subitem, 32, "Board Part Number");

            result = phrase_fan_array(information, id, subitem, model);
            if(result){
                ret = -1;
            }
            else{
            strncpy(md, model, strlen(model)); 
                ret = 0;
            }
        }
    
	if(tmp){
    	(void)free(tmp);
	    tmp = (char *)NULL;
    }
    
	cJSON_Delete(root);
    }

    return ret;
}


int get_fan_board_sn(int id, char *sn)
{
    int ret = -1;
    cJSON *root = (cJSON *)NULL;
    char serial[64] = {0};

    if (id <= (FAN_COUNT))
    {
        uint8_t result = 0;
        char *tmp = NULL;
        int len = 0;

        if(is_shm_mem_ready()){
            result = phrase_json_buffer(ONLP_FAN_FRU_CACHE_SHARED, ONLP_FAN_FRU_CACHE_SEM, &tmp, &len);
            if(ret < 0 || !tmp){
                return ret;
                }
        }else{
            sprintf(command, "cat %s",ONLP_FAN_CACHE_FILE);
            tmp = read_tmp_cache(command,ONLP_FAN_CACHE_FILE);
        }
        if(result){
	        return ret;
	    }

        if(result){
            return ret;
        }

        root = cJSON_Parse(tmp);
        if(!root){
            if(tmp){
                (void)free(tmp);
                tmp = (char *)NULL;
            }
            return ret;
        }

        cJSON *information = cJSON_GetObjectItem(root, "Information");
        if(!information){
            cJSON_Delete(root);
            if(tmp){
                (void)free(tmp);
                tmp = (char *)NULL;
            }
            return ret;
        }

        char subitem[32] = {0};
        memset(subitem, 0, sizeof(subitem));
        (void)snprintf(subitem, 32, "Product Serial");

        result = phrase_fan_array(information, id, subitem, serial);
        if(result){
            ret = -1;
        }
        else{
            strncpy(sn, serial, strlen(serial));
            ret = 0;
        }

        if(strlen(sn)<=0){
            (void)snprintf(subitem, 32, "Board Serial");
            result = phrase_fan_array(information, id, subitem, serial);
            if(result){
                ret = -1;
            }
            else{
                strncpy(sn, serial, strlen(serial));
                ret = 0;
            }
        }
        

        if(tmp){
    	    (void)free(tmp);
	        tmp = (char *)NULL;
 	    }
    
        cJSON_Delete(root);
    }

    return ret;
}
int parse_psu_array(cJSON *information, int id, const char *item, char *content)
{
    int ret = -1;
    char buf[64] = {0};
    char buf2[64] = {0};
    char tmp[64] = {0};

    cJSON *info = information ? information->child : 0;

    memset(buf, 0, sizeof(buf));
    memset(buf, 0, sizeof(buf2));

    (void)snprintf(buf, 64, "PSU%d FRU", id);
    (void)snprintf(buf2, 64, "FRU Information");

    while(info)
    {
        cJSON *psu_ptr = cJSON_GetObjectItem(info, buf);
        cJSON *item_ptr = cJSON_GetObjectItem(info, item);
        if(psu_ptr == NULL){
            info = info->next;
        }else{
            if(item_ptr != NULL){
                (void)strncpy(content, item_ptr->valuestring, strlen(item_ptr->valuestring));
            }
            
            ret = 0;
            //return ret;
            break;
        }
    }

    //Use second method to check second FRU format for PSU.
    if (strlen(content) <= 0)
    {
        cJSON *info = information ? information->child : 0;

        while (info)
        {
            cJSON *psu_ptr2 = cJSON_GetObjectItem(info, buf2);
            cJSON *item_ptr2 = cJSON_GetObjectItem(info, item);
            if (psu_ptr2 != NULL)
            {
                if (item_ptr2 == NULL)
                {
                    content = "";
                }
                else
                {
                    memset(tmp, 0, sizeof(tmp)); 
                    (void)snprintf(tmp, 64, "PSU%d", id);
                    if(!strncmp(psu_ptr2->valuestring, tmp, strlen(tmp)))
                    {
                        (void)strncpy(content, item_ptr2->valuestring, strlen(item_ptr2->valuestring)); 	
                    }
                }

                ret = 0;

            }
            info = info->next;
        }
    }

    return ret;
}

int parse_rpm_array(cJSON *information, int id, const char *name, char *content)
{
    int ret = -1;
    char item[64] = {0};

    cJSON *info = information ? information->child : 0;

    memset(item, 0, sizeof(item));

    (void)snprintf(item, 64, "Fan %d Rear", id);

    while(info)
    {
        cJSON *name_ptr = cJSON_GetObjectItem(info, name);
        cJSON *item_ptr = cJSON_GetObjectItem(info, item);

        if(name_ptr && item_ptr){
            if(!strncmp(name_ptr->valuestring, "fancpld-i2c-8-0d", strlen("fancpld-i2c-8-0d"))){
	    	(void)strncpy(content, item_ptr->valuestring, strlen(item_ptr->valuestring));
		 ret = 0;
            	return ret;
	    } 
        }
        info = info->next;
    }

    return ret;
}

int get_psu_item_content(int id, char *item, char *content)
{
    int ret = -1;

    if ((id <= (PSU_COUNT) && id >= 0) && item && content)
    {
        uint8_t result = 0;
        char *tmp = NULL;
        int len = 0;
	    cJSON *root = (cJSON *)NULL;
        
        if(is_shm_mem_ready()){
            result = phrase_json_buffer(ONLP_PSU_FRU_CACHE_SHARED, ONLP_PSU_FRU_CACHE_SEM, &tmp, &len);
            if(ret < 0 || !tmp){
                return ret;
                }
        }else{
            sprintf(command, "cat %s",ONLP_PSU_CACHE_FILE);
            tmp = read_tmp_cache(command,ONLP_PSU_CACHE_FILE);
        }
        if(result){
	        return ret;
	    }

        if(result){
            return ret;
        }

        root = cJSON_Parse(tmp);
        if(!root){
            if(tmp){
                (void)free(tmp);
                tmp = (char *)NULL;
            }
            return ret;
	    }

        cJSON *information = cJSON_GetObjectItem(root, "Information");
        if(!information){
            if(tmp){
                (void)free(tmp);
                tmp = (char *)NULL;
            }

            if(tmp){
                (void)free(tmp);
                tmp = (char *)NULL;
            }
            cJSON_Delete(root);
            return ret;
        } 
        result = parse_psu_array(information, id, item, content); 
        cJSON_Delete(root);
        ret = result;

        if(tmp){
    	    (void)free(tmp);
	        tmp = (char *)NULL;
	    }
        return ret; 
    }

    return ret;
}

int get_rear_fan_rpm(int id, int *rpm)
{
     uint8_t result = 0;
     int ret = -1;
     char *tmp = NULL;
     int len = 0;
     cJSON *root = (cJSON *)NULL;
     char content[64] = {0}; 

    if(id < 0 || id > 5){
	    return ret;
    }

    //result = phrase_json_buffer(ONLP_SENSOR_FRU_CACHE_SHARED, ONLP_SENSOR_FRU_CACHE_SEM, &tmp, &len);

    if(is_shm_mem_ready()){
        result = phrase_json_buffer(ONLP_SENSOR_FRU_CACHE_SHARED, ONLP_SENSOR_FRU_CACHE_SEM, &tmp, &len);
        if(result < 0 || !tmp){
            return ret;
        }
    }else{
        sprintf(command, "cat %s",ONLP_SENSOR_CACHE_FILE);
        tmp = read_tmp_cache(command,ONLP_SENSOR_CACHE_FILE);
    }

    if(result){
        return ret;
    }

    root = cJSON_Parse(tmp);
    if(!root){
        if(tmp){
            (void)free(tmp);
	        tmp = (char *)NULL;
 	    }
        return ret;
    }

    cJSON *information = cJSON_GetObjectItem(root, "Information");
    if(!information){
        if(tmp){
    	    (void)free(tmp);
	        tmp = (char *)NULL;
 	    }
        cJSON_Delete(root);
        return ret;
    }

    result = parse_rpm_array(information, id, "name", content);
    if(result < 0){
        if(tmp){
    	    (void)free(tmp);
	        tmp = (char *)NULL;
	    }
        cJSON_Delete(root);
        return ret;
    }

    result = search_rpm_val(content, rpm);
    if(result) {
        if(tmp){
    	    (void)free(tmp);
	        tmp = (char *)NULL;
	    }
        cJSON_Delete(root);
        return ret;
    }

    if(tmp){
        (void)free(tmp);
        tmp = (char *)NULL;
	}
    cJSON_Delete(root);
    return ret;
}

#else
int get_fan_present_status(int id)
{
    int ret = -1;

    // if(id >= (FAN_COUNT+2)|| id<0)
    //     return -1;

    if (id <= (FAN_COUNT))
    {
        uint16_t fan_stat_reg;
        uint8_t result = 0;
        fan_stat_reg = fan_sys_reg[id].ctrl_sta_reg;
        result = read_fan_register(fan_stat_reg);
        ret = result;
        //ret = result;
        //printf("result result=%x ret=%d\n",result,ret);
    }

    return ret;
}
#endif
int get_rear_fan_pwm(int id, int *speed)
{
    //unsigned char data;
    unsigned short value;
    int ret = -1;
    uint8_t max_speed = 0xFF;
    //int max_rpm_speed = 13800;
    //int lowest_rpm_speed = 1200;
    int max_rpm_speed = 18000;
    int lowest_rpm_speed = 1000;
    float speed_percentage;

    if (id <= (FAN_COUNT))
    {
        uint16_t fan_stat_reg;
        // uint8_t result = 0;
        fan_stat_reg = fan_sys_reg[id].rear_spd_reg;
        value = read_register(fan_stat_reg);
        //printf("value=%x maxspeed=%x",value,max_rpm_speed);
        speed_percentage = (value * 100) / max_speed;
        *speed = (speed_percentage * ((max_rpm_speed - lowest_rpm_speed) / 100) + lowest_rpm_speed);
    }

    return ret;
}

int get_rear_fan_per(int id, int *speed)
{

    //unsigned char data;
    unsigned short value;
    int ret = -1;
    uint8_t max_speed = 0xFF;

    if (id <= (FAN_COUNT))
    {
        uint16_t fan_stat_reg;
        // uint8_t result = 0;
        fan_stat_reg = fan_sys_reg[id].rear_spd_reg;
        value = read_register(fan_stat_reg);
        //printf("value=%x maxspeed=%x",value,max_speed);
        *speed = (value * 100) / max_speed;
    }

    return ret;
}

int fanSpeedSet(int id, unsigned short speed)
{

    int result;

    if (id <= (FAN_COUNT - 1))
    {
        uint16_t fan_reg = fan_sys_reg[id].pwm_reg;
        result = write_register(fan_reg, speed);
    }

    return result;
}

uint8_t get_psu_status(int id)
{

    uint8_t ret = 0xFF;

    if (id >= (PSU_COUNT + 2) || id < 0)
        return 0xFF;

    if (id <= (PSU_COUNT))
    {
        //uint16_t fan_stat_reg;
        uint8_t result = 0;
        //fan_stat_reg  = fan_sys_reg[id].ctrl_sta_reg;
        result = read_register(PSU_REGISTER);
        //ret = (result >> 2) & 0x1;
        ret = result;
        //printf("result result=%x ret=%d\n",result,ret);
    }

    return ret;
}

#define MAX_LENTH_OF_LINE 256
static int set_pos_by_line(FILE *fp, int line)
{
    int i = 0;
    char buffer[MAX_LENTH_OF_LINE + 1];
    fpos_t pos;
 
    rewind(fp);
    for (; i < line; i++)
	fgets(buffer, MAX_LENTH_OF_LINE, fp);

    fgetpos(fp, &pos);
    return 0;
}

/*
{
    "result": [
        "0x0",
        "",
        "Note:",
        "0x0: disable",
        "0x1: enable",
        "Bit[0:0] @ register 0x61, register value 0x0",
        ""
    ]
}
*/

/*
{
    "result": [
        "0x0",
        "",
        "Note:",
        "0x0: disable",
        "0x1: enable",
        "",
        "Bit[0:0] @ register 0x63, register value 0x0",
        ""
    ]
}
*/
#define LED_COLOR_KEY_STR 2 
static int read_by_line(const char *path, uint8_t *color)
{
    char buffer[MAX_LENTH_OF_LINE + 1];
    FILE *fp = (FILE *)NULL;

    fp = fopen(path, "r");
    if(!fp){
	return -1;
    }

    (void)set_pos_by_line(fp, LED_COLOR_KEY_STR);
    (void)fgets(buffer, MAX_LENTH_OF_LINE, fp);

    if(strstr(buffer, "0x0")){
  	/* green */
       *color = LED_GREEN_ENUM;
    }else{
	/* yellow */
        *color = LED_YELLOW_ENUM;
    }

    fclose(fp);
    
    return 0;
}

uint8_t get_led_color(const char *path, uint8_t *color)
{
    if(!path || !color){
	return 0xFF;
    }

    if(read_by_line(path, color)){
	return 0xFF;
    }    
    return 0;
}

uint8_t get_led_status(int id)
{
    uint8_t ret = 0xFF;
    //uint8_t color = 0;
    if (id > LED_COUNT || id < 0)
        return 0xFF;

    if (id <= LED_COUNT)
    {
        uint8_t result = 0;
        uint16_t led_stat_reg;
        
        led_stat_reg = led_mapper[id].dev_reg;
	    result = read_register(led_stat_reg);
        ret = result;
    }

    return ret;
}

#ifndef BMC_RESTFUL_API_SUPPORT 
uint8_t get_fan_led_status(int id)
{

    //     {"LED_SYSTEM",LED_SYSTEM,LED_SYSTEM_REGISTER},
    // {"LED_FAN_1",LED_FAN_1,0xA141},
    // {"LED_FAN_2",LED_FAN_2,0xA145},
    // {"LED_FAN_4",LED_FAN_4,0xA14D},
    // {"LED_FAN_5",LED_FAN_5,0xA151},
    // {"LED_ALARM",LED_ALARM,ALARM_REGISTER},
    // {"LED_PSU_1",LED_PSU_1,PSU_LED_REGISTER},
    // {"LED_PSU_2",LED_PSU_2,PSU_LED_REGISTER},
    uint8_t ret = 0xFF;

    if (id >= (LED_COUNT + 2) || id < 0)
        return 0xFF;

    if (id <= (LED_COUNT))
    {
        uint8_t result = 0;
        uint16_t led_stat_reg;

        led_stat_reg = led_mapper[id].dev_reg;
        result = read_fan_register(led_stat_reg);
        
        ret = result;
        //printf("result result=%x ret=%d\n",result,ret);
    }

    return ret;
}
#else
uint8_t get_fan_led_status(int id)
{
    uint8_t ret = 0xFF;

    if (id > LED_COUNT || id < 0)
        return 0xFF;
    
    if (id <= LED_COUNT){
	    uint8_t result = 0;
        char *tmp = NULL;
        cJSON *present = (cJSON *)NULL;
        int len = 0;

        if(is_shm_mem_ready()){
            result = phrase_json_buffer(ONLP_STATUS_FRU_CACHE_SHARED, ONLP_STATUS_FRU_CACHE_SEM, &tmp, &len);
            if(ret < 0 || !tmp){
                return ret;
                }
        }else{
            sprintf(command, "cat %s",ONLP_STATUS_CACHE_FILE);
            tmp = read_tmp_cache(command,ONLP_STATUS_CACHE_FILE);
        }
        
        char subitem[8] = {0};
        memset(subitem, 0, sizeof(subitem));
        (void)snprintf(subitem, 8, "Fan%d", id);
 
        result += phrase_json_key_word(tmp, "Information", subitem, id - 1, &present);
        if(!present){
           ret = 0xFF;
           printf("\nNothing\n");
        }
        else{
            if(!strncmp(present->valuestring, " Present", strlen(" Present"))){
                ret = 0;
            }
            else{
                ret = -1;
                printf("\nMismatch\n");
            }
            free(present->valuestring);
            free(present);
        }

        if(tmp){
    	    (void)free(tmp);
	        tmp = (char *)NULL;
	    }
	ret = result;
    }

    return ret; 
}
#endif
uint8_t getThermalStatus(int id)
{

    uint8_t ret = 0;
    // if(id >= (THERMAL_COUNT)|| id<0)
    //     return 0;

    if (id <= (THERMAL_COUNT - 1))
    {
        uint8_t result = 0;
        result = read_register(THERMAL_REGISTER);
        ret = result;
        //printf("result = %x , %d\n",ret,ret);
    }

    return ret;
}

int led_mask(int start, uint8_t value)
{

    int twobits_mask = 3;

    int cal = ((value >> start) & twobits_mask);

    return cal;
}
char *read_fru(int fru_id)
{
    FILE *pFd = NULL;
    char c; //,s_id[4]; //stat[5000],
    char *str = (char *)malloc(sizeof(char) * 5000);
    int i = 0;
    //float marks;
    //sprintf(command,"cat /home/tdcadmin/Work/Git/seastone2-diag/onl-sysinfo/fru.txt");
    //sprintf(s_id,"%d",id);

    sprintf(command, "ipmitool fru print %d", fru_id);
    pFd = popen(command, "r");
    if (pFd != NULL)
    {

        c = fgetc(pFd);
        while (c != EOF)
        {
            //printf ("%c", c);
            str[i] = c;
            i++;
            c = fgetc(pFd);
        }

        pclose(pFd);
    }
    else
    {
        printf("execute command %s failed\n", command);
    }

    return str;
}

char *read_psu_sdr(int id)
{
    FILE *pFd = NULL;
    char c; //,s_id[4]; //stat[5000],
    char *str = (char *)malloc(sizeof(char) * 5000);
    int i = 0;

    sprintf(command,"ipmitool  sdr | grep PSU");
    pFd = popen(command, "r");
    if (pFd != NULL)
    {

        c = fgetc(pFd);
        while (c != EOF)
        {
            //printf ("%c", c);
            str[i] = c;
            i++;
            c = fgetc(pFd);
        }

        pclose(pFd);
    }
    else
    {
        printf("execute command %s failed\n", command);
    }

    return str;
}

char *read_ipmi(char *cmd)
{
    FILE *pFd = NULL;
    char c; //,s_id[4]; //stat[5000],
    char *str = (char *)malloc(sizeof(char) * 5000);
    int i = 0;
    //float marks;
    //sprintf(command,"cat /home/tdcadmin/Work/Git/seastone2-diag/onl-sysinfo/fru.txt");
    //sprintf(s_id,"%d",id);
    //sprintf(command);
    pFd = popen(cmd, "r");
    if (pFd != NULL)
    {

        c = fgetc(pFd);
        while (c != EOF)
        {
            //printf ("%c", c);
            str[i] = c;
            i++;
            c = fgetc(pFd);
        }

        pclose(pFd);
    }
    else
    {
        printf("execute command %s failed\n", command);
    }

    return str;
}

int psu_get_info(int id, int *mvin, int *mvout, int *mpin, int *mpout, int *miin, int *miout)
{

    //char *sdr_value = NULL;
    int position = 0, val_pos = 0;
    if(sdr_value == NULL){
        sdr_value = read_psu_sdr(id);
    }
        
    
    char ctemp[18] = "\0";
    float ftemp = 0.0;
    int str_index = 0;
    if(id == 2)
        str_index = 6;

    int val[12]={0,0,0,0,0,0,0,0,0,0,0,0};

    for (; str_index < NELEMS(search_psu_sdr_info); str_index++)
    {

        position = keyword_match(sdr_value, search_psu_sdr_info[str_index].keyword);

        if (position != -1)
        {
            int status_pos = position + 39;
            //printf("Status = 0%c1%c2%c\n",sdr_value[status_pos],sdr_value[status_pos+1],sdr_value[status_pos+2]);
            if (sdr_value[status_pos] != 'n')
            {
                int v_pos = position + 19;
                int a = 0;
                //printf("Map the %s with %c\n",search_psu_sdr_info[str_index].keyword,search_psu_sdr_info[str_index].unit);
                for (; a <= 18; a++)
                {
                    //printf("%c",sdr_value[v_pos+a]);
                    // 			//if(search_psu_sdr_info[str_index].unit!=NULL){
                    //                 printf("%c",sdr_value[v_pos+a]);
                    if (sdr_value[v_pos + a] != search_psu_sdr_info[str_index].unit)
                    {
                        // ctemp[a]=sdr_value[v_pos+a];
                        append(ctemp, sdr_value[v_pos + a]);
                        //printf("%c",sdr_value[v_pos+a]);
                    }
                    else
                    {
                        //printf("ctemp = %s\n",ctemp);
                        ftemp = atof(ctemp);
                        //printf("ftemp %f\n",ftemp);
                        val[val_pos] = ftemp * 1000;
                        //printf("val[%d] = %d %s\n",val_pos,val[val_pos],search_psu_sdr_info[str_index].keyword);
                        memset(ctemp, 0, sizeof(ctemp));
                        val_pos++;
                        break;
                    }
                }
            }
        }
    }

    *mvin = val[0];
    *miin = val[1];
    *mpin = val[2];
    *mvout = val[3];
    *miout = val[4];
    *mpout = val[5];

    return 1;
}

int psu_get_model_sn(int id, char *model, char *serial_number)
{
    //char buf[256];
    //int index;
    char *token = NULL;
    cJSON *json = NULL;
    cJSON *items = NULL;

    if (0 == strcasecmp(psu_information[0].model, "unknown")) {
        
        long len = 0;
        char *data = NULL;
        fp = fopen(PSU_CACHE_PATH, "rb");

        /* get the length */
        fseek(fp, 0, SEEK_END);
        len = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        data = (char*)malloc(len + 1);

        fread(data, 1, len, fp);
        data[len] = '\0';
        fclose(fp);

        json = cJSON_Parse(data);
        
        if (!json)
        {
            printf("Error before: [%s]\n", cJSON_GetErrorPtr());
        }
        else
        {
            
            items = cJSON_GetObjectItem(json, "informations");
            token = cJSON_Print(items);
            int i = 0;
            for (; i < PSU_COUNT; i++) {  // Presumably "max" can be derived from "items" somehow
                // char *psu_name = NULL;
                // cJSON *model = NULL;
                // cJSON *sn = NULL;
                // cJSON* item = cJSON_GetArrayItem(items, i);
                // sprintf(psu_name,"PSU%d FRU",i+1);
                // //psu_name = cJSON_GetObjectItem(item, psu_name);
                // model = cJSON_GetObjectItem(item, "Product Name");
                // sn = cJSON_GetObjectItem(item, "Serial Number");
                // char* rendered1 = cJSON_Print(model);
                // char* rendered2 = cJSON_Print(sn);
                // printf("%s %s %s",psu_name,rendered1,rendered2);
                // free(psu_name);
                // cJSON_Delete(model);
                // cJSON_Delete(sn);
            }
            printf("%s\n", token);
            free(token);

            cJSON_Delete(json);
            cJSON_Delete(items);
        }

        free(data);
        //pclose(fp);
        
    }

    strcpy(model, psu_information[id].model);
    strcpy(serial_number, psu_information[id].serial_number);

    return 1;
}

void append(char *s, char c)
{
    int len = strlen(s);
    s[len] = c;
    s[len + 1] = '\0';
}

int keyword_match(char *a, char *b)
{

    int position = 0;
    char *x, *y;

    x = a;
    y = b;

    while (*a)
    {
        while (*x == *y)
        {
            x++;
            y++;
            if (*x == '\0' || *y == '\0')
                break;
        }
        if (*y == '\0')
            break;

        a++;
        position++;
        x = a;
        y = b;
    }
    if (*a)
        return position;
    else
        return -1;
}

char* read_fans_fru(){

    FILE *pFd = NULL;
    char c; //,s_id[4]; //stat[5000],
    char *str = (char *)malloc(sizeof(char) * 5000);
    int i = 0;

    sprintf(command, "ipmitool fru print | grep -A 4 FRU_FAN");
    pFd = popen(command, "r");
    if (pFd != NULL)
    {

        c = fgetc(pFd);
        while (c != EOF)
        {
            //printf ("%c", c);
            str[i] = c;
            i++;
            c = fgetc(pFd);
        }

        pclose(pFd);
    }
    else
    {
        printf("execute command %s failed\n", command);
    }

    return str;
}

int getFaninfo(int id, char *model, char *serial)
{
    char buf[256];
    int index;
    char *token;

    if (0 == strcasecmp(fan_information[0].model, "unknown")) {
        
        index = 0;
        sprintf(command, "ipmitool fru print | grep -A 10 FRU_FAN");
        fp = popen(command, "r");
        if (fp == 0)
        {
            printf("Failed - Failed to obtain system information\n");
            return -1;
        }
        while (EOF != fscanf(fp, "%[^\n]\n", buf))
        {
            if (strstr(buf, "FRU Device Description")) {
                index++;
            }
            if (strstr(buf, "Board Serial")) {
                token = strtok(buf, ":");
                token = strtok(NULL, ":");
                char* trim_token = trim(token);
                sprintf(fan_information[index].serial_number,"%s",trim_token);
            }
            else if (strstr(buf, "Board Part Number")) {
                token = strtok(buf, ":");
                token = strtok(NULL, ":");
                char* trim_token = trim(token);
                sprintf(fan_information[index].model,"%s",trim_token);
            
            }
        }
        sprintf(fan_information[0].model,"pass"); //Mark as complete
        pclose(fp);
        
    }

    strcpy(model, fan_information[id].model);
    strcpy(serial, fan_information[id].serial_number);

    return 1;
}

char *read_temp(char *name)
{
    FILE *pFd = NULL;
    char c; //,s_id[4]; //stat[5000],
    char *str = (char *)malloc(sizeof(char) * 5000);
    int i = 0;
    //float marks;
    //sprintf(command,"cat /home/tdcadmin/Work/Git/seastone2-diag/onl-sysinfo/fru.txt");
    //sprintf(s_id,"%d",id);

    sprintf(command, "ipmitool sensor | grep %s ", name);
    pFd = popen(command, "r");
    if (pFd != NULL)
    {

        c = fgetc(pFd);
        while (c != EOF)
        {
            //printf ("%c", c);
            str[i] = c;
            i++;
            c = fgetc(pFd);
        }

        pclose(pFd);
    }
    else
    {
        printf("execute command %s failed\n", command);
    }

    return str;
}

#ifndef BMC_RESTFUL_API_SUPPORT
int getThermalStatus_Ipmi(int id, int *tempc)
{
    //printf("desc = %s\n",desc);
    char data_temp[500] = "\0";
    int position;
    char ctemp[18] = "\0";
    float ftemp = 0.0;
    if(temp_sdr_value == NULL){
        sprintf(command, "ipmitool sdr list | grep Temp");
        temp_sdr_value = read_ipmi(command);
    }
    position = keyword_match(temp_sdr_value, Thermal_sensor_name[id - 1]);
    if (position != -1){
        //Grep word untill newline
        //printf("Position = %d | text = %c%c",position,temp_sdr_value[position],temp_sdr_value[position+1]);
        int search_pos = 0;
        while(temp_sdr_value[position+search_pos] != '\n'){
            append(data_temp,temp_sdr_value[position+search_pos]);
            search_pos++;
        }
    }

    if (keyword_match(data_temp, "ns") > 0)
    {
        if( id >= THERMAL_COUNT){
            free(temp_sdr_value);
            temp_sdr_value=NULL;
        }
        return 0;
    }
    else
    {
        int response_len = strlen(data_temp);
        int i = 18;
        for (; i < response_len; i++)
        {
            if (data_temp[i] == 'c')
                break;
            append(ctemp, data_temp[i]);
        }
        ftemp = atof(ctemp);
        *tempc = ftemp * 1000;
    }
    
    //Free malloc temp_sdr_value after use.
    if( id >= THERMAL_COUNT){
        free(temp_sdr_value);
        temp_sdr_value=NULL;
    }

    return 1;
}
#else
//#define CPU_CORE_TEMPERATURE "/sys/class/thermal/thermal_zone0/temp"
int read_cpu_temp(int *temperature)
{
    int res = -1;
    FILE *fp_thermal = (FILE *)NULL;
    char buf[64] = {0};
    int temp = 0;
    (void)snprintf(buf, 64, "%s %s","cat", CPU_CORE_TEMPERATURE);

    fp_thermal = popen(buf, "r");
    if(!fp_thermal){
        return res;
    }

    fscanf(fp_thermal, "%d", &temp);
    pclose(fp_thermal);
    *temperature = temp;
    res = 0;
    return res;
}
int search_temp_val(char *tempc, int *tempi)
{
    int ret = -1;
    char *digit = NULL;
    char *tmp = NULL; 
    int size = 0;
    if(!tempc || !tempi){
  	return ret;
    }
  
    tmp = strchr(tempc, '+');
    if(!tmp){
	return ret;
    }
    digit = (tmp + 1);
    tmp = digit;
    char *c = strchr(digit, 'C');
    if(!c){
	return ret;
    }  

    size = c - digit;

    char *str = malloc(size + 1);
    if (!str){
	return ret; 
    }

    memset(str, 0, size + 1);
    memcpy(str, digit, size);

    *tempi = atof(str)*1000;

    free(str);
    ret = 0;
    return ret;
}

int search_current_val(char *amper_c, int *amper_i)
{
    int ret = -1;
    char *digit = NULL;
    char *tmp = NULL;
    int size = 0;
    if(!amper_c || !amper_i){
        return ret;
    }

    tmp = strchr(amper_c, '+');
    if(!tmp){
        return ret;
    }
    digit = (tmp + 1);
    tmp = digit;
    char *c = strchr(digit, 'A');
    if(!c){
        return ret;
    }

    size = c - digit;

    char *str = malloc(size + 1);
    if (!str){
        return ret;
    }

    memset(str, 0, size + 1);
    memcpy(str, digit, size);

    *amper_i = atof(str)*1000;

    free(str);
    ret = 0;
    return ret;
}

int search_voltage_val(char *volt_c, int *volt_i)
{
    int ret = -1;
    char *digit = NULL;
    char *tmp = NULL;
    int size = 0;
    if(!volt_c || !volt_i){
        return ret;
    }

    tmp = strchr(volt_c, '+');
    if(!tmp){
        return ret;
    }
    digit = (tmp + 1);
    tmp = digit;
    char *c = strchr(digit, 'V');
    if(!c){
        return ret;
    }

    size = c - digit;

    char *str = malloc(size + 1);
    if (!str){
        return ret;
    }

    memset(str, 0, size + 1);
    memcpy(str, digit, size);

    *volt_i = atof(str)*1000;

    free(str);
    ret = 0;
    return ret;
}

int search_power_val(char *watt_c, int *watt_i)
{
    int ret = -1;
    char *digit = NULL;
    char *tmp = NULL;
    int size = 0;
    if(!watt_c || !watt_i){
        return ret;
    }

    tmp = watt_c;
    while(!isdigit(*tmp))
    {
	if(*tmp == 'W'){
	    break;
	}

	tmp++;
    }   
 
    digit = tmp;
    char *c = strchr(digit, 'W');
    if(!c){
        return ret;
    }

    size = c - digit;

    char *str = malloc(size + 1);
    if (!str){
        return ret;
    }

    memset(str, 0, size + 1);
    memcpy(str, digit, size);

    *watt_i = atof(str)*1000;

    free(str);
    ret = 0;
    return ret;
}

int search_rpm_val(char *rpm_c, int *rpm_i)
{
    int ret = -1;
    char *digit = NULL;
    char *tmp = NULL;
    int size = 0;
    if(!rpm_c || !rpm_i){
        return ret;
    }

    tmp = rpm_c;
    while(!isdigit(*tmp))
    {
	/* RPM*/
        if(*tmp == 'R'){
            break;
        }

        tmp++;
    }

    digit = tmp;
    char *c = strchr(digit, 'R');
    if(!c){
        return ret;
    }

    size = c - digit;

    char *str = malloc(size + 1);
    if (!str){
        return ret;
    }

    memset(str, 0, size + 1);
    memcpy(str, digit, size);

    *rpm_i = atoi(str);

    free(str);
    ret = 0;
    return ret;
}

int parse_sensor_array(cJSON *information, const char *adap_cont, const char *name_cont, const char *item_name, char *item_cont)
{
    int ret = -1;
  
    cJSON *info = information ? information->child : 0;

    while(info)
    {
        //cJSON *adap_ptr = cJSON_GetObjectItem(info, "Adapter");
        cJSON *name_ptr = cJSON_GetObjectItem(info, "name");
        cJSON *item_ptr = cJSON_GetObjectItem(info, item_name); 
        if(/*adap_ptr && */name_ptr && item_ptr){
 	    if(/*!strncmp(adap_ptr->valuestring, adap_cont, strlen(adap_cont)) && */
 	    !strncmp(name_ptr->valuestring, name_cont, strlen(name_cont))){
	 	(void)strncpy(item_cont, item_ptr->valuestring, strlen(item_ptr->valuestring));
	        ret = 0; 
		return ret;	
	    }
         	
        } 

 	info = info->next;
    } 

    return ret;
}

int read_sensor_temp(const char *adapter, const char *name, const char *item, int *temp)
{
    int ret = -1;
    int len = 0;
    char *tmp = (char *)NULL; 
    char content[64] = {0};
    cJSON *root = (cJSON *)NULL; 

    if (/*!adapter ||*/ !name || !item || !temp){
	return ret;
    }  

    if(is_shm_mem_ready()){
        ret = phrase_json_buffer(ONLP_SENSOR_FRU_CACHE_SHARED, ONLP_SENSOR_FRU_CACHE_SEM, &tmp, &len);
        if(ret < 0 || !tmp){
            return ret;
        }
    }else{
        sprintf(command, "cat %s",ONLP_SENSOR_CACHE_FILE);
        tmp = read_tmp_cache(command,ONLP_SENSOR_CACHE_FILE);
        ret = 1;
    }
    
    if(ret < 0){
        if(tmp){
    	    (void)free(tmp);
	        tmp = (char *)NULL;
	    }
	    return ret;
    }

    root = cJSON_Parse(tmp);
    if(!root){
        ret = -1;
        if(tmp){
            (void)free(tmp);
            tmp = (char *)NULL;
        }
        return ret;
    }

    cJSON *information = cJSON_GetObjectItem(root, "Information");
    if(!information){
        cJSON_Delete(root);
	    ret = -1;

        if(tmp){
    	    (void)free(tmp);
	        tmp = (char *)NULL;
	    }
        return ret;
    }

    ret = parse_sensor_array(information, adapter, name, item, content);
    ret += search_temp_val(content, temp);
    cJSON_Delete(root);
    if(tmp){
    	(void)free(tmp);
	    tmp = (char *)NULL;
    }
    return ret;
}

int read_psu_inout(const char *adapter, const char *name, const char *item, char *content)
{
    int ret = -1;
    int len = 0;
    char *tmp = (char *)NULL;
    cJSON *root = (cJSON *)NULL;

    if (!name || !item || !content){
        return ret;
    }

    if(is_shm_mem_ready()){
        ret = phrase_json_buffer(ONLP_SENSOR_FRU_CACHE_SHARED, ONLP_SENSOR_FRU_CACHE_SEM, &tmp, &len);
        if(ret < 0 || !tmp){
            return ret;
        }
    }else{
        sprintf(command, "cat %s",ONLP_SENSOR_CACHE_FILE);
        tmp = read_tmp_cache(command,ONLP_SENSOR_CACHE_FILE);
    }
    

    root = cJSON_Parse(tmp);
    if(!root){
        ret = -1;
        if(tmp){
    	    (void)free(tmp);
	        tmp = (char *)NULL;
	    }	
        return ret;
    }

    cJSON *information = cJSON_GetObjectItem(root, "Information");
    if(!information){
        cJSON_Delete(root);
        ret = -1;
        if(tmp){
    	    (void)free(tmp);
	        tmp = (char *)NULL;
	    }
        return ret;
    }

    ret = parse_sensor_array(information, adapter, name, item, content);
    cJSON_Delete(root);
    if(tmp){
    	(void)free(tmp);
	    tmp = (char *)NULL;
    }
    return ret;
}

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

int getThermalStatus_Ipmi(int id, int *tempc)
{
    int res = -1;
    char thermal_name[32] = {0};

    switch(id)
    {
	case THERMAL_SWITCH_REMOTE_U148:
	    (void)strncpy(thermal_name, SWITCH_REMOTE_U148_NAME, strlen(SWITCH_REMOTE_U148_NAME));
	    break;
	case THERMAL_BASE_R_INLET_TEMP_U41:
	    (void)strncpy(thermal_name, BASE_R_INLET_TEMP_U41_NAME, strlen(BASE_R_INLET_TEMP_U41_NAME));
	    break;
	case THERMAL_BASE_C_INLET_TEMP_U3:
	    (void)strncpy(thermal_name, BASE_C_INLET_TEMP_U3_NAME, strlen(BASE_C_INLET_TEMP_U3_NAME));
	    break;
	case THERMAL_SWITCH_OUTLET_TEMP_U33:
	    (void)strncpy(thermal_name, SWITCH_OUTLET_TEMP_U33_NAME, strlen(SWITCH_OUTLET_TEMP_U33_NAME));
	    break;
	case THERMAL_PSU_INLET_L_TEMP_U8:
	    (void)strncpy(thermal_name, PSU_INLET_L_TEMP_U8_NAME, strlen(PSU_INLET_L_TEMP_U8_NAME));
 	    break;
	case THERMAL_PSU_INLET_R_TEMP_U10:
	    (void)strncpy(thermal_name, PSU_INLET_R_TEMP_U10_NAME, strlen(PSU_INLET_R_TEMP_U10_NAME));
	    break;
	case THERMAL_FAN_L_TEMP_U8:
	    (void)strncpy(thermal_name, FAN_L_TEMP_U8_NAME, strlen(FAN_L_TEMP_U8_NAME));
	    break;
	case THERMAL_FAN_R_TEMP_U10:
	    (void)strncpy(thermal_name, FAN_R_TEMP_U10_NAME, strlen(FAN_R_TEMP_U10_NAME));
	    break;
	case THERMAL_RT_LINC_TEMP_U26:
	    (void)strncpy(thermal_name, RT_LINC_TEMP_U26_NAME, strlen(RT_LINC_TEMP_U26_NAME));
	    break;
	case THERMAL_LT_LINC_TEMP_U25:
	    (void)strncpy(thermal_name, LT_LINC_TEMP_U25_NAME, strlen(LT_LINC_TEMP_U25_NAME));
	    break;
	case THERMAL_RB_LINC_TEMP_U26:
	    (void)strncpy(thermal_name, RB_LINC_TEMP_U26_NAME, strlen(RB_LINC_TEMP_U26_NAME));
	    break;
	case THERMAL_LB_LINC_TEMP_U25:
	    (void)strncpy(thermal_name, LB_LINC_TEMP_U25_NAME, strlen(LB_LINC_TEMP_U25_NAME));
	    break;
	default:
	    return res;
    }

    if(id == THERMAL_SWITCH_REMOTE_U148){
	res = read_sensor_temp(NULL, thermal_name, "Local temp",tempc);
    }
    else{
        res = read_sensor_temp(NULL, thermal_name, "temp1",tempc);
    }
    return res;
}
#endif
int deviceNodeReadBinary(char *filename, char *buffer, int buf_size, int data_len)
{
    int fd;
    int len;

    if ((buffer == NULL) || (buf_size < 0))
    {
        return -1;
    }

    if ((fd = open(filename, O_RDONLY)) == -1)
    {
        return -1;
    }

    if ((len = read(fd, buffer, buf_size)) < 0)
    {
        close(fd);
        return -1;
    }

    if ((close(fd) == -1))
    {
        return -1;
    }

    if ((len > buf_size) || (data_len != 0 && len != data_len))
    {
        return -1;
    }

    return 0;
}

int deviceNodeReadString(char *filename, char *buffer, int buf_size, int data_len)
{
    int ret;

    if (data_len >= buf_size)
    {
        return -1;
    }

    ret = deviceNodeReadBinary(filename, buffer, buf_size - 1, data_len);
    //printf("deviceNodeReadBinary = ret %d buffer = %s\n",ret,buffer);
    if (ret == 0)
    {
        buffer[buf_size - 1] = '\0';
    }

    return ret;
}

char* trim (char *s)
{
    int i;

    while (isspace (*s)) s++;   // skip left side white spaces
    for (i = strlen (s) - 1; (isspace (s[i])); i--) ;   // skip right side white spaces
    s[i + 1] = '\0';
    //printf ("%s\n", s);
    return s;
}
