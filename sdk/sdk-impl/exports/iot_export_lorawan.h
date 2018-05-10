/*
 * Copyright (c) 2013-2018 Molmc Group. All rights reserved.
 * License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef __IOT_EXPORT_LORAWAN_H__
#define __IOT_EXPORT_LORAWAN_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "sdk_config.h"
#include "iotx_lorawan_api.h"

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
} iot_lorawan_if_t;

extern const iot_lorawan_if_t LoRaWan;

#ifdef __cplusplus
}
#endif

#endif

