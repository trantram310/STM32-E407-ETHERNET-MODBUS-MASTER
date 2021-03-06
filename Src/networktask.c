/* Built-in C library includes ---------------*/
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Platform includes --------------------------*/
#include "stm32f4xx_hal.h"
#include "cmsis_os.h"
#include "FreeRTOS.h"
#include "main.h"
#include "fs.h"
#include "rtc.h"
#include "flash.h"
/* Shared Variable ----------------------------------*/

extern host_param_t hostParam;
extern port_param_t portParam[MB_MAX_PORT];
extern network_param_t mqttHostParam;

extern UART_HandleTypeDef huart6;

#include "lwip.h"
#include "socket.h"

/* Includes ------------------------------------------------------------------*/
#include "lwip/opt.h"
#include "lwip/arch.h"
#include "lwip/api.h"
#include "lwip/apps/fs.h"
#include "tcp.h"
#include "string.h"

#include "cmsis_os.h"

#include <stdio.h>
void http_server_netconn_thread(void *arg);
void http_server_netconn_init(void);
void NetworkTimeTask(void const *arg);
void DynWebPage(struct netconn *conn);
void TcpDiscoverTask(void *arg);

/* Private typedef -----------------------------------------------------------*/
//typdef struct {
//	struct ip_pcb  *ip;
//
//};
/* Private define ------------------------------------------------------------*/
#define WEBSERVER_THREAD_PRIO (osPriorityNormal)

/* Shared Variables -----------------------------------------*/
osThreadId netTimeTask;
osThreadId netHTTPTask;
osThreadId netTcpEchoTask;
osThreadId netFlashSave;

extern osMessageQId xQueueControlHandle;
extern osMessageQId xQueueMessageHandle;
extern osSemaphoreId netMqttIpSemaphoreHandle;

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
u32_t nPageHits = 0;

char tempBuffer[100];
uint8_t uiUserPass[30];

uint8_t IS_LOGIN = 0;

extern RTC_HandleTypeDef hrtc;

