/*
 * sdcard.c
 *
 *  Created on: Jun 19, 2021
 *      Author: ACER
 */


#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "sdcard.h"

uint8_t SD_Device(char buffer[200],uint8_t port,uint8_t deviceID,uint8_t func,char *deviceChannel,char *deviceType,char *deviceTitle,char *deviceName,char *valueType, uint8_t devicestatus, uint16_t scale)
{
    memset(buffer,'\0',200);
    sprintf(buffer,"{\"PORT\":%d,\"ID\":%d,\"FC\":%d,\"CHANNEL\":\"%s\",\"DEVICETYPE\":\"%s\",\"DEVICENAME\":\"%s\",\"CHANNELTITLE\":\"%s\",\"VALUETYPE\":\"%s\",\"DEVICESTATUS\":%d,\"SCALE\":%d}\r\n",port,deviceID,func,deviceChannel,deviceType,deviceName,deviceTitle,valueType,devicestatus,scale);
}
/* {"mqttId":"null","username":"null","pwd":"null","port":1883,"apikey":"60cda6bc55193093bbcd001f"}*/
uint8_t SD_Mqtt(char buffer[200],uint16_t mqtt_port,char *mqttId,char *username,char *pwd,char *apikey)
{
    memset(buffer,'\0',200);
    sprintf(buffer,"{\"mqtt\":{\"mqttId\":\"%s\",\"username\":\"%s\",\"pwd\":\"%s\",\"port\":%d,\"apikey\":\"%s\"}}\r\n",mqttId,username,pwd,mqtt_port,apikey);
}
/*
 * baud = 1200xhs(4800, 9600, 19200, 115200)*/
/* {"rs232":{"baud":115200,"databits":8,"stopbits":1,"parity":0}} */
uint8_t SD_Serial(char buffer[100],uint8_t type_serial,uint8_t baud,uint8_t databits, uint8_t stopbit,uint8_t parirty)
{
    memset(buffer,'\0',sizeof(buffer));
    if (type_serial==1){ //rs232
    	 sprintf(buffer,"{\"rs232\":{\"baud\":%d,\"databits\":%d,\"stopbits\":%d,\"parity\":%d}}\r\n",1200*baud,databits,stopbit,parirty);
    }else if(type_serial==2){ //port 0
    	sprintf(buffer,"{\"port0\":{\"baud\":%d,\"databits\":%d,\"stopbits\":%d,\"parity\":%d}}\r\n",1200*baud,databits,stopbit,parirty);
    }else if(type_serial==3){ //port 1
    	sprintf(buffer,"{\"port1\":{\"baud\":%d,\"databits\":%d,\"stopbits\":%d,\"parity\":%d}}\r\n",1200*baud,databits,stopbit,parirty);
    }
}
uint8_t SD_timeout(char buffer[50],uint16_t timeout)
{
    memset(buffer,'\0',sizeof(buffer));
    sprintf(buffer,"{\"timeout\":%d}\r\n",timeout);
}

