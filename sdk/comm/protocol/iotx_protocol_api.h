/**
 ******************************************************************************
  Copyright (c) 2013-2014 IntoRobot Team.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
  ******************************************************************************
*/
//AT指令协议数据处理
#ifndef _INTOYUN_PROCOTOL_H
#define _INTOYUN_PROCOTOL_H

#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>

/** 事件类型*/
typedef enum {
    event_cloud_data                   = 1,
    event_lorawan_status               = 2,
    event_lora_radio_status            = 3,
} system_event_t;

/** 事件枚举*/
enum SystemEventsParam {
    //lorawan
    ep_lorawan_join_success            = 1,
    ep_lorawan_join_fail               = 2,
    ep_lorawan_send_success            = 3,
    ep_lorawan_send_fail               = 4,
    ep_lorawan_module_wakeup           = 5,

    //cloud data
    ep_cloud_data_raw                  = 1,  //原始数据 事件
    ep_cloud_data_datapoint            = 2,  //数据点数据协议处理 事件
    ep_cloud_data_custom               = 3,  //自定义数据协议处理 事件

    //lora radio
    ep_lora_radio_tx_done              = 1,
    ep_lora_radio_tx_fail              = 2,
    ep_lora_radio_rx_done              = 3,
    ep_lora_radio_rx_timeout           = 4,
    ep_lora_radio_rx_error             = 5,
    ep_lora_radio_module_wakeup        = 6,
};

enum {
    // waitFinalResp Responses
    NOT_FOUND     =  0,
    WAIT          = -1, // TIMEOUT
    RESP_OK       = -2,
    RESP_ERROR    = -3,
    RESP_FAIL     = -4,
    RESP_PROMPT   = -5,
    RESP_ABORTED  = -6,
    RESP_BUSY     = -7,
    RESP_SENDING  = -8,

    // getLine Responses
    #define LENGTH(x)  (x & 0x00FFFF) //!< extract/mask the length
    #define TYPE(x)    (x & 0xFF0000) //!< extract/mask the type

    TYPE_UNKNOWN            = 0x000000,
    TYPE_OK                 = 0x110000,
    TYPE_ERROR              = 0x120000,
    TYPE_FAIL               = 0x130000,
    TYPE_CONNECT            = 0x210000,
    TYPE_UNLINK             = 0x220000,
    TYPE_CONNECTCLOSTED     = 0x230000,
    TYPE_DHCP               = 0x240000,
    TYPE_DISCONNECT         = 0x250000,
    TYPE_BUSY               = 0x260000,
    TYPE_SMARTCONFIG        = 0x270000,
    TYPE_SENDING            = 0x280000,
    TYPE_PROMPT             = 0x300000,
    TYPE_PLUS               = 0x400000,
    TYPE_TEXT               = 0x500000,
    TYPE_ABORTED            = 0x600000,

    // special timout constant
    TIMEOUT_BLOCKING = 0x7FFFFFFF
};


#define AT_ERROR  -1
#define CUSTOMER_DEFINE_DATA     0x32
#define INTOYUN_DATAPOINT_DATA   0x31


typedef struct {
    char *_b;        //!< buffer
    char *_a;        //!< allocated buffer
    int _s;          //!< size of buffer (s - 1) elements can be stored
    volatile int _w; //!< write index
    volatile int _r; //!< read index
    int _o;          //!< offest index used by parsing functions

}pipe_t;


typedef struct{
    char module_version[10];//模组版本号
    char module_type[10];//模组类型
    char device_id[32];//设备ID
    uint8_t at_mode;//注册类型
    char product_id[32];//产品ID
    char product_secret[32];//产品秘钥
    char hardware_version[10];//板子硬件版本
    char software_version[10];//板子软件版本
    char activation_code[32];//设备激活码
    char access_token[32];//设备秘钥
}device_info_t;

typedef struct{
    uint32_t freq;
    uint8_t datarate;
    uint8_t minDR;
    uint8_t maxDR;
    uint8_t channelEnabled;
}channel_params;

typedef struct {
    bool available;//是否接收到数据 true有效
    uint16_t bufferSize; //接收的数据长度
    uint8_t buffer[256]; //缓存大小
    int rssi;
}lora_data_t;


#define LORA_SEND_SUCCESS    (0)
#define LORA_SEND_FAIL       (-1)
#define LORA_SENDING         (1)

