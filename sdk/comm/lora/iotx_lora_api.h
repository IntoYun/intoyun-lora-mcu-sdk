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

#ifndef __IOTX_LORA_API_H__
#define __IOTX_LORA_API_H__

#ifdef __cplusplus
extern "C"
{
#endif

int8_t IOT_LoRa_GetRadioSendStatus(void);
bool IOT_LoRa_SetRadioRx(uint32_t rxTimeout);
uint16_t IOT_LoRa_RadioRx(uint8_t *buffer, uint16_t length, int *rssi);
bool IOT_LoRa_SetRadioFreq(uint32_t freq);
bool IOT_LoRa_RadioStartCad(void);
uint8_t IOT_LoRa_GetRadioSnr(void);
int16_t IOT_LoRa_GetRadioRssi(void);
int IOT_LoRa_GetRadioFreq(void);
bool IOT_LoRa_SetRadioMaxPayloadLen(uint8_t payloadLen);
int16_t IOT_LoRa_GetRadioMaxPayloadLen(void);
int8_t IOT_LoRa_GetRadioMode(void);
bool IOT_LoRa_SetRadioMode(uint8_t mode);
bool IOT_LoRa_SetRadioSf(uint8_t sf);
int8_t IOT_LoRa_GetRadioSf(void);
bool IOT_LoRa_SetRadioBw(uint8_t bw);
int8_t IOT_LoRa_GetRadioBw(void);
bool IOT_LoRa_SetRadioCoderate(uint8_t coderate);
int8_t IOT_LoRa_GetRadioCoderate(void);
bool IOT_LoRa_SetRadioPreambleLen(uint16_t preambleLen);
int IOT_LoRa_GetRadioPreambleLen(void);
bool IOT_LoRa_GetRadioFixLenOn(void);
bool IOT_LoRa_SetRadioFixLenOn(bool fixLenOn);
bool IOT_LoRa_SetRadioCrcEnabled(bool enable);
bool IOT_LoRa_GetRadioCrcEnabled(void);
bool IOT_LoRa_GetRadioFreqHopOn(void);
bool IOT_LoRa_SetRadioFreqHopOn(bool freqHopOn);
uint8_t IOT_LoRa_GetRadioHopPeriod(void);
bool IOT_LoRa_SetRadioHopPeriod(uint8_t hopPeriod);
bool IOT_LoRa_SetRadioIqInverted(bool iqInverted);
bool IOT_LoRa_GetRadioIqInverted(void);
bool IOT_LoRa_SetRadioRxContinuous(bool rxContinuous);
bool IOT_LoRa_GetRadioRxContinuous(void);
bool IOT_LoRa_SetRadioTxPower(int8_t txPower);
int8_t IOT_LoRa_GetRadioTxPower(void);
bool IOT_LoRa_SetRadioSymbTimeout(uint16_t symbTimeout);
int16_t IOT_LoRa_GetRadioSymbTimeout(void);
bool IOT_LoRa_RadioSleep(void);
bool IOT_LoRa_SetRadioFixPayloadLen(uint8_t payloadLen);
int16_t IOT_LoRa_GetRadioFixPayloadLen(void);
bool IOT_LoRa_SetRadioWriteRegister(uint8_t addr, uint8_t val);
int8_t IOT_LoRa_GetRadioReadRegister(uint8_t addr);
int IOT_LoRa_RadioSendData(const uint8_t *buffer, uint8_t len, uint32_t timeout);

#ifdef __cplusplus
}
#endif

#endif