//char tempBuffer[100];
/* Format of dynamic web page: the page header */
static const unsigned char PAGE_START[] = { 0x3c, 0x21, 0x44, 0x4f, 0x43, 0x54,
		0x59, 0x50, 0x45, 0x20, 0x68, 0x74, 0x6d, 0x6c, 0x20, 0x50, 0x55, 0x42,
		0x4c, 0x49, 0x43, 0x20, 0x22, 0x2d, 0x2f, 0x2f, 0x57, 0x33, 0x43, 0x2f,
		0x2f, 0x44, 0x54, 0x44, 0x20, 0x48, 0x54, 0x4d, 0x4c, 0x20, 0x34, 0x2e,
		0x30, 0x31, 0x2f, 0x2f, 0x45, 0x4e, 0x22, 0x20, 0x22, 0x68, 0x74, 0x74,
		0x70, 0x3a, 0x2f, 0x2f, 0x77, 0x77, 0x77, 0x2e, 0x77, 0x33, 0x2e, 0x6f,
		0x72, 0x67, 0x2f, 0x54, 0x52, 0x2f, 0x68, 0x74, 0x6d, 0x6c, 0x34, 0x2f,
		0x73, 0x74, 0x72, 0x69, 0x63, 0x74, 0x2e, 0x64, 0x74, 0x64, 0x22, 0x3e,
		0x0d, 0x0a, 0x3c, 0x68, 0x74, 0x6d, 0x6c, 0x3e, 0x0d, 0x0a, 0x3c, 0x68,
		0x65, 0x61, 0x64, 0x3e, 0x0d, 0x0a, 0x20, 0x20, 0x3c, 0x74, 0x69, 0x74,
		0x6c, 0x65, 0x3e, 0x53, 0x54, 0x4d, 0x33, 0x32, 0x46, 0x37, 0x78, 0x78,
		0x54, 0x41, 0x53, 0x4b, 0x53, 0x3c, 0x2f, 0x74, 0x69, 0x74, 0x6c, 0x65,
		0x3e, 0x0d, 0x0a, 0x20, 0x20, 0x3c, 0x6d, 0x65, 0x74, 0x61, 0x20, 0x68,
		0x74, 0x74, 0x70, 0x2d, 0x65, 0x71, 0x75, 0x69, 0x76, 0x3d, 0x22, 0x43,
		0x6f, 0x6e, 0x74, 0x65, 0x6e, 0x74, 0x2d, 0x54, 0x79, 0x70, 0x65, 0x22,
		0x0d, 0x0a, 0x20, 0x63, 0x6f, 0x6e, 0x74, 0x65, 0x6e, 0x74, 0x3d, 0x22,
		0x74, 0x65, 0x78, 0x74, 0x2f, 0x68, 0x74, 0x6d, 0x6c, 0x3b, 0x20, 0x63,
		0x68, 0x61, 0x72, 0x73, 0x65, 0x74, 0x3d, 0x77, 0x69, 0x6e, 0x64, 0x6f,
		0x77, 0x73, 0x2d, 0x31, 0x32, 0x35, 0x32, 0x22, 0x3e, 0x0d, 0x0a, 0x20,
		0x20, 0x3c, 0x6d, 0x65, 0x74, 0x61, 0x20, 0x68, 0x74, 0x74, 0x70, 0x2d,
		0x65, 0x71, 0x75, 0x69, 0x76, 0x3d, 0x22, 0x72, 0x65, 0x66, 0x72, 0x65,
		0x73, 0x68, 0x22, 0x20, 0x63, 0x6f, 0x6e, 0x74, 0x65, 0x6e, 0x74, 0x3d,
		0x22, 0x31, 0x22, 0x3e, 0x0d, 0x0a, 0x20, 0x20, 0x3c, 0x6d, 0x65, 0x74,
		0x61, 0x20, 0x63, 0x6f, 0x6e, 0x74, 0x65, 0x6e, 0x74, 0x3d, 0x22, 0x4d,
		0x53, 0x48, 0x54, 0x4d, 0x4c, 0x20, 0x36, 0x2e, 0x30, 0x30, 0x2e, 0x32,
		0x38, 0x30, 0x30, 0x2e, 0x31, 0x35, 0x36, 0x31, 0x22, 0x20, 0x6e, 0x61,
		0x6d, 0x65, 0x3d, 0x22, 0x47, 0x45, 0x4e, 0x45, 0x52, 0x41, 0x54, 0x4f,
		0x52, 0x22, 0x3e, 0x0d, 0x0a, 0x20, 0x20, 0x3c, 0x73, 0x74, 0x79, 0x6c,
		0x65, 0x20, 0x3d, 0x22, 0x66, 0x6f, 0x6e, 0x74, 0x2d, 0x77, 0x65, 0x69,
		0x67, 0x68, 0x74, 0x3a, 0x20, 0x6e, 0x6f, 0x72, 0x6d, 0x61, 0x6c, 0x3b,
		0x20, 0x66, 0x6f, 0x6e, 0x74, 0x2d, 0x66, 0x61, 0x6d, 0x69, 0x6c, 0x79,
		0x3a, 0x20, 0x56, 0x65, 0x72, 0x64, 0x61, 0x6e, 0x61, 0x3b, 0x22, 0x3e,
		0x3c, 0x2f, 0x73, 0x74, 0x79, 0x6c, 0x65, 0x3e, 0x0d, 0x0a, 0x3c, 0x2f,
		0x68, 0x65, 0x61, 0x64, 0x3e, 0x0d, 0x0a, 0x3c, 0x62, 0x6f, 0x64, 0x79,
		0x3e, 0x0d, 0x0a, 0x3c, 0x68, 0x34, 0x3e, 0x3c, 0x73, 0x6d, 0x61, 0x6c,
		0x6c, 0x20, 0x73, 0x74, 0x79, 0x6c, 0x65, 0x3d, 0x22, 0x66, 0x6f, 0x6e,
		0x74, 0x2d, 0x66, 0x61, 0x6d, 0x69, 0x6c, 0x79, 0x3a, 0x20, 0x56, 0x65,
		0x72, 0x64, 0x61, 0x6e, 0x61, 0x3b, 0x22, 0x3e, 0x3c, 0x73, 0x6d, 0x61,
		0x6c, 0x6c, 0x3e, 0x3c, 0x62, 0x69, 0x67, 0x3e, 0x3c, 0x62, 0x69, 0x67,
		0x3e, 0x3c, 0x62, 0x69, 0x67, 0x0d, 0x0a, 0x20, 0x73, 0x74, 0x79, 0x6c,
		0x65, 0x3d, 0x22, 0x66, 0x6f, 0x6e, 0x74, 0x2d, 0x77, 0x65, 0x69, 0x67,
		0x68, 0x74, 0x3a, 0x20, 0x62, 0x6f, 0x6c, 0x64, 0x3b, 0x22, 0x3e, 0x3c,
		0x62, 0x69, 0x67, 0x3e, 0x3c, 0x73, 0x74, 0x72, 0x6f, 0x6e, 0x67, 0x3e,
		0x3c, 0x65, 0x6d, 0x3e, 0x3c, 0x73, 0x70, 0x61, 0x6e, 0x0d, 0x0a, 0x20,
		0x73, 0x74, 0x79, 0x6c, 0x65, 0x3d, 0x22, 0x66, 0x6f, 0x6e, 0x74, 0x2d,
		0x73, 0x74, 0x79, 0x6c, 0x65, 0x3a, 0x20, 0x69, 0x74, 0x61, 0x6c, 0x69,
		0x63, 0x3b, 0x22, 0x3e, 0x53, 0x54, 0x4d, 0x33, 0x32, 0x46, 0x37, 0x78,
		0x78, 0x20, 0x4c, 0x69, 0x73, 0x74, 0x20, 0x6f, 0x66, 0x20, 0x74, 0x61,
		0x73, 0x6b, 0x73, 0x20, 0x61, 0x6e, 0x64, 0x0d, 0x0a, 0x74, 0x68, 0x65,
		0x69, 0x72, 0x20, 0x73, 0x74, 0x61, 0x74, 0x75, 0x73, 0x3c, 0x2f, 0x73,
		0x70, 0x61, 0x6e, 0x3e, 0x3c, 0x2f, 0x65, 0x6d, 0x3e, 0x3c, 0x2f, 0x73,
		0x74, 0x72, 0x6f, 0x6e, 0x67, 0x3e, 0x3c, 0x2f, 0x62, 0x69, 0x67, 0x3e,
		0x3c, 0x2f, 0x62, 0x69, 0x67, 0x3e, 0x3c, 0x2f, 0x62, 0x69, 0x67, 0x3e,
		0x3c, 0x2f, 0x62, 0x69, 0x67, 0x3e, 0x3c, 0x2f, 0x73, 0x6d, 0x61, 0x6c,
		0x6c, 0x3e, 0x3c, 0x2f, 0x73, 0x6d, 0x61, 0x6c, 0x6c, 0x3e, 0x3c, 0x2f,
		0x68, 0x34, 0x3e, 0x0d, 0x0a, 0x3c, 0x68, 0x72, 0x20, 0x73, 0x74, 0x79,
		0x6c, 0x65, 0x3d, 0x22, 0x77, 0x69, 0x64, 0x74, 0x68, 0x3a, 0x20, 0x31,
		0x30, 0x30, 0x25, 0x3b, 0x20, 0x68, 0x65, 0x69, 0x67, 0x68, 0x74, 0x3a,
		0x20, 0x32, 0x70, 0x78, 0x3b, 0x22, 0x3e, 0x3c, 0x73, 0x70, 0x61, 0x6e,
		0x0d, 0x0a, 0x20, 0x73, 0x74, 0x79, 0x6c, 0x65, 0x3d, 0x22, 0x66, 0x6f,
		0x6e, 0x74, 0x2d, 0x77, 0x65, 0x69, 0x67, 0x68, 0x74, 0x3a, 0x20, 0x62,
		0x6f, 0x6c, 0x64, 0x3b, 0x22, 0x3e, 0x0d, 0x0a, 0x3c, 0x2f, 0x73, 0x70,
		0x61, 0x6e, 0x3e, 0x3c, 0x73, 0x70, 0x61, 0x6e, 0x20, 0x73, 0x74, 0x79,
		0x6c, 0x65, 0x3d, 0x22, 0x66, 0x6f, 0x6e, 0x74, 0x2d, 0x77, 0x65, 0x69,
		0x67, 0x68, 0x74, 0x3a, 0x20, 0x62, 0x6f, 0x6c, 0x64, 0x3b, 0x22, 0x3e,
		0x0d, 0x0a, 0x3c, 0x74, 0x61, 0x62, 0x6c, 0x65, 0x20, 0x73, 0x74, 0x79,
		0x6c, 0x65, 0x3d, 0x22, 0x77, 0x69, 0x64, 0x74, 0x68, 0x3a, 0x20, 0x39,
		0x36, 0x31, 0x70, 0x78, 0x3b, 0x20, 0x68, 0x65, 0x69, 0x67, 0x68, 0x74,
		0x3a, 0x20, 0x33, 0x30, 0x70, 0x78, 0x3b, 0x22, 0x20, 0x62, 0x6f, 0x72,
		0x64, 0x65, 0x72, 0x3d, 0x22, 0x31, 0x22, 0x0d, 0x0a, 0x20, 0x63, 0x65,
		0x6c, 0x6c, 0x70, 0x61, 0x64, 0x64, 0x69, 0x6e, 0x67, 0x3d, 0x22, 0x32,
		0x22, 0x20, 0x63, 0x65, 0x6c, 0x6c, 0x73, 0x70, 0x61, 0x63, 0x69, 0x6e,
		0x67, 0x3d, 0x22, 0x32, 0x22, 0x3e, 0x0d, 0x0a, 0x20, 0x20, 0x3c, 0x74,
		0x62, 0x6f, 0x64, 0x79, 0x3e, 0x0d, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x3c,
		0x74, 0x72, 0x3e, 0x0d, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x3c,
		0x74, 0x64, 0x0d, 0x0a, 0x20, 0x73, 0x74, 0x79, 0x6c, 0x65, 0x3d, 0x22,
		0x66, 0x6f, 0x6e, 0x74, 0x2d, 0x66, 0x61, 0x6d, 0x69, 0x6c, 0x79, 0x3a,
		0x20, 0x56, 0x65, 0x72, 0x64, 0x61, 0x6e, 0x61, 0x3b, 0x20, 0x66, 0x6f,
		0x6e, 0x74, 0x2d, 0x77, 0x65, 0x69, 0x67, 0x68, 0x74, 0x3a, 0x20, 0x62,
		0x6f, 0x6c, 0x64, 0x3b, 0x20, 0x66, 0x6f, 0x6e, 0x74, 0x2d, 0x73, 0x74,
		0x79, 0x6c, 0x65, 0x3a, 0x20, 0x69, 0x74, 0x61, 0x6c, 0x69, 0x63, 0x3b,
		0x20, 0x62, 0x61, 0x63, 0x6b, 0x67, 0x72, 0x6f, 0x75, 0x6e, 0x64, 0x2d,
		0x63, 0x6f, 0x6c, 0x6f, 0x72, 0x3a, 0x20, 0x72, 0x67, 0x62, 0x28, 0x35,
		0x31, 0x2c, 0x20, 0x35, 0x31, 0x2c, 0x20, 0x32, 0x35, 0x35, 0x29, 0x3b,
		0x20, 0x74, 0x65, 0x78, 0x74, 0x2d, 0x61, 0x6c, 0x69, 0x67, 0x6e, 0x3a,
		0x20, 0x63, 0x65, 0x6e, 0x74, 0x65, 0x72, 0x3b, 0x22, 0x3e, 0x3c, 0x73,
		0x6d, 0x61, 0x6c, 0x6c, 0x3e, 0x3c, 0x61, 0x0d, 0x0a, 0x20, 0x68, 0x72,
		0x65, 0x66, 0x3d, 0x22, 0x2f, 0x53, 0x54, 0x4d, 0x33, 0x32, 0x46, 0x37,
		0x78, 0x78, 0x2e, 0x68, 0x74, 0x6d, 0x6c, 0x22, 0x3e, 0x3c, 0x73, 0x70,
		0x61, 0x6e, 0x20, 0x73, 0x74, 0x79, 0x6c, 0x65, 0x3d, 0x22, 0x63, 0x6f,
		0x6c, 0x6f, 0x72, 0x3a, 0x20, 0x77, 0x68, 0x69, 0x74, 0x65, 0x3b, 0x22,
		0x3e, 0x48, 0x6f, 0x6d, 0x65, 0x0d, 0x0a, 0x70, 0x61, 0x67, 0x65, 0x3c,
		0x2f, 0x73, 0x70, 0x61, 0x6e, 0x3e, 0x3c, 0x2f, 0x61, 0x3e, 0x3c, 0x2f,
		0x73, 0x6d, 0x61, 0x6c, 0x6c, 0x3e, 0x3c, 0x2f, 0x74, 0x64, 0x3e, 0x0d,
		0x0a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x3c, 0x74, 0x64, 0x0d, 0x0a,
		0x20, 0x73, 0x74, 0x79, 0x6c, 0x65, 0x3d, 0x22, 0x66, 0x6f, 0x6e, 0x74,
		0x2d, 0x66, 0x61, 0x6d, 0x69, 0x6c, 0x79, 0x3a, 0x20, 0x56, 0x65, 0x72,
		0x64, 0x61, 0x6e, 0x61, 0x3b, 0x20, 0x66, 0x6f, 0x6e, 0x74, 0x2d, 0x77,
		0x65, 0x69, 0x67, 0x68, 0x74, 0x3a, 0x20, 0x62, 0x6f, 0x6c, 0x64, 0x3b,
		0x20, 0x66, 0x6f, 0x6e, 0x74, 0x2d, 0x73, 0x74, 0x79, 0x6c, 0x65, 0x3a,
		0x20, 0x69, 0x74, 0x61, 0x6c, 0x69, 0x63, 0x3b, 0x20, 0x62, 0x61, 0x63,
		0x6b, 0x67, 0x72, 0x6f, 0x75, 0x6e, 0x64, 0x2d, 0x63, 0x6f, 0x6c, 0x6f,
		0x72, 0x3a, 0x20, 0x72, 0x67, 0x62, 0x28, 0x35, 0x31, 0x2c, 0x20, 0x35,
		0x31, 0x2c, 0x20, 0x32, 0x35, 0x35, 0x29, 0x3b, 0x20, 0x74, 0x65, 0x78,
		0x74, 0x2d, 0x61, 0x6c, 0x69, 0x67, 0x6e, 0x3a, 0x20, 0x63, 0x65, 0x6e,
		0x74, 0x65, 0x72, 0x3b, 0x22, 0x3e, 0x3c, 0x61, 0x0d, 0x0a, 0x20, 0x68,
		0x72, 0x65, 0x66, 0x3d, 0x22, 0x53, 0x54, 0x4d, 0x33, 0x32, 0x46, 0x37,
		0x78, 0x78, 0x41, 0x44, 0x43, 0x2e, 0x68, 0x74, 0x6d, 0x6c, 0x22, 0x3e,
		0x3c, 0x73, 0x70, 0x61, 0x6e, 0x20, 0x73, 0x74, 0x79, 0x6c, 0x65, 0x3d,
		0x22, 0x66, 0x6f, 0x6e, 0x74, 0x2d, 0x77, 0x65, 0x69, 0x67, 0x68, 0x74,
		0x3a, 0x20, 0x62, 0x6f, 0x6c, 0x64, 0x3b, 0x22, 0x3e, 0x3c, 0x2f, 0x73,
		0x70, 0x61, 0x6e, 0x3e, 0x3c, 0x2f, 0x61, 0x3e, 0x3c, 0x73, 0x6d, 0x61,
		0x6c, 0x6c, 0x3e, 0x3c, 0x61, 0x0d, 0x0a, 0x20, 0x68, 0x72, 0x65, 0x66,
		0x3d, 0x22, 0x2f, 0x53, 0x54, 0x4d, 0x33, 0x32, 0x46, 0x37, 0x78, 0x78,
		0x54, 0x41, 0x53, 0x4b, 0x53, 0x2e, 0x68, 0x74, 0x6d, 0x6c, 0x22, 0x3e,
		0x3c, 0x73, 0x70, 0x61, 0x6e, 0x20, 0x73, 0x74, 0x79, 0x6c, 0x65, 0x3d,
		0x22, 0x63, 0x6f, 0x6c, 0x6f, 0x72, 0x3a, 0x20, 0x77, 0x68, 0x69, 0x74,
		0x65, 0x3b, 0x22, 0x3e, 0x4c, 0x69, 0x73, 0x74, 0x0d, 0x0a, 0x6f, 0x66,
		0x20, 0x74, 0x61, 0x73, 0x6b, 0x73, 0x3c, 0x2f, 0x73, 0x70, 0x61, 0x6e,
		0x3e, 0x3c, 0x2f, 0x61, 0x3e, 0x3c, 0x2f, 0x73, 0x6d, 0x61, 0x6c, 0x6c,
		0x3e, 0x3c, 0x2f, 0x74, 0x64, 0x3e, 0x0d, 0x0a, 0x20, 0x20, 0x20, 0x20,
		0x3c, 0x2f, 0x74, 0x72, 0x3e, 0x0d, 0x0a, 0x20, 0x20, 0x3c, 0x2f, 0x74,
		0x62, 0x6f, 0x64, 0x79, 0x3e, 0x0d, 0x0a, 0x3c, 0x2f, 0x74, 0x61, 0x62,
		0x6c, 0x65, 0x3e, 0x0d, 0x0a, 0x3c, 0x62, 0x72, 0x3e, 0x0d, 0x0a, 0x3c,
		0x2f, 0x73, 0x70, 0x61, 0x6e, 0x3e, 0x3c, 0x73, 0x70, 0x61, 0x6e, 0x20,
		0x73, 0x74, 0x79, 0x6c, 0x65, 0x3d, 0x22, 0x66, 0x6f, 0x6e, 0x74, 0x2d,
		0x77, 0x65, 0x69, 0x67, 0x68, 0x74, 0x3a, 0x20, 0x62, 0x6f, 0x6c, 0x64,
		0x3b, 0x22, 0x3e, 0x3c, 0x2f, 0x73, 0x70, 0x61, 0x6e, 0x3e, 0x3c, 0x73,
		0x6d, 0x61, 0x6c, 0x6c, 0x3e, 0x3c, 0x73, 0x70, 0x61, 0x6e, 0x0d, 0x0a,
		0x20, 0x73, 0x74, 0x79, 0x6c, 0x65, 0x3d, 0x22, 0x66, 0x6f, 0x6e, 0x74,
		0x2d, 0x66, 0x61, 0x6d, 0x69, 0x6c, 0x79, 0x3a, 0x20, 0x56, 0x65, 0x72,
		0x64, 0x61, 0x6e, 0x61, 0x3b, 0x22, 0x3e, 0x4e, 0x75, 0x6d, 0x62, 0x65,
		0x72, 0x20, 0x6f, 0x66, 0x20, 0x70, 0x61, 0x67, 0x65, 0x20, 0x68, 0x69,
		0x74, 0x73, 0x3a, 0x0d, 0x0a, 0x00 };

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/**
 * @brief serve tcp connection
 * @param conn: pointer on connection structure
 * @retval None
 */
