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
#include "iotx_lora_api.h"


int8_t IOT_LoRa_GetRadioSendStatus(void)
{
    return loraSendStatus;
}

bool IOT_LoRa_SetRadioRx(uint32_t rxTimeout)
{
    return IOT_Protocol_SetRadioRx(rxTimeout);
}

uint16_t IOT_LoRa_RadioRx(uint8_t *buffer, uint16_t length, int *rssi)
{
    uint16_t size = 0;
    if(loraBuffer.available){
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

bool IOT_LoRa_SetRadioFreq(uint32_t freq)
{
    if(freq > 525000000 || freq < 137000000) {
        return false;
    }
    return IOT_Protocol_SetRadioFreq(freq);
}

bool IOT_LoRa_RadioStartCad(void)
{
    return IOT_Protocol_RadioStartCad();
}

uint8_t IOT_LoRa_GetRadioSnr(void)
{
    return (uint8_t)IOT_Protocol_GetRadioSnr();
}

int16_t IOT_LoRa_GetRadioRssi(void)
{
    return loraBuffer.rssi;
}

int IOT_LoRa_GetRadioFreq(void)
{
    return IOT_Protocol_GetRadioFreq();
}

bool IOT_LoRa_SetRadioMaxPayloadLen(uint8_t payloadLen)
{
    if(payloadLen > 255) {
        return false;
    }
    return IOT_Protocol_SetRadioMaxPayloadLen(payloadLen);
}

int16_t IOT_LoRa_GetRadioMaxPayloadLen(void)
{
    return IOT_Protocol_GetRadioMaxPayloadLen();
}

int8_t IOT_LoRa_GetRadioMode(void)
{
    return IOT_Protocol_GetRadioMode();
}

bool IOT_LoRa_SetRadioMode(uint8_t mode)
{
    if(mode > 2) {
        return false;
    }
    return IOT_Protocol_SetRadioMode(mode);
}

bool IOT_LoRa_SetRadioSf(uint8_t sf)
{
    if(sf < 7 || sf >12) {
        return false;
    }
    return IOT_Protocol_SetRadioSf(sf);
}

int8_t IOT_LoRa_GetRadioSf(void)
{
    return IOT_Protocol_GetRadioSf();
}

bool IOT_LoRa_SetRadioBw(uint8_t bw)
{
    if(bw > 2) {
        return false;
    }
    return IOT_Protocol_SetRadioBw(bw);
}

int8_t IOT_LoRa_GetRadioBw(void)
{
    return IOT_Protocol_GetRadioBw();
}

bool IOT_LoRa_SetRadioCoderate(uint8_t coderate)
{
    if(coderate < 1 || coderate > 4) {
        return false;
    }
    return IOT_Protocol_SetRadioCoderate(coderate);
}

int8_t IOT_LoRa_GetRadioCoderate(void)
{
    return IOT_Protocol_GetRadioCoderate();
}

bool IOT_LoRa_SetRadioPreambleLen(uint16_t preambleLen)
{
    if(preambleLen < 6 || preambleLen > 65535) {
        return false;
    }
    return IOT_Protocol_SetRadioPreambleLen(preambleLen);
}

int IOT_LoRa_GetRadioPreambleLen(void)
{
    return IOT_Protocol_GetRadioPreambleLen();
}

bool IOT_LoRa_GetRadioFixLenOn(void)
{
    if(IOT_Protocol_GetRadioFixLenOn() == 1) {
        return true;
    } else {
        return false;
    }
}

bool IOT_LoRa_SetRadioFixLenOn(bool fixLenOn)
{
    uint8_t temp;
    if(fixLenOn) {
        temp = 1;
    } else {
        temp = 0;
    }
    return IOT_Protocol_SetRadioFixLenOn(temp);
}

bool IOT_LoRa_SetRadioCrcEnabled(bool enable)
{
    if(enable) {
        return IOT_Protocol_SetRadioCrcEnabled(1);
    } else {
        return IOT_Protocol_SetRadioCrcEnabled(0);
    }
}

bool IOT_LoRa_GetRadioCrcEnabled(void)
{
    if(IOT_Protocol_GetRadioCrcEnabled() == 1) {
        return true;
    } else {
        return false;
    }
}

bool IOT_LoRa_GetRadioFreqHopOn(void)
{
    if(IOT_Protocol_GetRadioFreqHopOn() == 1){
        return true;
    }else{
        return false;
    }
}

bool IOT_LoRa_SetRadioFreqHopOn(bool freqHopOn)
{
    uint8_t temp;
    if(freqHopOn) {
        temp = 1;
    } else {
        temp = 0;
    }
    return IOT_Protocol_SetRadioFreqHopOn(temp);
}

uint8_t IOT_LoRa_GetRadioHopPeriod(void)
{
    return IOT_Protocol_GetRadioHopPeriod();
}

bool IOT_LoRa_SetRadioHopPeriod(uint8_t hopPeriod)
{
    return IOT_Protocol_SetRadioFreqHopOn(hopPeriod);
}

bool IOT_LoRa_SetRadioIqInverted(bool iqInverted)
{
    if(iqInverted){
        return IOT_Protocol_SetRadioIqInverted(1);
    } else {
        return IOT_Protocol_SetRadioIqInverted(0);
    }
}

bool IOT_LoRa_GetRadioIqInverted(void)
{
    if(IOT_Protocol_GetRadioIqInverted() == 1) {
        return true;
    } else {
        return false;
    }
}

bool IOT_LoRa_SetRadioRxContinuous(bool rxContinuous)
{
    if(rxContinuous){
        return IOT_Protocol_SetRadioRxContinuous(1);
    }else{
        return IOT_Protocol_SetRadioRxContinuous(0);
    }
}

bool IOT_LoRa_GetRadioRxContinuous(void)
{
    if(IOT_Protocol_GetRadioRxContinuous() == 1) {
        return true;
    } else {
        return false;
    }
}

bool IOT_LoRa_SetRadioTxPower(int8_t txPower)
{
    if(txPower > 20) {
        return false;
    }
    return IOT_Protocol_SetRadioTxPower(txPower);
}

int8_t IOT_LoRa_GetRadioTxPower(void)
{
    return IOT_Protocol_GetRadioTxPower();
}

bool IOT_LoRa_SetRadioSymbTimeout(uint16_t symbTimeout)
{
    if(symbTimeout < 4 || symbTimeout > 1023) {
        return false;
    }
    return IOT_Protocol_SetRadioSymbTimeout(symbTimeout);
}

int16_t IOT_LoRa_GetRadioSymbTimeout(void)
{
    return IOT_Protocol_GetRadioSymbTimeout();
}

bool IOT_LoRa_RadioSleep(void)
{
    return IOT_Protocol_RadioSleep();
}

bool IOT_LoRa_SetRadioFixPayloadLen(uint8_t payloadLen)
{
    if(payloadLen > 255) {
        return false;
    }
    return IOT_Protocol_SetRadioFixPayloadLen(payloadLen);
}

int16_t IOT_LoRa_GetRadioFixPayloadLen(void)
{
    return IOT_Protocol_GetRadioFixPayloadLen();
}

bool IOT_LoRa_SetRadioWriteRegister(uint8_t addr, uint8_t val)
{
    if(addr > 0x70) {
        return false;
    }
    return IOT_Protocol_SetRadioWriteRegister(addr,val);
}

int8_t IOT_LoRa_GetRadioReadRegister(uint8_t addr)
{
    if(addr > 0x70) {
        return false;
    }
    return IOT_Protocol_GetRadioReadRegister(addr);
}

int IOT_LoRa_RadioSendData(const uint8_t *buffer, uint8_t len, uint32_t timeout)
{
    bool sendState = false;
    uint32_t _timeout = timeout;
    if(_timeout != 0) {
        if(_timeout < LORA_RADIO_SEND_TIMEOUT) {
            _timeout = LORA_RADIO_SEND_TIMEOUT;
        }
    }

    sendState = IOT_Protocol_SendRadioData(buffer, len, _timeout);
    log_v("sendState=%d\r\n",sendState);
    if(!sendState) {//发送忙
        loraSendStatus = LORA_SEND_FAIL;
        return LORA_SEND_FAIL;
    } else {
        if(_timeout == 0) { //不阻塞 发送结果由事件方式返回
            loraSendStatus = LORA_SENDING;
            return LORA_SENDING; //发送中
        } else {
            uint32_t prevTime = millis();
            loraSendResult = 0;
            while(1) {
                intoyunLoop();
                if(loraSendResult == ep_lora_radio_tx_done) {
                    loraSendStatus = LORA_SEND_SUCCESS;
                    return LORA_SEND_SUCCESS;
                } else if(loraSendResult == ep_lora_radio_tx_fail) {
                    loraSendStatus = LORA_SEND_FAIL;
                    return LORA_SEND_FAIL;
                }

                if(millis() - prevTime > _timeout) {
                    loraSendStatus = LORA_SEND_FAIL;
                    return LORA_SEND_FAIL;
                }
            }
        }
    }
}

