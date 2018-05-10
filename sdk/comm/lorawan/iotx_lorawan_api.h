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

#ifndef __IOTX_LORAWAN_API_H__
#define __IOTX_LORAWAN_API_H__

#ifdef __cplusplus
extern "C"
{
#endif

int IOT_LoRaWAN_SendConfirmed(uint8_t port, uint8_t *buffer, uint16_t len, uint16_t timeout);
int IOT_LoRaWAN_SendUnconfirmed(uint8_t port, uint8_t *buffer, uint16_t len, uint16_t timeout);
int8_t IOT_LoRaWAN_GetMacSendStatus(void);
uint16_t IOT_LoRaWAN_MacReceive(uint8_t *buffer, uint16_t length, int *rssi);
int8_t IOT_LoRaWAN_GetMacClassType(void);
bool IOT_LoRaWAN_SetMacClassType(uint8_t type);
bool IOT_LoRaWAN_GetMacDeviceAddr(char *devAddr);
bool IOT_LoRaWAN_GetMacDeviceEui(char *devEui);
bool IOT_LoRaWAN_GetMacAppEui(char *appEui);
bool IOT_LoRaWAN_SetMacOTAAParams(char *devEui, char *appEui, char *appKey);
bool IOT_LoRaWAN_SetMacABPParams(char *devAddr, char *nwkSkey, char *appSkey);
bool IOT_LoRaWAN_SetMacPowerIndex(uint8_t index);
int8_t IOT_LoRaWAN_GetMacDatarate(void);
bool IOT_LoRaWAN_SetMacDatarate(uint8_t datarate);
bool IOT_LoRaWAN_SetMacAdr(bool enable);
bool IOT_LoRaWAN_GetMacAdr(void);
bool IOT_LoRaWAN_SetMacDutyCyclePrescaler(uint16_t dutyCycle);
uint16_t IOT_LoRaWAN_GetMacDutyCyclePrescaler(void);
uint32_t IOT_LoRaWAN_GetMacChannelFreq(uint8_t channelId);
bool IOT_LoRaWAN_SetMacChannelFreq(uint8_t channelId, uint32_t freq);
bool IOT_LoRaWAN_GetMacChannelDRRange(uint8_t channelId, uint8_t *minDR, uint8_t *maxDR);
bool IOT_LoRaWAN_SetMacChannelDRRange(uint8_t channelId, uint8_t minDR, uint8_t maxDR);
bool IOT_LoRaWAN_GetMacChannelEnable(uint8_t channelId);
bool IOT_LoRaWAN_SetMacChannelEnable(uint8_t channelId, bool enable);
uint8_t IOT_LoRaWAN_GetMacConfirmedTrials(void);
bool IOT_LoRaWAN_SetMacConfirmedTrials(uint8_t count);
uint8_t IOT_LoRaWAN_GetMacUncomfirmedTrials(void);
bool IOT_LoRaWAN_SetMacUnconfirmedTrials(uint8_t count);
uint8_t IOT_LoRaWAN_GetMacJoinTrials(void);
bool IOT_LoRaWAN_SetMacJoinTrials(uint8_t count);
uint8_t IOT_LoRaWAN_GetMacMargin(void);
uint8_t IOT_LoRaWAN_GetMacGatewayNumber(void);
uint8_t IOT_LoRaWAN_GetMacSnr(void);
int16_t IOT_LoRaWAN_GetMacRssi(void);
uint16_t IOT_LoRaWAN_GetMacRX1Delay(void);
bool IOT_LoRaWAN_SetMacRX1Delay(uint16_t delay);
bool IOT_LoRaWAN_GetMacRX2Params(uint8_t *datarate, uint32_t *freq);
bool IOT_LoRaWAN_SetMacRX2Params(uint8_t datarate, uint32_t freq);
bool IOT_LoRaWAN_SetMacUplinkCount(uint32_t count);
int IOT_LoRaWAN_GetMacUplinkCount(void);
bool IOT_LoRaWAN_SetMacDownlinkCount(uint32_t count);
int IOT_LoRaWAN_GetMacDownlinkCount(void);

#ifdef __cplusplus
}
#endif

#endif