#define LORAWAN_JOIN_SUCCESS    (0)
#define LORAWAN_JOIN_FAIL       (-1)
#define LORAWAN_JOINING         (1)

#define LORAWAN_JOIN_TIMEOUT    180
#define LORAWAN_SEND_TIMEOUT    120
#define LORA_RADIO_SEND_TIMEOUT 3000

//速率
#define DR_0        0  // SF12 - BW125
#define DR_1        1  // SF11 - BW125
#define DR_2        2  // SF10 - BW125
#define DR_3        3  // SF9  - BW125
#define DR_4        4  // SF8  - BW125
#define DR_5        5  // SF7  - BW125

typedef enum {
    CHANGE,
    RISING,
    FALLING
} InterruptMode;

typedef enum eDeviceClass
{
    CLASS_A,
    CLASS_B,
    CLASS_C,
}DeviceClass_t;

typedef enum
{
    JOIN_ABORT = 1,
    JOIN_ABP   = 2,
    JOIN_OTAA  = 3,
}join_mode_t;

typedef enum
{
    PROTOCOL_LORAWAN = 0,
    PROTOCOL_P2P,
}protocol_mode_t;

typedef int (*callbackPtr)(int type, const char *buf, int len, void *param);
typedef void (*recCallback_t)(uint8_t *data, uint32_t len);

extern uint8_t loraSendResult;
extern int lorawanJoinStatus; //0 连接平台 1 连接中 -1 未连接
extern int8_t loraSendStatus; //数据发送状态 0 成功 1 发送中 -1 失败

//数据缓冲区
static int PipeFree(pipe_t *pipe);
static int PipeSize(pipe_t *pipe);
//AT指令解析
bool ProtocolParserInit(void);
void ProtocolPutPipe(uint8_t c);
void ProtocolModuleActiveSendHandle(void);
bool IOT_Protocol_SendPlatformData(uint8_t frameType, uint8_t port, const uint8_t *buffer, uint16_t length,uint32_t timeout);
bool IOT_Protocol_SendRadioData(const uint8_t *buffer, uint16_t length, uint32_t timeout);

bool IOT_Protocol_Reboot(void);
bool IOT_Protocol_Restore(void);
bool IOT_Protocol_SystemSleep(char *pin, uint8_t edgeTriggerMode, uint32_t timeout);
bool IOT_Protocol_EnterDFU(void);
bool IOT_Protocol_GetInfo(device_info_t *info);
bool IOT_Protocol_SetDevice(char *product_id, char *hardware_version, char *software_version);
bool IOT_Protocol_SetProtocolMode(uint8_t mode);
int8_t IOT_Protocol_GetMacClassType(void);
bool IOT_Protocol_SetMacClassType(uint8_t classType);
int IOT_Protocol_MacJoin(uint8_t type, uint32_t timeout);
bool IOT_Protocol_GetMacDeviceAddr(char *devAddr);
bool IOT_Protocol_GetMacDeviceEui(char *deveui);
bool IOT_Protocol_GetMacAppEui(char *appeui);
bool IOT_Protocol_SetMacOTAAParams(char *devEui, char *appEui, char *appKey);
bool IOT_Protocol_SetMacABPParams(char *devAddr, char *nwkSkey, char *appSkey);
bool IOT_Protocol_SetMacPowerIndex(uint8_t index);
int8_t IOT_Protocol_GetMacDatarate(void);
bool IOT_Protocol_SetMacDatarate(uint8_t datarate);
int8_t IOT_Protocol_GetMacAdr(void);
bool IOT_Protocol_SetMacAdr(uint8_t adrEnabled);
bool IOT_Protocol_SetMacDutyCyclePrescaler(uint16_t dutyCycle);
uint16_t IOT_Protocol_GetMacDutyCyclePrescaler(void);
uint32_t IOT_Protocol_GetMacChannelFreq(uint8_t channelId);
bool IOT_Protocol_SetMacChannelFreq(uint8_t channelId, uint32_t freq);
bool IOT_Protocol_GetMacChannelDRRange(uint8_t channelId, channel_params *drRange);
bool IOT_Protocol_SetMacChannelDRRange(uint8_t channelId, uint8_t minDR, uint8_t maxDR);
int8_t IOT_Protocol_GetMacChannelEnable(uint8_t channelId);
bool IOT_Protocol_SetMacChannelEnable(uint8_t channelId, uint8_t enable);
uint8_t IOT_Protocol_GetMacConfirmedTrials(void);
bool IOT_Protocol_SetMacConfirmedTrials(uint8_t count);
uint8_t IOT_Protocol_GetMacUncomfirmedTrials(void);
bool IOT_Protocol_SetMacUncomfirmedTrials(uint8_t count);
uint8_t IOT_Protocol_GetMacJoinTrials(void);
bool IOT_Protocol_SetMacJoinTrials(uint8_t count);
uint8_t IOT_Protocol_GetMacMargin(void);
uint8_t IOT_Protocol_GetMacGatewayNumber(void);
uint8_t IOT_Protocol_GetMacSnr(void);
int IOT_Protocol_GetMacRssi(void);
uint32_t IOT_Protocol_GetMacRX1Delay(void);
bool IOT_Protocol_SetMacRX1Delay(uint32_t rx1Ms);
bool IOT_Protocol_GetMacRX2Params(channel_params *rx2Params);
bool IOT_Protocol_SetMacRX2Params(uint8_t datarate, uint32_t freq);
bool IOT_Protocol_SetMacUplinkCount(uint32_t count);
int IOT_Protocol_GetMacUplinkCount(void);
bool IOT_Protocol_SetMacDownlinkCount(uint32_t count);
int IOT_Protocol_GetMacDownlinkCount(void);