static void http_server_serve(struct netconn *conn) {
	struct netbuf *inbuf;
	err_t recv_err;
	char *buf;
	u16_t buflen;
	struct fs_file file;
	char *ptrUser;
	char *ptrMalloc;
	/* Read the data from the port, blocking if nothing yet there.
	 We assume the request (the part we care about) is in one netbuf */
	recv_err = netconn_recv(conn, &inbuf);

	if (recv_err == ERR_OK) {
		if (netconn_err(conn) == ERR_OK) {

			netbuf_data(inbuf, (void **) &buf, &buflen);
			printf("\r\nReceived data from  to client with %d byte:  \n",
					buflen);

			HAL_UART_Transmit(&huart6, buf, buflen, 1000);

			/*Find useful data in frame recived and add to a pointer*/
			ptrUser = strstr(buf, "username");
			if (strcmp(ptrUser, uiUserPass) == 0) {
				printf("\r\n Matched user and pw \r\n");
				fs_open(&file, "/index.html");
				netconn_write(conn, (const unsigned char * )(file.data),
						(size_t )file.len, NETCONN_NOCOPY);
				fs_close(&file);
			}

			/* Is this an HTTP GET command? (only check the first 5 chars, since
			 there are other formats for GET, and we're keeping it very simple )*/
			if ((buflen >= 5) && (strncmp(buf, "GET /", 5) == 0)) {
				/* Check if request to get ST.gif */
				/*------------------------------------------------------------------------------------*/
				if (strncmp((char const *) buf, "GET /script/common.js", 18)
						== 0) {
					fs_open(&file, "/script/common.js");
					netconn_write(conn, (const unsigned char * )(file.data),
							(size_t )file.len, NETCONN_NOCOPY);
					fs_close(&file);
				}
				//				else if (strncmp((char const *) buf,
				//						"GET /script/dataport.js", 23) == 0) {
				//					ptrMalloc = malloc(100 * sizeof(char));
				//					printf("-------------------------------------------------");
				//					memset(ptrMalloc, 0, sizeof(100 * sizeof(char)));

				//					for (uint8_t uiPort = 0; uiPort < MB_MAX_PORT; uiPort++) {
				//						strcat((char *) ptrMalloc, "var DataString	");
				//						strcat((char*) ptrMalloc,
				//								(char *) itoa_user(uiPort, 10));
				//						strcat((char *) ptrMalloc, " = new Object();\n");
				//						/*Port number*/
				//						strcat((char *) ptrMalloc, "DataString.port");
				//						strcat((char *) ptrMalloc,
				//								(char *) itoa_user(uiPort, 10));
				/*Port state*/
				//						strcat((char *) ptrMalloc, "_active = \"");
				//						strcat((char *) ptrMalloc,
				//								(char *) itoa_user(
				//										portParam[uiPort].uiPortState, 10));
				//						strcat((char *) ptrMalloc, "\";\n");
				//						for (uint8_t uiDev = 0; uiDev < PORT_MAX_DEV; uiDev++) {
				//
				//							/*Port devices Adr*/
				//
				//							strcat((char *) ptrMalloc, "var DataString");
				//							strcat((char*) ptrMalloc,
				//									(char *) itoa_user(uiPort, 10));
				//							strcat((char *) ptrMalloc, ".dev");
				//							strcat((char *) ptrMalloc,
				//									(char *) itoa_user(uiDev, 10));
				//							strcat((char *) ptrMalloc, "_adr = \"");
				//							strcat((char *) ptrMalloc,
				//									(char *) itoa_user(
				//											portParam[uiPort].portDev[uiDev].uiAdr,
				//											10));
				//							strcat((char *) ptrMalloc, "\";\n");
				//							/*Port devices state*/
				//							strcat((char *) ptrMalloc, "var DataString");
				//							strcat((char*) ptrMalloc,
				//									(char *) itoa_user(uiPort, 10));
				//							strcat((char *) ptrMalloc, ".dev");
				//							strcat((char *) ptrMalloc,
				//									(char *) itoa_user(uiDev, 10));
				//							strcat((char *) ptrMalloc, "_act = \"");
				//							strcat((char *) ptrMalloc,
				//									(char *) itoa_user(
				//											portParam[uiPort].portDev[uiDev].uiDevState,
				//											10));
				//							strcat((char *) ptrMalloc, "\";\n");
				//
				//						}
				//
				//					}
				//					free(ptrMalloc);
				//				}
				else if (strncmp((char const *) buf, "GET /datahost.js", 20)
						== 0) {
				} else if (strncmp((char const *) buf, "GET /script/lang_en.js",
						19) == 0) {
					fs_open(&file, "/script/lang_en.js");
					netconn_write(conn, (const unsigned char * )(file.data),
							(size_t )file.len, NETCONN_NOCOPY);
					fs_close(&file);
				} else if (strncmp((char const *) buf, "GET /style/style.css",
						17) == 0) {
					fs_open(&file, "/style/style.css");
					netconn_write(conn, (const unsigned char * )(file.data),
							(size_t )file.len, NETCONN_NOCOPY);
					fs_close(&file);
				} else if (strncmp((char const *) buf, "GET /reboot.html", 13)
						== 0) {
					fs_open(&file, "/reboot.html");
					netconn_write(conn, (const unsigned char * )(file.data),
							(size_t )file.len, NETCONN_NOCOPY);
					fs_close(&file);
				} else if (strncmp((char const *) buf, "GET /portconfig.html",
						17) == 0) {
					fs_open(&file, "/portconfig.html");
					netconn_write(conn, (const unsigned char * )(file.data),
							(size_t )file.len, NETCONN_NOCOPY);
					fs_close(&file);
				} else if (strncmp((char const *) buf, "GET /maintain.html", 15)
						== 0) {
					fs_open(&file, "/maintain.html");
					netconn_write(conn, (const unsigned char * )(file.data),
							(size_t )file.len, NETCONN_NOCOPY);
					fs_close(&file);
				} else if (strncmp((char const *) buf, "GET /lanconfig.html",
						16) == 0) {
					fs_open(&file, "/lanconfig.html");
					netconn_write(conn, (const unsigned char * )(file.data),
							(size_t )file.len, NETCONN_NOCOPY);
					fs_close(&file);
				} else if (strncmp((char const *) buf, "GET /index.html", 12)
						== 0) {
					fs_open(&file, "/index.html");
					netconn_write(conn, (const unsigned char * )(file.data),
							(size_t )file.len, NETCONN_NOCOPY);
					fs_close(&file);
				}

				else if (strncmp((char const *) buf, "GET /hostconfig.html", 17)
						== 0) {
					fs_open(&file, "/hostconfig.html");
					netconn_write(conn, (const unsigned char * )(file.data),
							(size_t )file.len, NETCONN_NOCOPY);
					fs_close(&file);
				}

				else if ((strncmp(buf, "GET /login.html", 12) == 0)
						|| (strncmp(buf, "GET / ", 6) == 0)) {
					/* Load STM32F7xx page */
					fs_open(&file, "/login.html");
					netconn_write(conn, (const unsigned char * )(file.data),
							(size_t )file.len, NETCONN_NOCOPY);
					fs_close(&file);
				}

				else {
					/* Load Error page */
					fs_open(&file, "/404.html");
					netconn_write(conn, (const unsigned char * )(file.data),
							(size_t )file.len, NETCONN_NOCOPY);
					fs_close(&file);
				}
				/*------------------------------------------------------------------------------------*/
			}
		}
	}
	/* Close the connection (server closes in HTTP) */
	netconn_close(conn);

	/* Delete the buffer (netconn_recv gives us ownership,
	 so we have to make sure to deallocate the buffer) */
	netbuf_delete(inbuf);
}

