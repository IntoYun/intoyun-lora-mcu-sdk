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

enum LoRaEvents{
    event_lorawan_status = 1,
    event_lora_radio_status = 2,
};

/** 事件枚举*/
typedef enum
{
    ep_lorawan_join_success     = 1,
    ep_lorawan_join_fail        = 2,
    ep_lorawan_send_success     = 3,
    ep_lorawan_send_fail        = 4,
    ep_lorawan_module_wakeup    = 5,
    ep_lorawan_datapoint        = 6,
    ep_lorawan_custom_data      = 7,
} lorawan_event_type_t;

typedef enum
{
    ep_lora_radio_tx_done              = 1,
    ep_lora_radio_tx_fail              = 2,
    ep_lora_radio_rx_timeout           = 4,
    ep_lora_radio_rx_error             = 5,
    ep_lora_radio_module_wakeup        = 6,
    ep_lora_radio_rx_data              = 7,
}lora_radio_event_type_t;

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


typedef int (*callbackPtr)(int type, const char *buf, int len, void *param);

//数据缓冲区
static int PipeFree(pipe_t *pipe);
static int PipeSize(pipe_t *pipe);
//AT指令解析
bool ProtocolParserInit(void);
void ProtocolPutPipe(uint8_t c);
void ProtocolModuleActiveSendHandle(void);
bool ProtocolSendPlatformData(uint8_t frameType, uint8_t port, const uint8_t *buffer, uint16_t length,uint32_t timeout);
bool ProtocolSendRadioData(const uint8_t *buffer, uint16_t length, uint32_t timeout);

bool ProtocolExecuteRestart(void);
bool ProtocolExecuteRestore(void);
bool ProtocolSetupSystemSleep(uint32_t timeout);
bool ProtocolExecuteDFU(void);
bool ProtocolQueryInfo(device_info_t *info);
bool ProtocolSetupDevice(char *product_id, char *hardware_version, char *software_version);
bool ProtocolSetupProtocolMode(uint8_t mode);
int8_t ProtocolQueryMacClassType(void);
bool ProtocolSetupMacClassType(uint8_t classType);
int ProtocolExecuteMacJoin(uint8_t type, uint32_t timeout);
bool ProtocolQueryMacDeviceAddr(char *devAddr);
bool ProtocolQueryMacDeviceEui(char *deveui);
bool ProtocolQueryMacAppEui(char *appeui);
bool ProtocolSetupMacOTAAParams(char *devEui, char *appEui, char *appKey);
bool ProtocolSetupMacABPParams(char *devAddr, char *nwkSkey, char *appSkey);
bool ProtocolSetupMacPowerIndex(uint8_t index);
int8_t ProtocolQueryMacDatarate(void);
bool ProtocolSetupMacDatarate(uint8_t datarate);
int8_t ProtocolQueryMacAdr(void);
bool ProtocolSetupMacAdr(uint8_t adrEnabled);
uint32_t ProtocolQueryMacChannelFreq(uint8_t channelId);
bool ProtocolSetupMacChannelFreq(uint8_t channelId, uint32_t freq);
bool ProtocolQueryMacChannelDRRange(uint8_t channelId, channel_params *drRange);
bool ProtocolSetupMacChannelDRRange(uint8_t channelId, uint8_t minDR, uint8_t maxDR);
int8_t ProtocolQueryMacChannelEnable(uint8_t channelId);
bool ProtocolSetupMacChannelEnable(uint8_t channelId, uint8_t enable);
uint8_t ProtocolQueryMacConfirmedTrials(void);
bool ProtocolSetupMacConfirmedTrials(uint8_t count);
uint8_t ProtocolQueryMacUncomfirmedTrials(void);
bool ProtocolSetupMacUncomfirmedTrials(uint8_t count);
uint8_t ProtocolQueryMacJoinTrials(void);
bool ProtocolSetupMacJoinTrials(uint8_t count);
uint8_t ProtocolQueryMacMargin(void);
uint8_t ProtocolQueryMacGatewayNumber(void);
uint8_t ProtocolQueryMacSnr(void);
int ProtocolQueryMacRssi(void);
uint32_t ProtocolQueryMacRX1Delay(void);
bool ProtocolSetupMacRX1Delay(uint32_t rx1Ms);
bool ProtocolQueryMacRX2Params(channel_params *rx2Params);
bool ProtocolSetupMacRX2Params(uint8_t datarate, uint32_t freq);
bool ProtocoSetupMacUplinkCount(uint32_t count);
int ProtocolQueryMacUplinkCount(void);
bool ProtocolSetupMacDownlinkCount(uint32_t count);
int ProtocolQueryMacDownlinkCount(void);

bool ProtocolSetupRadioRx(uint32_t rxTimeout);
bool ProtocolExecuteRadioStartCad(void);
int ProtocolQueryRadioSnr(void);
int ProtocolQueryRadioRssi(void);
int ProtocolQueryRadioFreq(void);
bool ProtocolSetupRadioFreq(uint32_t freq);
bool ProtocolSetupRadioMaxPayloadLen(uint8_t maxPayloadLen);
int16_t ProtocolQueryRadioMaxPayloadLen(void);
int8_t ProtocolQueryRadioMode(void);
bool ProtocolSetupRadioMode(uint8_t mode);
bool ProtocolSetupRadioSf(uint8_t sf);
int8_t ProtocolQueryRadioSf(void);
bool ProtocolSetupRadioBw(uint8_t bw);
int8_t ProtocolQueryRadioBw(void);
bool ProtocolSetupRadioCoderate(uint8_t cr);
int8_t ProtocolQueryRadioCoderate(void);
bool ProtocolSetupRadioPreambleLen(uint16_t preambleLen);
int ProtocolQueryRadioPreambleLen(void);
int8_t ProtocolQueryRadioFixLenOn(void);
bool ProtocolSetupRadioFixLenOn(uint8_t fixLenOn);
bool ProtocolSetupRadioCrcEnabled(uint8_t crcEnabled);
int8_t ProtocolQueryRadioCrcEnabled(void);
int8_t ProtocolQueryRadioFreqHopOn(void);
bool ProtocolSetupRadioFreqHopOn(uint8_t hopOn);
uint8_t ProtocolQueryRadioHopPeriod(void);
bool ProtocolSetupRadioHopPeriod(uint8_t hopPeriod);
bool ProtocolSetupRadioIqInverted(uint8_t iqInverted);
int8_t ProtocolQueryRadioIqInverted(void);
bool ProtocolSetupRadioRxContinuous(uint8_t rxContinuous);
int8_t ProtocolQueryRadioRxContinuous(void);
bool ProtocolSetupRadioTxPower(uint8_t txPower);
int8_t ProtocolQueryRadioTxPower(void);
bool ProtocolSetupRadioFixPayloadLen(uint8_t payloadLen);
int16_t ProtocolQueryRadioFixPayloadLen(void);
bool ProtocolSetupRadioSymbTimeout(uint16_t symbTimeout);
int16_t ProtocolQueryRadioSymbTimeout(void);
bool ProtocolExecuteRadioSleep(void);
bool ProtocolSetupRadioWriteRegister(uint8_t addr, uint8_t val);
int8_t ProtocolQueryRadioReadRegister(uint8_t addr);

extern lora_data_t loraBuffer;
#endif
