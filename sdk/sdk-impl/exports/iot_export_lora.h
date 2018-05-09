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

#ifndef __IOT_EXPORT_CLOUD_H__
#define __IOT_EXPORT_CLOUD_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "sdk_config.h"

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
} iot_lora_if_t;

extern const iot_lora_if_t LoRa;

#ifdef __cplusplus
}
#endif

#endif