/**
 * @brief  http server thread
 * @param arg: pointer on argument(not used here)
 * @retval None
 */
void http_server_netconn_thread(void *arg) {
	struct netconn *conn, *newconn;
	err_t err, accept_err;
	printf("\r\n http_server_netconn_thread \r\n");
	xQueueControl_t xQueueControl;
	uint8_t uiSysState;
	xQueueControl.xTask = netHTTPTask;

	do {
		osDelay(10);
		xQueuePeek(xQueueMessageHandle, &uiSysState, 0);
	} while (uiSysState != SYS_HTTP);
	xQueueReceive(xQueueMessageHandle, &uiSysState, 0);
	printf("\r\b HTTP Service startring \r\n");

	/* Create a new TCP connection handle */
	conn = netconn_new(NETCONN_TCP);

	memset(uiUserPass, 0, sizeof(uiUserPass));
	strcat(uiUserPass, "username=");
	strcat(uiUserPass, hostParam.username[0].cdata);
	strcat(uiUserPass, "&password=");
	strcat(uiUserPass, hostParam.password[0].cdata);
	osDelay(1000);
	while (1)
		if (conn != NULL) {
			/* Bind to port 80 (HTTP) with default IP address */
			err = netconn_bind(conn, NULL, 80);

			if (err == ERR_OK) {
				/* Put the connection into LISTEN state */
				netconn_listen(conn);
				xQueueControl.xState = TASK_RUNNING;
				xQueueSend(xQueueControlHandle, &xQueueControl, 10);
				while (1) {
					/* accept any icoming connection */
					accept_err = netconn_accept(conn, &newconn);
					if (accept_err == ERR_OK) {
						/* serve connection */
						http_server_serve(newconn);

						/* delete connection */
						netconn_delete(newconn);
					}
				}
			}
		}
}

