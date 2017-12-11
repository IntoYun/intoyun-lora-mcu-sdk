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

#ifndef _INTOYUN_DATAPOINT_H
#define _INTOYUN_DATAPOINT_H

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "intoyun_protocol.h"


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

// transmit
typedef enum {
    DP_TRANSMIT_MODE_MANUAL = 0,       // 用户控制发送
    DP_TRANSMIT_MODE_AUTOMATIC,        // 系统自动发送
}dp_transmit_mode_t;

// Permission
typedef enum {
    DP_PERMISSION_UP_ONLY = 0,   //只上送
    DP_PERMISSION_DOWN_ONLY,     //只下发
    DP_PERMISSION_UP_DOWN        //可上送下发
}dp_permission_t;

// Policy
typedef enum {
    DP_POLICY_NONE = 0,         //立即发送
    DP_POLICY_TIMED,            //间隔发送
    DP_POLICY_ON_CHANGE         //改变发送
}dp_policy_t;

typedef enum{
    DATA_TYPE_BOOL = 0,   //bool型
    DATA_TYPE_NUM,        //数值型
    DATA_TYPE_ENUM,       //枚举型
    DATA_TYPE_STRING,     //字符串型
    DATA_TYPE_BINARY      //透传型
}data_type_t;

typedef enum{
    RESULT_DATAPOINT_OLD  = 0,   // 旧数据
    RESULT_DATAPOINT_NEW  = 1,   // 新收取数据
    RESULT_DATAPOINT_NONE = 2,   // 没有该数据点
}read_datapoint_result_t;

//float型属性
typedef struct {
    double minValue;
    double maxValue;
    int resolution;
}number_property_t;

//透传型属性
typedef struct {
    uint8_t *value;
    uint16_t len;
}binary_property_t;

// Property configuration
typedef struct {
    uint16_t dpID;
    data_type_t dataType;
    dp_permission_t permission;
    bool change; //数据是否改变 true 数据有变化
    read_datapoint_result_t readFlag;
    number_property_t numberProperty;
    bool boolValue;
    int32_t numberIntValue;
    double numberDoubleValue;
    int enumValue;
    char *stringValue;
    binary_property_t binaryValue;
}property_conf;

//datapoint control
typedef struct {
    dp_transmit_mode_t datapoint_transmit_mode;  // 数据点发送类型
    uint32_t datapoint_transmit_lapse;           // 数据点自动发送 时间间隔
    long runtime;                                // 数据点间隔发送的运行时间
}datapoint_control_t;

typedef void (*event_handler_t)(system_event_t event, int param, uint8_t *data, uint16_t len);

//System API
void intoyunInit(void);
void intoyunLoop(void);
void intoyunSetEventCallback(event_handler_t loraHandler);
void intoyunQueryInfo(char *moduleVersion, char *moduleType, char *deviceId, uint8_t *at_mode);
void intoyunSetupDevice(char *productId, char *hardVer, char *softVer);
bool intoyunSetupProtocol(uint8_t mode);
bool intoyunExecuteRestart(void);
bool intoyunExecuteRestore(void);
bool intoyunSetupSystemSleep(uint32_t timeout);
bool intoyunExecuteDFU(void);
void intoyunPutPipe(uint8_t value);

//Cloud API
int intoyunExecuteMacJoin(uint8_t type, uint32_t timeout);
int intoyunQueryConnected(void);
void intoyunExecuteDisconnect(void);
bool intoyunQueryDisconnected(void);

void intoyunDatapointControl(dp_transmit_mode_t mode, uint32_t lapse);
int intoyunSendCustomData(uint8_t type,uint8_t port, uint32_t timeout, const uint8_t *buffer, uint16_t len);
int intoyunSendAllDatapointManual(bool confirmed, uint32_t timeout);
void intoyunSendDatapointAutomatic(void);

//定义数据点
void intoyunDefineDatapointBool(const uint16_t dpID, dp_permission_t permission, const bool value);
void intoyunDefineDatapointNumber(const uint16_t dpID, dp_permission_t permission, const double minValue, const double maxValue, const int resolution, const double value);
void intoyunDefineDatapointEnum(const uint16_t dpID, dp_permission_t permission, const int value);
void intoyunDefineDatapointString(const uint16_t dpID, dp_permission_t permission, const char *value);
void intoyunDefineDatapointBinary(const uint16_t dpID, dp_permission_t permission, const uint8_t *value, const uint16_t len);

