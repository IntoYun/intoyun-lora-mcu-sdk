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

#ifndef _INTOYUN_INTERFACE_H
#define _INTOYUN_INTERFACE_H

#include "intoyun_datapoint.h"
#include "intoyun_protocol.h"
#include "intoyun_key.h"
#include "intoyun_timer.h"
#include "intoyun_config.h"
#include "intoyun_log.h"

#define UINT_MAX    0xFFFFFFFF

typedef struct
{
    void (*init)(void);
    void (*loop)(void);

    void (*setEventCallback)(event_handler_t handler);
    void (*getModuleInfo)(char *moduleVersion, char *moduleType, char *deviceId, uint8_t *atMode); //获取模块信息
    void (*setDeviceInfo)(char *productId, char *hardVer, char *softVer);  //设置设备信息
    bool (*setProtocol)(uint8_t mode);  //设置协议模式

    bool (*resetModule)(void);
    bool (*restoreModule)(void);
    bool (*sleepModule)(char *pin, InterruptMode edgeTriggerMode, uint32_t timeout);
    bool (*updateModule)(void);
    void (*putPipe)(uint8_t value);

}system_t;

typedef struct
{
    int (*connect)(uint8_t mode, uint32_t timeout); //lorawan连接设置
    int (*connected)(void); //查询lorawan连接状态
    void (*disconnect)(void); //lorawan断开连接
    bool (*disconnected)(void); //查询lorawan是否已经断开连接
    //数据点相关接口
    #ifdef CONFIG_INTOYUN_DATAPOINT
    //定义数据点
    void (*defineDatapointBool)(const uint16_t dpID, dp_permission_t permission, const bool value);
    void (*defineDatapointNumber)(const uint16_t dpID, dp_permission_t permission, const double minValue, const double maxValue, const int resolution, const double value);
    void (*defineDatapointEnum)(const uint16_t dpID, dp_permission_t permission, const int value);
    void (*defineDatapointString)(const uint16_t dpID, dp_permission_t permission, const char *value);
    void (*defineDatapointBinary)(const uint16_t dpID, dp_permission_t permission, const uint8_t *value, const uint16_t len);
    //读取数据点
    read_datapoint_result_t (*readDatapointBool)(const uint16_t dpID, bool *value);
    read_datapoint_result_t (*readDatapointNumberInt32)(const uint16_t dpID, int32_t *value);
    read_datapoint_result_t (*readDatapointNumberDouble)(const uint16_t dpID, double *value);
    read_datapoint_result_t (*readDatapointEnum)(const uint16_t dpID, int *value);
    read_datapoint_result_t (*readDatapointString)(const uint16_t dpID, char *value);
    read_datapoint_result_t (*readDatapointBinary)(const uint16_t dpID, uint8_t *value, uint16_t *len);
    //写数据点
    void (*writeDatapointBool)(const uint16_t dpID, bool value);
    void (*writeDatapointNumberInt32)(const uint16_t dpID, int32_t value);
    void (*writeDatapointNumberDouble)(const uint16_t dpID, double value);
    void (*writeDatapointEnum)(const uint16_t dpID, int value);
    void (*writeDatapointString)(const uint16_t dpID, const char *value);
    void (*writeDatapointBinary)(const uint16_t dpID, const uint8_t *value, uint16_t len);
    //发送数据点值
    int (*sendDatapointBool)(const uint16_t dpID, bool value, bool confirmed, uint16_t timeout);
    int (*sendDatapointNumberInt32)(const uint16_t dpID, int32_t value, bool confirmed, uint16_t timeout);
    int (*sendDatapointNumberDouble)(const uint16_t dpID, double value, bool confirmed, uint16_t timeout);
    int (*sendDatapointEnum)(const uint16_t dpID, int value, bool confirmed, uint16_t timeout);
    int (*sendDatapointString)(const uint16_t dpID, const char *value, bool confirmed, uint16_t timeout);
    int (*sendDatapointBinary)(const uint16_t dpID, const uint8_t *value, uint16_t len, bool confirmed, uint16_t timeout);
    int (*sendDatapointAll)(bool confirmed, uint32_t timeout); //发送全部数据点数据
    #endif
}cloud_t;

typedef struct
{
    //LoRaWan通讯接口
    int (*sendConfirmed)(uint8_t port, uint8_t *buffer, uint16_t len, uint16_t timeout);    //带确认发送   true:发送成功 false:发送失败
    int (*sendUnconfirmed)(uint8_t port, uint8_t *buffer, uint16_t len, uint16_t timeout);  //不带确认发送 true:发送成功 false:发送失败
    int8_t (*sendStatus)(void);
    uint16_t (*receive)(uint8_t *buffer, uint16_t length, int *rssi);                        //返回接收数据
    bool (*setMacClassType)(uint8_t type);
    int8_t (*getMacClassType)(void);
    //LoRaWan参数设置接口
    bool (*getDeviceAddr)(char *devAddr);
    bool (*getDeviceEui)(char *devEui);
    bool (*getAppEui)(char *appEui);
    bool (*setOTAAParams)(char *devEui, char *appEui, char *appKey);
    bool (*setABPParams)(char *devAddr, char *nwkSkey, char *appSkey);
    bool (*setTxPower)(uint8_t index);
    bool (*setDataRate)(uint8_t datarate);
    int8_t (*getDataRate)(void);
    bool (*setAdrOn)(bool enable);
    bool (*getAdrOn)(void);
    bool (*setDutyCyclePrescaler)(uint16_t dutyCycle);
    uint16_t (*getDutyCyclePrescaler)(void);
    bool (*setChannelFreq)(uint8_t channelId, uint32_t freq);
    uint32_t (*getChannelFreq)(uint8_t channelId);
    bool (*setChannelDRRange)(uint8_t channelId, uint8_t minDR, uint8_t maxDR);
    bool (*getChannelDRRange)(uint8_t channelId, uint8_t *minDR, uint8_t *maxDR);
    bool (*setChannelStatus)(uint8_t channelId, bool enable);
    bool (*getChannelStatus)(uint8_t channelId);
    bool (*setConfirmedNbTrials)(uint8_t count);
    uint8_t (*getConfirmedNbTrials)(void);
    bool (*setUnconfirmedNbTrials)(uint8_t count);
    uint8_t (*getUncomfirmedNbTrials)(void);
    bool (*setJoinNbTrials)(uint8_t count);
    uint8_t (*getJoinNbTrials)(void);
    bool (*setUpCounter)(uint32_t count);
    int (*getUpCounter)(void);
    bool (*setDownCounter)(uint32_t count);
    int (*getDownCounter)(void);
    bool (*setRX2Params)(uint8_t datarate, uint32_t freq);
    bool (*getRX2Params)(uint8_t *datarate, uint32_t *freq);
    bool (*setRX1Delay)(uint16_t delay);
    uint16_t (*getRX1Delay)(void);
    uint8_t (*getMargin)(void);
    uint8_t (*getGatewayNumber)(void);
    uint8_t (*getSnr)(void);
    int16_t (*getRssi)(void);
}lorawan_t;

