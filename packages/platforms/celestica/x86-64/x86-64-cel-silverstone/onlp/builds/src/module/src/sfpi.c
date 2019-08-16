/************************************************************
 * <bsn.cl fy=2014 v=onl>
 *
 *        Copyright 2014, 2015 Big Switch Networks, Inc.
 *
 * Licensed under the Eclipse Public License, Version 1.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *        http://www.eclipse.org/legal/epl-v10.html
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the
 * License.
 *
 * </bsn.cl>
 ************************************************************
 *
 *
 ********************************************************** */
#include <onlp/platformi/sfpi.h>
#include <x86_64_cel_silverstone/x86_64_cel_silverstone_config.h>
#include "x86_64_cel_silverstone_log.h"
#include "platform.h"

static int qsfp_count__ = 32;
static int sfp_count__ = 2;
static int i2c_bus_offset = 9;
static char node_path[PREFIX_PATH_LEN] = {0};
char command[256];
char buf[256];
FILE *fp;



static int 
cel_silverstone_qsfp_sfp_node_read_int(char *path, int *value, int data_len)
{
    int ret = 0;
    char buf[8];    
    *value = 0;

    ret = deviceNodeReadString(path, buf, sizeof(buf), data_len);
    if (ret == 0) {
        //port present
        //*value = atoi(buf);
        int is_not_present = atoi(buf);
        if(!is_not_present){
            *value = !is_not_present;
        }
        
    }
    return ret;
}

static char* 
cel_silverstone_sfp_qsfp_get_port_path(int port, char *node_name)
{

    if(port <= qsfp_count__ + sfp_count__){
        if(port<=qsfp_count__){
            //sprintf(node_path, "%sSFF/QSFP%d/qsfp_modprs", PLATFORM_PATH, port); 
            sprintf(node_path, "%s/QSFP%d/qsfp_modprsL", PLATFORM_PATH, port);
        }else{
            //sprintf(node_path, "%sSFF/SFP%d/sfp_modabs", PLATFORM_PATH, port-qsfp_count__);
            sprintf(node_path, "%s/SFP%d/sfp_modabs", PLATFORM_PATH, port-qsfp_count__);
        }
    }else{
        AIM_LOG_ERROR("Number of port config is mismatch port(%d)\r\n", port);
        return "";
    }

    return node_path;
}

static char* 
cel_silverstone_sfp_qsfp_get_eeprom_path(int port, char *node_name)
{

    if(port <= qsfp_count__ + sfp_count__){
        if(port<=qsfp_count__){
            //sprintf(node_path, "%sSFF/QSFP%d/i2c/eeprom", PLATFORM_PATH, port);
            sprintf(node_path, "%s/%d-0050/eeprom", I2C_DEVICE_PATH, port+i2c_bus_offset); //QSFP 10 - 41
        }else{
            //sprintf(node_path, "%sSFF/SFP%d/i2c/eeprom", PLATFORM_PATH, port-qsfp_count__);
            sprintf(node_path, "%s/%d-0050/eeprom", I2C_DEVICE_PATH, port-qsfp_count__); //SFP 1 - 2
        }
    }else{
        AIM_LOG_ERROR("Number of port config is mismatch port(%d)\r\n", port);
        return "";
    }

    return node_path;
}

static uint64_t
cel_silverstone_sfp_qsfp_get_all_ports_present(void)
{
	int i, ret;
	uint64_t present = 0;
    char* path;

	for(i = 0; i < (qsfp_count__+sfp_count__); i++) {
		path = cel_silverstone_sfp_qsfp_get_port_path(i + 1, "present");
	    if (cel_silverstone_qsfp_sfp_node_read_int(path, &ret, 0) != 0) {
	        ret = 0;
	    }
		present |= ((uint64_t)ret << i);
	}

    return present;
}

int
onlp_sfpi_init(void)
{
    return ONLP_STATUS_OK;
}

int
onlp_sfpi_bitmap_get(onlp_sfp_bitmap_t* bmap)
{

    int p;
    AIM_BITMAP_CLR_ALL(bmap);
    
    for(p = 0; p < (qsfp_count__+sfp_count__); p++) {
        AIM_BITMAP_SET(bmap, p);
    }

    return ONLP_STATUS_OK;
}

