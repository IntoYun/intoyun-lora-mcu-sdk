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

#include "iot_import.h"
#include "iotx_protocol_api.h"
#include "iotx_lorawan_api.h"


int IOT_LoRaWAN_SendConfirmed(uint8_t port, uint8_t *buffer, uint16_t len, uint16_t timeout)
{
    return intoyunTransmitData(0,port,buffer,len,timeout);
}

int IOT_LoRaWAN_SendUnconfirmed(uint8_t port, uint8_t *buffer, uint16_t len, uint16_t timeout)
{
    return intoyunTransmitData(1,port,buffer,len,timeout);
}

int8_t IOT_LoRaWAN_GetMacSendStatus(void)
{
    return loraSendStatus;
}

uint16_t IOT_LoRaWAN_MacReceive(uint8_t *buffer, uint16_t length, int *rssi)
{
    uint16_t size = 0;
    if(loraBuffer.available) {
        loraBuffer.available = false;
        if(length < loraBuffer.bufferSize) {
            size = length;
        } else {
            size = loraBuffer.bufferSize;
        }
        *rssi = loraBuffer.rssi;
        memcpy(buffer, loraBuffer.buffer, size);
    }
    return size;
}

int8_t IOT_LoRaWAN_GetMacClassType(void)
{
    return IOT_Protocol_GetMacClassType();
}

bool IOT_LoRaWAN_SetMacClassType(uint8_t type)
{
    return IOT_Protocol_SetMacClassType(type);
}

bool IOT_LoRaWAN_GetMacDeviceAddr(char *devAddr)
{
    return IOT_Protocol_GetMacDeviceAddr(devAddr);
}

bool IOT_LoRaWAN_GetMacDeviceEui(char *devEui)
{
    return IOT_Protocol_GetMacDeviceEui(devEui);
}

bool IOT_LoRaWAN_GetMacAppEui(char *appEui)
{
    return IOT_Protocol_GetMacAppEui(appEui);
}

bool IOT_LoRaWAN_SetMacOTAAParams(char *devEui, char *appEui, char *appKey)
{
    return IOT_Protocol_SetMacOTAAParams(devEui,appEui,appKey);
}

bool IOT_LoRaWAN_SetMacABPParams(char *devAddr, char *nwkSkey, char *appSkey)
{
    return IOT_Protocol_SetMacABPParams(devAddr,nwkSkey,appSkey);
}

bool IOT_LoRaWAN_SetMacPowerIndex(uint8_t index)
{
    return IOT_Protocol_SetMacPowerIndex(index);
}

int8_t IOT_LoRaWAN_GetMacDatarate(void)
{
    return IOT_Protocol_GetMacDatarate();
}

bool IOT_LoRaWAN_SetMacDatarate(uint8_t datarate)
{
    if(datarate > 5) {
        return false;
    }
    return IOT_Protocol_SetMacDatarate(datarate);
}

bool IOT_LoRaWAN_SetMacAdr(bool enable)
{
    if(enable) {
        return IOT_Protocol_SetMacAdr(1);
    } else {
        return IOT_Protocol_SetMacAdr(0);
    }
}

bool IOT_LoRaWAN_GetMacAdr(void)
{
    if(IOT_Protocol_GetMacAdr() == 1) {
        return true;
    } else {
        return false;
    }
}

bool IOT_LoRaWAN_SetMacDutyCyclePrescaler(uint16_t dutyCycle)
{
    return IOT_Protocol_SetMacDutyCyclePrescaler(dutyCycle);
}

uint16_t IOT_LoRaWAN_GetMacDutyCyclePrescaler(void)
{
    return IOT_Protocol_GetMacDutyCyclePrescaler();
}

uint32_t IOT_LoRaWAN_GetMacChannelFreq(uint8_t channelId)
{
    if(channelId > 15) {
        return false;
    }
    return IOT_Protocol_GetMacChannelFreq(channelId);
}

bool IOT_LoRaWAN_SetMacChannelFreq(uint8_t channelId, uint32_t freq)
{
    if(channelId > 15 || freq > 525000000 || freq < 137000000) {
        return false;
    }
    return IOT_Protocol_SetMacChannelFreq(channelId,freq);
}