typedef struct
{
    //P2P通讯接口
    int (*radioSend)(const uint8_t *buffer,uint8_t len, uint32_t timeout);
    int8_t (*radioSendStatus)(void);
    bool (*radioStartRx)(uint32_t rxTimeout);
    uint16_t (*radioRx)(uint8_t *buffer, uint16_t length, int *rssi);  //lora射频接收数据
    //P2P参数设置接口
    bool (*radioCad)(void);
    int16_t (*radioGetRssi)(void);
    uint8_t (*radioGetSnr)(void);
    bool (*radioSleep)(void);
    bool (*radioSetFreq)(uint32_t freq);
    int (*radioGetFreq)(void);
    bool (*radioSetMaxPayloadLength)(uint8_t maxPayloadLen);
    int16_t (*radioGetMaxPayloadLength)(void);
    bool (*radioSetModem)(uint8_t mode);
    int8_t (*radioGetModem)(void);
    bool (*radioSetBandwidth)(uint8_t bw);
    int8_t (*radioGetBandwidth)(void);
    bool (*radioSetSF)(uint8_t sf);
    int8_t (*radioGetSF)(void);
    bool (*radioSetCR)(uint8_t coderate);
    int8_t (*radioGetCR)(void);
    bool (*radioSetPreambleLen)(uint16_t preambleLen);
    int (*radioGetPreambleLen)(void);
    bool (*radioSetFixLenOn)(bool fixLenOn);
    bool (*radioGetFixLenOn)(void);
    bool (*radioSetCrcOn)(bool enable);
    bool (*radioGetCrcOn)(void);
    bool (*radioSetFreqHopOn)(bool freqHopOn);
    bool (*radioGetFreqHopOn)(void);
    bool (*radioSetHopPeriod)(uint8_t hopPeriod);
    uint8_t (*radioGetHopPeriod)(void);
    bool (*radioSetIqInverted)(bool iqInverted);
    bool (*radioGetIqInverted)(void);
    bool (*radioSetRxContinuous)(bool rxContinuous);
    bool (*radioGetRxContinuous)(void);
    bool (*radioSetTxPower)(int8_t txPower);
    int8_t (*radioGetTxPower)(void);
    bool (*radioSetFixPayloadLen)(uint8_t payloadLen);
    int16_t (*radioGetFixPayloadLen)(void);
    bool (*radioSetSymbTimeout)(uint16_t symbTimeout);
    int16_t (*radioGetSymbTimeout)(void);
    int8_t (*radioReadReg)(uint8_t addr);
    bool (*radioWriteReg)(uint8_t addr, uint8_t val);
}lora_t;

#ifdef CONFIG_INTOYUN_KEY

typedef struct {
    void (*init)(void);
    void (*setParams)(bool invert, uint32_t debounceTime, uint32_t clickTime, uint32_t pressTime);
    void (*keyRegister)(uint8_t num, cbInitFunc initFunc, cbGetValueFunc getValFunc);
    void (*attachClick)(uint8_t num, cbClickFunc cbFunc);           //注册单击处理函数
    void (*attachDoubleClick)(uint8_t num, cbClickFunc cbFunc);     //注册双击处理函数
    void (*attachLongPressStart)(uint8_t num, cbPressFunc cbFunc);  //注册按下按键处理函数
    void (*attachLongPressStop)(uint8_t num, cbPressFunc cbFunc);   //注册释放按键处理函数
    void (*attachDuringLongPress)(uint8_t num, cbPressFunc cbFunc); //注册按键按下回调函数
    void (*loop)(void);
}keys_t;

extern const keys_t Key;

#endif

#ifdef CONFIG_INTOYUN_TIMER

typedef struct {
    void (*timerRegister)(uint8_t num, uint32_t period, bool oneShot, cbTimerFunc cbFunc);
    void (*changePeriod)(uint8_t num, uint32_t period);
    void (*start)(uint8_t num);
    void (*stop)(uint8_t num);
    void (*reset)(uint8_t num);
    void (*loop)(void);
}timers_t;

extern const timers_t Timer;

#endif

void delay(uint32_t ms);
uint32_t millis(void);
uint32_t timerGetId(void);
bool timerIsEnd(uint32_t timerID, uint32_t time);

extern const system_t System;
extern const cloud_t Cloud;
extern const lorawan_t LoRaWan;
extern const lora_t LoRa;

#endif /*_INTERFACE_H*/
