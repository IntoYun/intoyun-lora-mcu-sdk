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
#include "intoyun_interface.h"
#include "hal_interface.h"

const system_t System =
{
    intoyunInit,
    intoyunLoop,

    intoyunSetEventCallback,
    intoyunQueryInfo,

    intoyunSetupDevice,
    intoyunSetupProtocol,

    intoyunExecuteRestart,
    intoyunExecuteRestore,
    intoyunSetupSystemSleep,
    intoyunExecuteDFU,
    intoyunPutPipe,
};


const cloud_t Cloud =
{
    intoyunExecuteMacJoin,
    intoyunQueryConnected,
    intoyunExecuteDisconnect,
    intoyunQueryDisconnected,

    intoyunDefineDatapointBool,
    intoyunDefineDatapointNumber,
    intoyunDefineDatapointEnum,
    intoyunDefineDatapointString,
    intoyunDefineDatapointBinary,

    intoyunReadDatapointBool,
    intoyunReadDatapointNumberInt32,
    intoyunReadDatapointNumberDouble,
    intoyunReadDatapointEnum,
    intoyunReadDatapointString,
    intoyunReadDatapointBinary,

    intoyunWriteDatapointBool,
    intoyunWriteDatapointNumberInt32,
    intoyunWriteDatapointNumberDouble,
    intoyunWriteDatapointEnum,
    intoyunWriteDatapointString,
    intoyunWriteDatapointBinary,

    intoyunSendDatapointBool,
    intoyunSendDatapointNumberInt32,
    intoyunSendDatapointNumberDouble,
    intoyunSendDatapointEnum,
    intoyunSendDatapointString,
    intoyunSendDatapointBinary,
    intoyunSendAllDatapointManual,
};


const lorawan_t LoRaWan =
{
    //LoRaWan通讯接口
    intoyunSendConfirmed,
    intoyunSendUnconfirmed,
    intoyunQueryMacSendStatus,
    intoyunMacReceive,
    intoyunSetupMacClassType,
    intoyunQueryMacClassType,
    //LoRaWan参数设置接口
    intoyunQueryMacDeviceAddr,
    intoyunQueryMacDeviceEui,
    intoyunQueryMacAppEui,
    intoyunSetupMacOTAAParams,
    intoyunSetupMacABPParams,
    intoyunSetupMacPowerIndex,
    intoyunSetupMacDatarate,
    intoyunQueryMacDatarate,
    intoyunSetupMacAdr,
    intoyunQueryMacAdr,
    intoyunSetupMacChannelFreq,
    intoyunQueryMacChannelFreq,
    intoyunSetupMacChannelDRRange,
    intoyunQueryMacChannelDRRange,
    intoyunSetupMacChannelEnable,
    intoyunQueryMacChannelEnable,
    intoyunSetupMacConfirmedTrials,
    intoyunQueryMacConfirmedTrials,
    intoyunSetupMacUnconfirmedTrials,
    intoyunQueryMacUncomfirmedTrials,
    intoyunSetupMacJoinTrials,
    intoyunQueryMacJoinTrials,
    intoyunSetupMacUplinkCount,
    intoyunQueryMacUplinkCount,
    intoyunSetupMacDownlinkCount,
    intoyunQueryMacDownlinkCount,
    intoyunSetupMacRX2Params,
    intoyunQueryMacRX2Params,
    intoyunSetupMacRX1Delay,
    intoyunQueryMacRX1Delay,
    intoyunQueryMacMargin,
    intoyunQueryMacGatewayNumber,
    intoyunQueryMacSnr,
    intoyunQueryMacRssi,
};

const lora_t LoRa =
{
    //P2P通讯接口
    intoyunRadioSendData,
    intoyunQueryRadioSendStatus,
    intoyunSetupRadioRx,
    intoyunRadioRx,
    //P2P参数设置接口
    intoyunExecuteRadioStartCad,
    intoyunQueryRadioRssi,
    intoyunQueryRadioSnr,
    intoyunExecuteRadioSleep,
    intoyunSetupRadioFreq,
    intoyunQueryRadioFreq,
    intoyunSetupRadioMaxPayloadLen,
    intoyunQueryRadioMaxPayloadLen,
    intoyunSetupRadioMode,
    intoyunQueryRadioMode,
    intoyunSetupRadioSf,
    intoyunQueryRadioSf,
    intoyunSetupRadioBw,
    intoyunQueryRadioBw,
    intoyunSetupRadioCoderate,
    intoyunQueryRadioCoderate,
    intoyunSetupRadioPreambleLen,
    intoyunQueryRadioPreambleLen,
    intoyunSetupRadioFixLenOn,
    intoyunQueryRadioFixLenOn,
    intoyunSetupRadioCrcEnabled,
    intoyunQueryRadioCrcEnabled,
    intoyunSetupRadioFreqHopOn,
    intoyunQueryRadioFreqHopOn,
    intoyunSetupRadioHopPeriod,
    intoyunQueryRadioHopPeriod,
    intoyunSetupRadioIqInverted,
    intoyunQueryRadioIqInverted,
    intoyunSetupRadioRxContinuous,
    intoyunQueryRadioRxContinuous,
    intoyunSetupRadioTxPower,
    intoyunQueryRadioTxPower,
    intoyunSetupRadioFixPayloadLen,
    intoyunQueryRadioFixPayloadLen,
    intoyunSetupRadioSymbTimeout,
    intoyunQueryRadioSymbTimeout,
    intoyunQueryRadioReadRegister,
    intoyunSetupRadioWriteRegister,
};

void delay(uint32_t ms)
{
    uint32_t start_millis = HAL_Millis();
    uint32_t current_millis = 0;
    uint32_t elapsed_millis = 0;

    if (ms == 0) return;
    while (1) {
        current_millis = HAL_Millis();
        if (current_millis < start_millis){
            elapsed_millis =  UINT_MAX - start_millis + current_millis;
        } else {
            elapsed_millis = current_millis - start_millis;
        }

        if (elapsed_millis > ms) {
            break;
        }
        intoyunLoop();
    }
}

uint32_t millis(void)
{
    return HAL_Millis();
}