bool IOT_Protocol_SetRadioRx(uint32_t rxTimeout);
bool IOT_Protocol_RadioStartCad(void);
int IOT_Protocol_GetRadioSnr(void);
int IOT_Protocol_GetRadioRssi(void);
int IOT_Protocol_GetRadioFreq(void);
bool IOT_Protocol_SetRadioFreq(uint32_t freq);
bool IOT_Protocol_SetRadioMaxPayloadLen(uint8_t maxPayloadLen);
int16_t IOT_Protocol_GetRadioMaxPayloadLen(void);
int8_t IOT_Protocol_GetRadioMode(void);
bool IOT_Protocol_SetRadioMode(uint8_t mode);
bool IOT_Protocol_SetRadioSf(uint8_t sf);
int8_t IOT_Protocol_GetRadioSf(void);
bool IOT_Protocol_SetRadioBw(uint8_t bw);
int8_t IOT_Protocol_GetRadioBw(void);
bool IOT_Protocol_SetRadioCoderate(uint8_t cr);
int8_t IOT_Protocol_GetRadioCoderate(void);
bool IOT_Protocol_SetRadioPreambleLen(uint16_t preambleLen);
int IOT_Protocol_GetRadioPreambleLen(void);
int8_t IOT_Protocol_GetRadioFixLenOn(void);
bool IOT_Protocol_SetRadioFixLenOn(uint8_t fixLenOn);
bool IOT_Protocol_SetRadioCrcEnabled(uint8_t crcEnabled);
int8_t IOT_Protocol_GetRadioCrcEnabled(void);
int8_t IOT_Protocol_GetRadioFreqHopOn(void);
bool IOT_Protocol_SetRadioFreqHopOn(uint8_t hopOn);
uint8_t IOT_Protocol_GetRadioHopPeriod(void);
bool IOT_Protocol_SetRadioHopPeriod(uint8_t hopPeriod);
bool IOT_Protocol_SetRadioIqInverted(uint8_t iqInverted);
int8_t IOT_Protocol_GetRadioIqInverted(void);
bool IOT_Protocol_SetRadioRxContinuous(uint8_t rxContinuous);
int8_t IOT_Protocol_GetRadioRxContinuous(void);
bool IOT_Protocol_SetRadioTxPower(uint8_t txPower);
int8_t IOT_Protocol_GetRadioTxPower(void);
bool IOT_Protocol_SetRadioFixPayloadLen(uint8_t payloadLen);
int16_t IOT_Protocol_GetRadioFixPayloadLen(void);
bool IOT_Protocol_SetRadioSymbTimeout(uint16_t symbTimeout);
int16_t IOT_Protocol_GetRadioSymbTimeout(void);
bool IOT_Protocol_RadioSleep(void);
bool IOT_Protocol_SetRadioWriteRegister(uint8_t addr, uint8_t val);
int8_t IOT_Protocol_GetRadioReadRegister(uint8_t addr);

int IOT_Protocol_SendData(uint8_t frameType,uint8_t port, const uint8_t *buffer, uint16_t len,uint16_t timeout);
bool IOT_Protocol_loop(void);
void IOT_Protocol_SetRevCallback(recCallback_t handler);

extern lora_data_t loraBuffer;
#endif