//读取数据点
read_datapoint_result_t intoyunReadDatapointBool(const uint16_t dpID, bool *value);
read_datapoint_result_t intoyunReadDatapointNumberInt32(const uint16_t dpID, int32_t *value);
read_datapoint_result_t intoyunReadDatapointNumberDouble(const uint16_t dpID, double *value);
read_datapoint_result_t intoyunReadDatapointEnum(const uint16_t dpID, int *value);
read_datapoint_result_t intoyunReadDatapointString(const uint16_t dpID, char *value);
read_datapoint_result_t intoyunReadDatapointBinary(const uint16_t dpID, uint8_t *value, uint16_t *len);

//写入数据点值
void intoyunWriteDatapointBool(const uint16_t dpID, bool value);
void intoyunWriteDatapointNumberInt32(const uint16_t dpID, int32_t value);
void intoyunWriteDatapointNumberDouble(const uint16_t dpID, double value);
void intoyunWriteDatapointEnum(const uint16_t dpID, int value);
void intoyunWriteDatapointString(const uint16_t dpID, const char *value);
void intoyunWriteDatapointBinary(const uint16_t dpID, const uint8_t *value, uint16_t len);

//发送数据点
int intoyunSendDatapointBool(const uint16_t dpID, bool value,bool confirmed, uint16_t timeout);
int intoyunSendDatapointNumberInt32(const uint16_t dpID, int32_t value,bool confirmed, uint16_t timeout);
int intoyunSendDatapointNumberDouble(const uint16_t dpID, double value,bool confirmed, uint16_t timeout);
int intoyunSendDatapointEnum(const uint16_t dpID, int value,bool confirmed, uint16_t timeout);
int intoyunSendDatapointString(const uint16_t dpID, const char *value,bool confirmed, uint16_t timeout);
int intoyunSendDatapointBinary(const uint16_t dpID, const uint8_t *value, uint16_t len,bool confirmed, uint16_t timeout);

//解析数据点
void intoyunParseReceiveDatapoints(const uint8_t *payload, uint32_t len, uint8_t *customData);

//LoRaWan API
int intoyunSendConfirmed(uint8_t port, uint8_t *buffer, uint16_t len, uint16_t timeout);    //发送确认帧
int intoyunSendUnconfirmed(uint8_t port, uint8_t *buffer, uint16_t len, uint16_t timeout);  //发送无需确认帧
int8_t intoyunQueryMacSendStatus(void);
uint16_t intoyunMacReceive(uint8_t *buffer, uint16_t length, int *rssi); //返回接收数据
bool intoyunQueryStatus(uint8_t *netStatus, uint8_t *sendStatus);
int8_t intoyunQueryMacClassType(void);
bool intoyunSetupMacClassType(uint8_t type);
bool intoyunQueryMacDeviceAddr(char *devAddr);
bool intoyunQueryMacDeviceEui(char *devEui);
bool intoyunQueryMacAppEui(char *appEui);
bool intoyunSetupMacOTAAParams(char *devEui, char *appEui, char *appKey);
bool intoyunSetupMacABPParams(char *devAddr, char *nwkSkey, char *appSkey);
bool intoyunSetupMacPowerIndex(uint8_t index);
int8_t intoyunQueryMacDatarate(void);
bool intoyunSetupMacDatarate(uint8_t datarate);
bool intoyunQueryMacAdr(void);
bool intoyunSetupMacAdr(bool enable);
bool intoyunSetupMacDutyCyclePrescaler(uint16_t dutyCycle);
uint16_t intoyunQueryMacDutyCyclePrescaler(void);
uint32_t intoyunQueryMacChannelFreq(uint8_t channelId);
bool intoyunSetupMacChannelFreq(uint8_t channelId, uint32_t freq);
bool intoyunQueryMacChannelDRRange(uint8_t channelId, uint8_t *minDR, uint8_t *maxDR);
bool intoyunSetupMacChannelDRRange(uint8_t channelId, uint8_t minDR, uint8_t maxDR);
bool intoyunQueryMacChannelEnable(uint8_t channelId);
bool intoyunSetupMacChannelEnable(uint8_t channelId, bool enable);
uint8_t intoyunQueryMacConfirmedTrials(void);
bool intoyunSetupMacConfirmedTrials(uint8_t count);
uint8_t intoyunQueryMacUncomfirmedTrials(void);
bool intoyunSetupMacUnconfirmedTrials(uint8_t count);
uint8_t intoyunQueryMacJoinTrials(void);
bool intoyunSetupMacJoinTrials(uint8_t count);
uint8_t intoyunQueryMacMargin(void);
uint8_t intoyunQueryMacGatewayNumber(void);
uint8_t intoyunQueryMacSnr(void);
int16_t intoyunQueryMacRssi(void);
uint16_t intoyunQueryMacRX1Delay(void);
bool intoyunSetupMacRX1Delay(uint16_t delay);
bool intoyunQueryMacRX2Params(uint8_t *datarate, uint32_t *freq);
bool intoyunSetupMacRX2Params(uint8_t datarate, uint32_t freq);
bool intoyunSetupMacUplinkCount(uint32_t count);
int intoyunQueryMacUplinkCount(void);
bool intoyunSetupMacDownlinkCount(uint32_t count);
int intoyunQueryMacDownlinkCount(void);