/*
* Return 1 if present.
* Return 0 if not present.
* Return < 0 if error.
*/
int
onlp_sfpi_is_present(int port)
{
    int present;
    char* path = cel_silverstone_sfp_qsfp_get_port_path(port + 1, "present");
    if (cel_silverstone_qsfp_sfp_node_read_int(path, &present, 0) != 0) {
        if(port <= qsfp_count__){
            AIM_LOG_ERROR("Unable to read present status from qsfp port(%d)\r\n", port);
        }else{
            AIM_LOG_ERROR("Unable to read present status from sfp port(%d)\r\n", port-qsfp_count__);
        }
        
        return ONLP_STATUS_E_INTERNAL;
    }
    return present;
}

int
onlp_sfpi_presence_bitmap_get(onlp_sfp_bitmap_t* dst)
{
    //AIM_BITMAP_CLR_ALL(dst);
    int i = 0;
    uint64_t presence_all = 0;

	presence_all = cel_silverstone_sfp_qsfp_get_all_ports_present();

    /* Populate bitmap */
    for(i = 0; presence_all; i++) {
        AIM_BITMAP_MOD(dst, i, (presence_all & 1));
        presence_all >>= 1;
    }
    return ONLP_STATUS_OK;
}

/*
 * This function reads the SFPs idrom and returns in
 * in the data buffer provided.
 */
