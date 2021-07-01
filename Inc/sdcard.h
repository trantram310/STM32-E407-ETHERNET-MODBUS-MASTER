/*
 * sdcard.h
 *
 *  Created on: Jun 19, 2021
 *      Author: ACER
 */

#ifndef SDCARD_H_
#define SDCARD_H_
#include <string.h>
#include <stdint.h>


uint8_t SD_Device(char buffer[200],uint8_t port,uint8_t deviceID,uint8_t func,char *deviceChannel,char *deviceType,char *deviceTitle,char *deviceName,char *valueType, uint8_t devicestatus, uint16_t scale);
uint8_t SD_Mqtt(char buffer[200],uint16_t mqtt_port,char *mqttId,char *username,char *pwd,char *apikey);
uint8_t SD_Serial(char buffer[200],uint8_t type_serial,uint8_t baud,uint8_t databits, uint8_t stopbit,uint8_t parirty);
uint8_t SD_timeout(char buffer[50],uint16_t timeout);
#endif /* SDCARD_H_ */