//LoRa API
int8_t intoyunQueryRadioSendStatus(void);
bool intoyunSetupRadioRx(uint32_t rxTimeout);
uint16_t intoyunRadioRx(uint8_t *buffer, uint16_t length, int *rssi); //获取接收到的数据
bool intoyunExecuteRadioStartCad(void);
uint8_t intoyunQueryRadioSnr(void);
int16_t intoyunQueryRadioRssi(void);
int intoyunQueryRadioFreq(void);
bool intoyunSetupRadioFreq(uint32_t freq);
bool intoyunSetupRadioMaxPayloadLen(uint8_t maxPayloadLen);
int16_t intoyunQueryRadioMaxPayloadLen(void);
int8_t intoyunQueryRadioMode(void);
bool intoyunSetupRadioMode(uint8_t mode);
bool intoyunSetupRadioSf(uint8_t sf);
int8_t intoyunQueryRadioSf(void);
bool intoyunSetupRadioBw(uint8_t bw);
int8_t intoyunQueryRadioBw(void);
bool intoyunSetupRadioCoderate(uint8_t cr);
int8_t intoyunQueryRadioCoderate(void);
bool intoyunSetupRadioPreambleLen(uint16_t preambleLen);
int intoyunQueryRadioPreambleLen(void);
bool intoyunQueryRadioFixLenOn(void);
bool intoyunSetupRadioFixLenOn(bool fixLenOn);
bool intoyunSetupRadioCrcEnabled(bool crcEnabled);
bool intoyunQueryRadioCrcEnabled(void);
bool intoyunQueryRadioFreqHopOn(void);
bool intoyunSetupRadioFreqHopOn(bool freqHopOn);
uint8_t intoyunQueryRadioHopPeriod(void);
bool intoyunSetupRadioHopPeriod(uint8_t hopPeriod);
bool intoyunSetupRadioIqInverted(bool iqInverted);
bool intoyunQueryRadioIqInverted(void);
bool intoyunSetupRadioRxContinuous(bool rxContinuous);
bool intoyunQueryRadioRxContinuous(void);
bool intoyunSetupRadioTxPower(int8_t txPower);
int8_t intoyunQueryRadioTxPower(void);
bool intoyunSetupRadioFixPayloadLen(uint8_t payloadLen);
int16_t intoyunQueryRadioFixPayloadLen(void);
bool intoyunSetupRadioSymbTimeout(uint16_t symbTimeout);
int16_t intoyunQueryRadioSymbTimeout(void);
bool intoyunExecuteRadioSleep(void);
bool intoyunSetupRadioWriteRegister(uint8_t addr, uint8_t val);
int8_t intoyunQueryRadioReadRegister(uint8_t addr);
int intoyunRadioSendData(const uint8_t *buffer, uint8_t len,uint32_t timeout);

extern event_handler_t loraEventHandler;
extern int lorawanJoinStatus;
extern uint8_t loraSendResult;
extern int8_t loraSendStatus;

#endif /*_DATAPOINT_H*/