/**
 * @brief  Initialize the HTTP server (start its thread)
 * @param  none
 * @retval None
 */
void http_server_netconn_init() {
	sys_thread_new("HTTP", http_server_netconn_thread, NULL,
	DEFAULT_THREAD_STACKSIZE * 12, WEBSERVER_THREAD_PRIO);
	osDelay(10);
}

/**
 * @brief  Create and send a dynamic Web Page. This page contains the list of
 *         running tasks and the number of page hits.
 * @param  conn pointer on connection structure
 * @retval None
 */
void DynWebPage(struct netconn *conn) {
	char PAGE_BODY[512];
	char pagehits[10] = { 0 };

	memset(PAGE_BODY, 0, 512);

	/* Update the hit count */
	nPageHits++;
	sprintf(pagehits, "%d", (int) nPageHits);
	strcat(PAGE_BODY, pagehits);
	strcat((char *) PAGE_BODY,
			"<pre><br>Name          State  Priority  Stack   Num");
	strcat((char *) PAGE_BODY,
			"<br>---------------------------------------------<br>");

	/* The list of tasks and their status */
	osThreadList((unsigned char *) (PAGE_BODY + strlen(PAGE_BODY)));
	strcat((char *) PAGE_BODY,
			"<br><br>---------------------------------------------");
	strcat((char *) PAGE_BODY,
			"<br>B : Blocked, R : Ready, D : Deleted, S : Suspended<br>");

	/* Send the dynamically generated page */
	netconn_write(conn, PAGE_START, strlen((char * )PAGE_START), NETCONN_COPY);
	netconn_write(conn, PAGE_BODY, strlen(PAGE_BODY), NETCONN_COPY);
}