int
onlp_sfpi_eeprom_read(int port, uint8_t data[256])
{

    char* path;

	//sprintf(sub_path, "/%d-0050/eeprom", CHASSIS_SFP_I2C_BUS_BASE + port);
	path= cel_silverstone_sfp_qsfp_get_eeprom_path(port + 1, "eeprom");

    /*
     * Read the SFP eeprom into data[]
     *
     * Return MISSING if SFP is missing.
     * Return OK if eeprom is read
     */
    memset(data, 0, 256);
    
    if (deviceNodeReadBinary(path, (char*)data, 256, 256) != 0) {
        AIM_LOG_ERROR("Unable to read eeprom from port(%d)\r\n", port);
        return ONLP_STATUS_E_INTERNAL;
    }
    
    // switch (port)
    // {
    // case 0:{
    //     uint8_t temp_data_7[256] = {0x18,0x30,0x00,0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x23,0x40
    //     ,0x81,0xc7,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
    //     ,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
    //     ,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
    //     ,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
    //     ,0x00,0x00,0x00,0x00,0x00,0x04,0x11,0x02,0x88,0x01,0x30,0x01,0x88,0x01,0x2e,0x01
    //     ,0x88,0x01,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
    //     ,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
    //     ,0x18,0x4a,0x55,0x4e,0x49,0x50,0x45,0x52,0x2d,0x31,0x46,0x31,0x20,0x20,0x20,0x20
    //     ,0x20,0x20,0x1b,0xc9,0x37,0x34,0x30,0x2d,0x30,0x39,0x30,0x31,0x37,0x30,0x20,0x20
    //     ,0x20,0x20,0x20,0x20,0x30,0x31,0x49,0x4e,0x49,0x42,0x5a,0x38,0x30,0x39,0x30,0x32
    //     ,0x31,0x39,0x41,0x20,0x20,0x20,0x31,0x39,0x30,0x32,0x31,0x31,0x20,0x20,0x43,0x4d
    //     ,0x50,0x51,0x41,0x48,0x46,0x42,0x41,0x41,0xa0,0x30,0x5e,0x23,0x00,0x00,0x00,0x00
    //     ,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xd7,0x31
    //     ,0x46,0x31,0x53,0x47,0x41,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x30
    //     ,0x31,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}; //7
    //     memcpy(data,temp_data_7,256);
    //     }
    //     break;
    // case 1:{
    //     uint8_t temp_data_9[256] = {0x18,0x30,0x80,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
    //     ,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
    //     ,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
    //     ,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
    //     ,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
    //     ,0x00,0x00,0x00,0x00,0x00,0x03,0x1d,0x01,0x88,0x01,0x1c,0x01,0x88,0x01,0x1b,0x01
    //     ,0x88,0x01,0x18,0x01,0x88,0x01,0x1a,0x01,0x88,0x01,0x16,0x01,0x88,0x01,0x15,0x01
    //     ,0x88,0x01,0x14,0x01,0x88,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
    //     ,0x18,0x41,0x6d,0x70,0x68,0x65,0x6e,0x6f,0x6c,0x20,0x20,0x20,0x20,0x20,0x20,0x20
    //     ,0x20,0x78,0xa7,0x14,0x4e,0x44,0x59,0x59,0x59,0x48,0x2d,0x30,0x30,0x30,0x35,0x20
    //     ,0x20,0x20,0x20,0x20,0x44,0x20,0x41,0x50,0x46,0x31,0x39,0x31,0x37,0x30,0x30,0x35
    //     ,0x39,0x33,0x35,0x54,0x20,0x20,0x31,0x39,0x30,0x35,0x31,0x30,0x20,0x20,0x00,0x00
    //     ,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x19,0x23,0x07,0x09,0x0f,0x00
    //     ,0x00,0x00,0x00,0x02,0x0a,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xa5,0x00
    //     ,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
    //     ,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}; //9
    //     memcpy(data,temp_data_9,256);
    // }
    // break;
    // case 2:{
    //     uint8_t temp_data_10[256] = {0x18,0x30,0x80,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
    //     ,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
    //     ,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
    //     ,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
    //     ,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
    //     ,0x00,0x00,0x00,0x00,0x00,0x03,0x1d,0x01,0x88,0x01,0x1c,0x01,0x88,0x01,0x1b,0x01
    //     ,0x88,0x01,0x18,0x01,0x88,0x01,0x1a,0x01,0x88,0x01,0x16,0x01,0x88,0x01,0x15,0x01
    //     ,0x88,0x01,0x14,0x01,0x88,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
    //     ,0x18,0x41,0x6d,0x70,0x68,0x65,0x6e,0x6f,0x6c,0x20,0x20,0x20,0x20,0x20,0x20,0x20
    //     ,0x20,0x78,0xa7,0x14,0x4e,0x44,0x59,0x59,0x59,0x46,0x2d,0x30,0x30,0x30,0x31,0x20
    //     ,0x20,0x20,0x20,0x20,0x44,0x20,0x41,0x50,0x46,0x31,0x39,0x31,0x37,0x30,0x30,0x31
    //     ,0x38,0x48,0x4d,0x48,0x20,0x20,0x31,0x39,0x30,0x34,0x32,0x36,0x20,0x20,0x00,0x00
    //     ,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x0a,0x23,0x05,0x06,0x0b,0x00
    //     ,0x00,0x00,0x00,0x02,0x0a,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xa9,0x00
    //     ,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
    //     ,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}; //10
    //     memcpy(data,temp_data_10,256);
    // }
    // break;
    // case 3:{
    //     uint8_t temp_data_11[256] = {0x18,0x30,0x00,0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x2b,0x5f,
    //     0x81,0x1d,0xee,0x6c,0x31,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    //     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    //     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    //     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    //     0x00,0x00,0x00,0x00,0x00,0x02,0x11,0x1c,0x84,0x01,0xff,0x00,0x00,0x00,0x00,0x00,
    //     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    //     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    //     0x18,0x49,0x4e,0x4e,0x4f,0x4c,0x49,0x47,0x48,0x54,0x20,0x20,0x20,0x20,0x20,0x20,
    //     0x20,0x44,0x7c,0x7f,0x54,0x2d,0x44,0x50,0x34,0x43,0x4e,0x48,0x2d,0x4e,0x30,0x30,
    //     0x20,0x20,0x20,0x20,0x31,0x42,0x49,0x4e,0x49,0x42,0x55,0x38,0x31,0x34,0x31,0x32,
    //     0x39,0x30,0x20,0x20,0x20,0x20,0x32,0x30,0x31,0x39,0x30,0x33,0x31,0x32,0x00,0x00,
    //     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xa0,0x30,0x00,0x0c,0x00,0x00,0x00,0x00,
    //     0x00,0x00,0x00,0x00,0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xa7,0x00,
    //     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    //     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}; //11
    //     memcpy(data,temp_data_11,256);
    // }
    // break;
    // }
    return ONLP_STATUS_OK;
}

int
onlp_sfpi_rx_los_bitmap_get(onlp_sfp_bitmap_t* dst)
{
    return ONLP_STATUS_OK;
}

/*
 * De-initialize the SFPI subsystem.
 */
int
onlp_sfpi_denit(void)
{
    return ONLP_STATUS_OK;
}