bool IOT_LoRaWAN_GetMacChannelDRRange(uint8_t channelId, uint8_t *minDR, uint8_t *maxDR)
{
    if(channelId > 15 ) {
        return false;
    }
    channel_params *drRange;
    if(IOT_Protocol_GetMacChannelDRRange(channelId,drRange)) {
        *minDR = drRange->minDR;
        *maxDR = drRange->maxDR;
        return true;
    } else {
        return false;
    }
}

bool IOT_LoRaWAN_SetMacChannelDRRange(uint8_t channelId, uint8_t minDR, uint8_t maxDR)
{
    if(channelId > 15 || maxDR > 5) {
        return false;
    }
    return IOT_Protocol_SetMacChannelDRRange(channelId,minDR,maxDR);
}

bool IOT_LoRaWAN_GetMacChannelEnable(uint8_t channelId)
{
    if(channelId > 15) {
        return false;
    }

    if(IOT_Protocol_GetMacChannelEnable(channelId) == 1) {
        return true;
    } else {
        return false;
    }
}

bool IOT_LoRaWAN_SetMacChannelEnable(uint8_t channelId, bool enable)
{
    uint8_t temp;
    if(channelId > 15) {
        return false;
    }
    if(enable) {
        temp = 1;
    } else {
        temp = 0;
    }
    return IOT_Protocol_SetMacChannelEnable(channelId,temp);
}

uint8_t IOT_LoRaWAN_GetMacConfirmedTrials(void)
{
    return IOT_Protocol_GetMacConfirmedTrials();
}

bool IOT_LoRaWAN_SetMacConfirmedTrials(uint8_t count)
{
    if(count > 8) {
        return false;
    }
    return IOT_Protocol_SetMacConfirmedTrials(count);
}

uint8_t IOT_LoRaWAN_GetMacUncomfirmedTrials(void)
{
    return IOT_Protocol_GetMacUncomfirmedTrials();
}

bool IOT_LoRaWAN_SetMacUnconfirmedTrials(uint8_t count)
{
    if(count > 15) {
        return false;
    }
    return IOT_Protocol_SetMacUncomfirmedTrials(count);
}

uint8_t IOT_LoRaWAN_GetMacJoinTrials(void)
{
    return IOT_Protocol_GetMacJoinTrials();
}

bool IOT_LoRaWAN_SetMacJoinTrials(uint8_t count)
{
    return IOT_Protocol_SetMacJoinTrials(count);
}

uint8_t IOT_LoRaWAN_GetMacMargin(void)
{
    return IOT_Protocol_GetMacMargin();
}

uint8_t IOT_LoRaWAN_GetMacGatewayNumber(void)
{
    return IOT_Protocol_GetMacGatewayNumber();
}

uint8_t IOT_LoRaWAN_GetMacSnr(void)
{
    return IOT_Protocol_GetMacSnr();
}

int16_t IOT_LoRaWAN_GetMacRssi(void)
{
    return loraBuffer.rssi;
}

uint16_t IOT_LoRaWAN_GetMacRX1Delay(void)
{
    return IOT_Protocol_GetMacRX1Delay();
}

bool IOT_LoRaWAN_SetMacRX1Delay(uint16_t delay)
{
    return IOT_Protocol_SetMacRX1Delay(delay);
}

bool IOT_LoRaWAN_GetMacRX2Params(uint8_t *datarate, uint32_t *freq)
{
    channel_params *rx2Params;
    if(IOT_Protocol_GetMacRX2Params(rx2Params)) {
        (*datarate) = rx2Params->datarate;
        (*freq) = rx2Params->freq;
        return true;
    } else {
        return false;
    }
}

bool IOT_LoRaWAN_SetMacRX2Params(uint8_t datarate, uint32_t freq)
{
    if(freq > 525000000 || freq < 137000000 || datarate > 5) {
        return false;
    }
    return IOT_Protocol_SetMacRX2Params(datarate,freq);
}

bool IOT_LoRaWAN_SetMacUplinkCount(uint32_t count)
{
    return ProtocoSetupMacUplinkCount(count);
}

int IOT_LoRaWAN_GetMacUplinkCount(void)
{
    return IOT_Protocol_GetMacUplinkCount();
}

bool IOT_LoRaWAN_SetMacDownlinkCount(uint32_t count)
{
    return IOT_Protocol_SetMacDownlinkCount(count);
}

int IOT_LoRaWAN_GetMacDownlinkCount(void)
{
    return IOT_Protocol_GetMacDownlinkCount();
}