void LoginWebPage(struct netconn *conn) {
}

/*
 * Brief: Handle Time Packet from the Internet.
 * @pram: tcp conn pointer
 * */
static void timer_nist_serve(struct netconn *conn) {

	RTC_TimeTypeDef sTime = { 0 };
	RTC_DateTypeDef sDate = { 0 };
	struct netbuf *inbuf;
	err_t recv_err;
	char *buf;
	u16_t buflen;
	char *ptrUser;
	/* Read the data from the port, blocking if nothing yet there.
	 We assume the request (the part we care about) is in one netbuf */
	recv_err = netconn_recv(conn, &inbuf);

	if (recv_err == ERR_OK) {
		if (netconn_err(conn) == ERR_OK) {
			netbuf_data(inbuf, (void **) &buf, &buflen);
			printf("\r\n Received data to client with %d byte:  \n", buflen);
			HAL_UART_Transmit(&huart6, buf, buflen, 1090);
			ptrUser = strstr(buf, "-");
			/*   58954 20-04-15 16:37:07 50 0 0 418.7 UTC(NIST) *
			 * */
			sDate.Month = (*(ptrUser + 1) - 48) * 10 + (*(ptrUser + 2) - 48);
			sDate.Date = (*(ptrUser + 4) - 48) * 10 + (*(ptrUser + 5) - 48);
			sDate.Year = (*(ptrUser - 2) - 48) * 10 + (*(ptrUser - 1) - 48);

			if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK) {
				printf("BUGGGGG");
				Error_Handler();
			}
			ptrUser = strstr(buf, ":");
#define EDT_TIME_OFFSET +7
			sTime.Hours = (*(ptrUser - 2) - 48) * 10 + (*(ptrUser - 1) - 48 + EDT_TIME_OFFSET);
			sTime.Minutes = (*(ptrUser + 1) - 48) * 10 + (*(ptrUser + 2) - 48);
			sTime.Seconds = (*(ptrUser + 4) - 48) * 10 + (*(ptrUser + 5) - 48);
			if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK) {
				printf("BUGGGGG");
				Error_Handler();
			}
			printf("\r\n Time: %d %d %d \r\n", sTime.Hours, sTime.Minutes,sTime.Seconds);

			/*Enable chip built-in RTC*/
			__HAL_RCC_RTC_ENABLE();
		} else {
			printf("\r\n Failled to the network time \r\n");
		}
	}
}
void netFlashSaveTask(void *arg)
{
	xFlashSave();
	vTaskDelay(100);
	NVIC_SystemReset();

	while(1);

}
void netTcpEcho(struct netconn *pxNetCon) {
	struct netbuf *pxRxBuffer;
	portCHAR *pcRxString;
	unsigned portSHORT usLength;
	struct netbuf *inbuf;
	err_t recv_err;
	char *buf;
	u16_t buflen;
	ip4_addr_t ip;
	uint16_t port;
	char *mess = "conChoLong";
	netconn_write(pxNetCon, mess, (u16_t )15, NETCONN_COPY);
	recv_err = netconn_recv(pxNetCon, &inbuf);
	//	recv_err = netconn_recv(conn, &inbuf);
//	ip =pxNetCon->pcb.tcp->local_ip;
	netconn_getaddr(pxNetCon, &ip, &port, 0);
	mqttHostParam.ip.idata = ip.addr;
	printf("\r\n TCP incomming from: %d %d %d %d", mqttHostParam.ip.cdata[0],
			mqttHostParam.ip.cdata[1], mqttHostParam.ip.cdata[2],
			mqttHostParam.ip.cdata[3]);

//	ip = (pxNetCon->pcb).ip->remote_ip;

	if (recv_err == ERR_OK) {
		if (netconn_err(pxNetCon) == ERR_OK) {
			netbuf_data(inbuf, (void **) &buf, &buflen);
			printf("\r\nReceived data from  to client with %d byte:  \r\n",
					buflen);
			/*Todo: Give semaphore to reconnect mqtt with new ip*/

			HAL_UART_Transmit(&huart6, buf, buflen, 1000);
			//			netconn_close(pxNetCon);

			/* Delete the buffer (netconn_recv gives us ownership,
			 so we have to make sure to deallocate the buffer) */
			netbuf_delete(inbuf);
		}
	}
	/* Store new MQTT addr - be careful!*/
//	portDISABLE_INTERRUPTS();
	osThreadDef(netFlashSave, netFlashSaveTask, osPriorityRealtime, 0, 2 * 128);
	netFlashSave = osThreadCreate(osThread(netFlashSave), NULL);
	__disable_irq();
//	vTaskEndScheduler();
//	mqttHostParam.ip.cdata[0] = 0x66;
//	mqttHostParam.ip.cdata[1] = 0x67;
//	mqttHostParam.ip.cdata[2] = 0x68;
//	mqttHostParam.ip.cdata[3] = 0x69;
	xFlashSave();
	__enable_irq();
	portENABLE_INTERRUPTS();

	netconn_close(pxNetCon);
	vTaskDelay(100);
	NVIC_SystemReset();
//	NVIC_SystemReset();

}
void NetworkTimeTask(void const *arg) {
	printf("\r\n netTimeTask \r\n");

	struct netconn *conn, *newconn;
	err_t err, accept_err;
	uint8_t TimeNistIP[4];
	/*[132.163.96.3]*/
	TimeNistIP[0] = 132;
	TimeNistIP[1] = 163;
	TimeNistIP[2] = 96;
	TimeNistIP[3] = 3;
	/*Waiting Os for start-up*/
	xQueueControl_t xQueueControl;
	uint8_t uiSysState;
	xQueueControl.xTask = netTimeTask;
	do {
		osDelay(10);
		xQueuePeek(xQueueMessageHandle, &uiSysState, 0);
	} while (uiSysState != SYS_NET_TIME);
	xQueueReceive(xQueueMessageHandle, &uiSysState, 0);
	/* Create a new TCP connection handle */
	conn = netconn_new(NETCONN_TCP);
	if (conn != NULL) {
		/* Bind to port 13 with default IP address */
		err = netconn_bind(conn, NULL, 13);
		if (err == ERR_OK) {
			/* Put the connection into LISTEN state */
			netconn_connect(conn, TimeNistIP, 13);
			printf("\r\n Get the network time \r\n");
			timer_nist_serve(conn);
			netconn_delete(conn);
			xQueueControl.xState = TASK_RUNNING;
			xQueueSend(xQueueControlHandle, &xQueueControl, 10);
			while (1) {
				osDelay(1);
			}
		} else {
			printf("\r\n Failled to the network time \r\n");
		}
	}
	//	char *repMess = "Hi am BAC-D01";
	conn = netconn_new(NETCONN_TCP);
	//	err_t recv_err;
	//	struct netbuf *inbuf;
	//	char* buf;
	//	u16_t buflen;
}
void TcpDiscoverTask(void *arg) {
	printf("\r\n netTcpDiscoverTask \r\n");
	struct netconn *conn, *newconn;
	err_t err, accept_err;
	xQueueControl_t xQueueControl;
	uint8_t uiSysState;
	xQueueControl.xTask = netTcpEchoTask;
	do {
		osDelay(10);
		xQueuePeek(xQueueMessageHandle, &uiSysState, 0);
	} while (uiSysState != SYS_CORE_DISCOV);
	xQueueReceive(xQueueMessageHandle, &uiSysState, 0);
	/* Create a new TCP connection handle */
	conn = netconn_new(NETCONN_TCP);
	uint8_t time_delay_core_counter = 0;
#define port_DEFAULT_WAIT_CORE 100
#define port_MAX_WAIT_CORE 250
	if (conn != NULL) {
		err = netconn_bind(conn, NULL, 6969);
		{
			if (err == ERR_OK) {
				netconn_listen(conn);
				while (1) {
					accept_err = netconn_accept(conn, &newconn);
					if (accept_err == ERR_OK) {

						netTcpEcho(newconn);
						netconn_delete(newconn);
						if (time_delay_core_counter < port_DEFAULT_WAIT_CORE) {
							xQueueControl.xState = TASK_RUNNING;
							xQueueSend(xQueueControlHandle, &xQueueControl, 10);
						} else {
							/*Give Semaphore for mqtt stack restart mqtt server*/
							//xSemaphoreGive(netMqttIpSemaphoreHandle);
//								netMqttIpSemaphoreHandle
						}
					}
					vTaskDelay(100);
					if (time_delay_core_counter < port_DEFAULT_WAIT_CORE) {
						time_delay_core_counter++;
					}

					if ((time_delay_core_counter > port_DEFAULT_WAIT_CORE)
							&& (time_delay_core_counter < port_MAX_WAIT_CORE)) {
						time_delay_core_counter = port_MAX_WAIT_CORE;
						xQueueControl.xState = TASK_ERR_1;
						xQueueSend(xQueueControlHandle, &xQueueControl, 10);
					}
				}
			}
		}
	}
}
//	struct sockaddr_in sAddr;
//	if ( sAddr = socket(AF_INET,SOCK_STREAM, IPPROTO_TCP) <0)
//    {
//        /* Bind failed - Maybe illegal port value or in use. */
//        (void)tcp_close(pxPCBListenOld);
//        bOkay = FALSE;
//    }
