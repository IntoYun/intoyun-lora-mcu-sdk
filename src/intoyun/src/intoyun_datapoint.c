/**
******************************************************************************
Copyright (c) 2013-2014 Intoyun Team.  All right reserved.

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

#include "intoyun_datapoint.h"
#include "intoyun_interface.h"
#include "intoyun_config.h"
#include "intoyun_log.h"

property_conf *properties[PROPERTIES_MAX];

int properties_count = 0;
event_handler_t loraEventHandler = NULL;
uint8_t loraSendResult = 0;
int lorawanJoinStatus = 0; //0 连接平台 1 连接中 -1 未连接
int8_t loraSendStatus = 0; //数据发送状态 0 成功 1 发送中 -1 失败

//初始化设置参数 手动发送 发送间隔时间  发送运行时间
volatile datapoint_control_t g_datapoint_control = {DP_TRANSMIT_MODE_MANUAL, 0, 0};

void intoyunInit(void)
{
    HAL_SystemInit();
    ProtocolParserInit();
}

void intoyunSetEventCallback(event_handler_t loraHandler)
{
    if(loraHandler!= NULL)
    {
        loraEventHandler = loraHandler;
    }
}

void intoyunLoop(void)
{
    ProtocolModuleActiveSendHandle();
}

void intoyunPutPipe(uint8_t value)
{
    ProtocolPutPipe(value);//将接收到的数据放入缓冲区
}

void intoyunSetDataAutoSend(uint32_t time)
{
    g_datapoint_control.datapoint_transmit_mode = DP_TRANSMIT_MODE_AUTOMATIC;
    if(time < DATAPOINT_TRANSMIT_AUTOMATIC_INTERVAL)
    {
        g_datapoint_control.datapoint_transmit_lapse = DATAPOINT_TRANSMIT_AUTOMATIC_INTERVAL;
        return;
    }
    g_datapoint_control.datapoint_transmit_lapse = time;
}

static int intoyunDiscoverProperty(const uint16_t dpID)
{
    for (int index = 0; index < properties_count; index++)
    {
        if (properties[index]->dpID == dpID)
        {
            return index;
        }
    }
    return -1;
}

static bool intoyunPropertyChanged(void)
{
    for (int i = 0; i < properties_count; i++)
    {
        if (properties[i]->change)
        {
            return true;
        }
    }
    return false;
}

static uint8_t intoyunGetPropertyCount(void)
{
    return properties_count;
}

static void intoyunPropertyChangeClear(void)
{
    for (int i = 0; i < properties_count; i++)
    {
        if (properties[i]->change)
        {
            properties[i]->change = false;
        }
    }
}


dp_transmit_mode_t intoyunGetDatapointTransmitMode(void)
{
    return g_datapoint_control.datapoint_transmit_mode;
}

void intoyunDatapointControl(dp_transmit_mode_t mode, uint32_t lapse)
{
    g_datapoint_control.datapoint_transmit_mode = mode;
    /* if(DP_TRANSMIT_MODE_AUTOMATIC == g_datapoint_control.datapoint_transmit_mode) */
    {
        if(lapse < DATAPOINT_TRANSMIT_AUTOMATIC_INTERVAL)
        {
            g_datapoint_control.datapoint_transmit_lapse = DATAPOINT_TRANSMIT_AUTOMATIC_INTERVAL;
        }
        else
        {
            g_datapoint_control.datapoint_transmit_lapse = lapse;
        }
    }
}

static double _pow(double base, int exponent)
{
    double result = 1.0;
    int i = 0;

    for (i = 0; i < exponent; i++) {
        result *= base;
    }
    return result;
}

static void intoyunDatapointValueInit(uint16_t count,uint16_t dpID,data_type_t dataType,dp_permission_t permission, dp_policy_t policy,int lapse)
{
    properties[count]=(property_conf*)malloc(sizeof(property_conf));

    properties[count]->dpID       = dpID;
    properties[count]->dataType   = dataType;
    properties[count]->permission = permission;
    properties[count]->policy     = policy;
    properties[count]->lapse      = lapse;
    properties[count]->runtime    = 0;
    properties[count]->readFlag   = RESULT_DATAPOINT_OLD;
}

void intoyunDefineDatapointBool(const uint16_t dpID, dp_permission_t permission, const bool value, dp_policy_t policy, const int lapse)
{
    int lapseTemp = lapse;

    if (-1 == intoyunDiscoverProperty(dpID))
    {
        if(DP_POLICY_NONE == policy)
        {
            lapseTemp = 0;
        }
        intoyunDatapointValueInit(properties_count,dpID,DATA_TYPE_BOOL,permission,policy,lapseTemp*1000);
        properties[properties_count]->boolValue=value;
        properties_count++; // count the number of properties
    }
}

void intoyunDefineDatapointNumber(const uint16_t dpID, dp_permission_t permission, const double minValue, const double maxValue, const int resolution, const double value, dp_policy_t policy, const int lapse)
{
    int lapseTemp = lapse;

    if (-1 == intoyunDiscoverProperty(dpID))
    {
        if(DP_POLICY_NONE == policy)
        {
            lapseTemp = 0;
        }

        intoyunDatapointValueInit(properties_count,dpID,DATA_TYPE_NUM,permission,policy,lapseTemp*1000);

        double defaultValue = value;
        if(defaultValue < minValue)
        {
            defaultValue = minValue;
        }
        else if(defaultValue > maxValue)
        {
            defaultValue = maxValue;
        }

        if(resolution == 0)
        {
            properties[properties_count]->numberIntValue=value;
        }
        else
        {
            properties[properties_count]->numberDoubleValue=value;
        }
        properties[properties_count]->numberProperty.minValue = minValue;
        properties[properties_count]->numberProperty.maxValue = maxValue;
        properties[properties_count]->numberProperty.resolution = resolution;
        properties_count++; // count the number of properties
    }
}

void intoyunDefineDatapointEnum(const uint16_t dpID, dp_permission_t permission, const int value, dp_policy_t policy, const int lapse)
{
    int lapseTemp;

    if (-1 == intoyunDiscoverProperty(dpID))
    {
        if(DP_POLICY_NONE == policy)
        {
            lapseTemp = 0;
        }

        int defaultValue = value;
        if(defaultValue < 0)
        {
            defaultValue = 0;
        }

        intoyunDatapointValueInit(properties_count,dpID,DATA_TYPE_ENUM,permission,policy,lapseTemp*1000);
        properties[properties_count]->enumValue = defaultValue;
        properties_count++; // count the number of properties
    }
}

void intoyunDefineDatapointString(const uint16_t dpID, dp_permission_t permission, const char *value, dp_policy_t policy, const int lapse)
{
    int lapseTemp;

    if (-1 == intoyunDiscoverProperty(dpID))
    {
        if(DP_POLICY_NONE == policy)
        {
            lapseTemp = 0;
        }

        intoyunDatapointValueInit(properties_count,dpID,DATA_TYPE_STRING,permission,policy,lapseTemp*1000);
        properties[properties_count]->stringValue = (char *)malloc(strlen(value)+1);
        strncpy(properties[properties_count]->stringValue,value,strlen(value)+1);
        properties_count++; // count the number of properties
    }
}

void intoyunDefineDatapointBinary(const uint16_t dpID, dp_permission_t permission, const uint8_t *value, const uint16_t len, dp_policy_t policy, const int lapse)
{
    int lapseTemp;

    if (-1 == intoyunDiscoverProperty(dpID))
    {
        if(DP_POLICY_NONE == policy)
        {
            lapseTemp = 0;
        }

        intoyunDatapointValueInit(properties_count,dpID,DATA_TYPE_BINARY,permission,policy,lapseTemp*1000);
        properties[properties_count]->binaryValue.value = (uint8_t *)malloc(len);
        for(uint8_t i=0;i<len;i++)
        {
            properties[properties_count]->binaryValue.value[i] = value[i];
        }
        properties[properties_count]->binaryValue.len = (uint16_t)len;
        properties_count++; // count the number of properties
    }
}

read_datapoint_result_t intoyunReadDatapointBool(const uint16_t dpID, bool *value)
{
    int index = intoyunDiscoverProperty(dpID);
    if (index == -1)
    {
        return RESULT_DATAPOINT_NONE;
    }

    (*value) = properties[index]->boolValue;
    read_datapoint_result_t readResult = properties[index]->readFlag;
    properties[index]->readFlag = RESULT_DATAPOINT_OLD;
    return readResult;
}

read_datapoint_result_t intoyunReadDatapointNumberInt32(const uint16_t dpID, int32_t *value)
{
    int index = intoyunDiscoverProperty(dpID);
    if (index == -1)
    {
        return RESULT_DATAPOINT_NONE;
    }

    (*value) = properties[index]->numberIntValue;
    read_datapoint_result_t readResult = properties[index]->readFlag;
    properties[index]->readFlag = RESULT_DATAPOINT_OLD;
    return readResult;
}

read_datapoint_result_t intoyunReadDatapointNumberDouble(const uint16_t dpID, double *value)
{
    int index = intoyunDiscoverProperty(dpID);
    if (index == -1)
    {
        return RESULT_DATAPOINT_NONE;
    }

    (*value) = properties[index]->numberDoubleValue;
    read_datapoint_result_t readResult = properties[index]->readFlag;
    properties[index]->readFlag = RESULT_DATAPOINT_OLD;
    return readResult;
}

read_datapoint_result_t intoyunReadDatapointEnum(const uint16_t dpID, int *value)
{
    int index = intoyunDiscoverProperty(dpID);
    if (index == -1)
    {
        return RESULT_DATAPOINT_NONE;
    }

    (*value) = properties[index]->enumValue;
    read_datapoint_result_t readResult = properties[index]->readFlag;
    properties[index]->readFlag = RESULT_DATAPOINT_OLD;
    return readResult;
}


read_datapoint_result_t intoyunReadDatapointString(const uint16_t dpID, char *value)
{
    int index = intoyunDiscoverProperty(dpID);
    if (index == -1)
    {
        return RESULT_DATAPOINT_NONE;
    }

    for(uint16_t i=0;i<strlen(properties[index]->stringValue);i++)
    {
        value[i] = properties[index]->stringValue[i];
    }
    read_datapoint_result_t readResult = properties[index]->readFlag;
    properties[index]->readFlag = RESULT_DATAPOINT_OLD;
    return readResult;
}

read_datapoint_result_t intoyunReadDatapointBinary(const uint16_t dpID, uint8_t *value, uint16_t len)
{
    int index = intoyunDiscoverProperty(dpID);
    if (index == -1)
    {
        return RESULT_DATAPOINT_NONE;
    }

    for(uint16_t i=0;i<len;i++)
    {
        value[i] = properties[index]->binaryValue.value[i];
    }
    len = properties[index]->binaryValue.len;
    read_datapoint_result_t readResult = properties[index]->readFlag;
    properties[index]->readFlag = RESULT_DATAPOINT_OLD;

    return readResult;
}

// dpCtrlType   0: 平台控制写数据   1：用户写数据
void intoyunPlatformWriteDatapointBool(const uint16_t dpID, bool value, bool dpCtrlType)
{
    int index = intoyunDiscoverProperty(dpID);
    if(index == -1)
    {
        return;
    }

    if(properties[index]->dataType != DATA_TYPE_BOOL)
    {
        return;
    }
    else
    {
        if(properties[index]->boolValue != value)
        {
            properties[index]->change = true;
            if(dpCtrlType)
            { //用户操作
                properties[index]->readFlag = RESULT_DATAPOINT_OLD;
            }
            else
            {
                properties[index]->readFlag = RESULT_DATAPOINT_NEW;
            }
            properties[index]->boolValue = value;
        }
        else
        {
            properties[index]->change = false;
            if(dpCtrlType)
            { //用户操作
                properties[index]->readFlag = RESULT_DATAPOINT_OLD;
            }
            else
            {
                properties[index]->readFlag = RESULT_DATAPOINT_NEW;
            }
        }

    }
}

void intoyunWriteDatapointBool(const uint16_t dpID, bool value)
{
    intoyunPlatformWriteDatapointBool(dpID,value,true);
}

void intoyunPlatformWriteDatapointNumberInt32(const uint16_t dpID, int32_t value, bool dpCtrlType)
{
    int index = intoyunDiscoverProperty(dpID);
    if(index == -1)
    {
        return;
    }

    if(properties[index]->dataType != DATA_TYPE_NUM || properties[index]->numberProperty.resolution != 0)
    {
        return;
    }
    else
    {
        int32_t tmp = value;
        if(tmp < properties[index]->numberProperty.minValue)
        {
            tmp = properties[index]->numberProperty.minValue;
        }
        else if(tmp > properties[index]->numberProperty.maxValue)
        {
            tmp = properties[index]->numberProperty.maxValue;
        }

        if(properties[index]->numberIntValue != value)
        {
            properties[index]->change = true;
            if(dpCtrlType)
            { //用户操作
                properties[index]->readFlag = RESULT_DATAPOINT_OLD;
            }
            else
            {
                properties[index]->readFlag = RESULT_DATAPOINT_NEW;
            }
            properties[index]->numberIntValue = tmp;
        }
        else
        {
            properties[index]->change = false;
            if(dpCtrlType)
            { //用户操作
                properties[index]->readFlag = RESULT_DATAPOINT_OLD;
            }
            else
            {
                properties[index]->readFlag = RESULT_DATAPOINT_NEW;
            }
        }
    }
}

void intoyunWriteDatapointNumberInt32(const uint16_t dpID, int32_t value)
{
    intoyunPlatformWriteDatapointNumberInt32(dpID,value,true);
}

void intoyunPlatformWriteDatapointNumberDouble(const uint16_t dpID, double value, bool dpCtrlType)
{
    int index = intoyunDiscoverProperty(dpID);
    if(index == -1)
    {
        return;
    }

    if(properties[index]->dataType != DATA_TYPE_NUM || properties[index]->numberProperty.resolution == 0)
    {
        return;
    }
    else
    {
        int32_t tmp;
        double d = value;
        if(d < properties[index]->numberProperty.minValue)
        {
            d = properties[index]->numberProperty.minValue;
        }
        else if(d > properties[index]->numberProperty.maxValue)
        {
            d = properties[index]->numberProperty.maxValue;
        }

        //保证小数点位数
        tmp = (int32_t)(d * _pow(10, properties[index]->numberProperty.resolution));

        if(properties[index]->numberDoubleValue != d)
        {
            properties[index]->change = true;
            if(dpCtrlType)
            { //用户操作
                properties[index]->readFlag = RESULT_DATAPOINT_OLD;
            }
            else
            {
                properties[index]->readFlag = RESULT_DATAPOINT_NEW;
            }

            properties[index]->numberDoubleValue = tmp/(double)_pow(10, properties[index]->numberProperty.resolution);
        }
        else
        {
            properties[index]->change = false;
            if(dpCtrlType)
            { //用户操作
                properties[index]->readFlag = RESULT_DATAPOINT_OLD;
            }
            else
            {
                properties[index]->readFlag = RESULT_DATAPOINT_NEW;
            }
        }
    }
}

void intoyunWriteDatapointNumberDouble(const uint16_t dpID, double value)
{
    intoyunPlatformWriteDatapointNumberDouble(dpID,value,true);
}

void intoyunPlatformWriteDatapointEnum(const uint16_t dpID, int value, bool dpCtrlType)
{
    int index = intoyunDiscoverProperty(dpID);
    if(index == -1)
    {
        return;
    }

    if(properties[index]->dataType != DATA_TYPE_ENUM)
    {
        return;
    }
    else
    {
        if(properties[index]->enumValue != value)
        {
            properties[index]->change = true;
            if(dpCtrlType)
            { //用户操作
                properties[index]->readFlag = RESULT_DATAPOINT_OLD;
            }
            else
            {
                properties[index]->readFlag = RESULT_DATAPOINT_NEW;
            }
            properties[index]->enumValue = value;
        }
        else
        {
            properties[index]->change = false;
            if(dpCtrlType)
            { //用户操作
                properties[index]->readFlag = RESULT_DATAPOINT_OLD;
            }
            else
            {
                properties[index]->readFlag = RESULT_DATAPOINT_NEW;
            }
        }
    }
}

void intoyunWriteDatapointEnum(const uint16_t dpID, int value)
{
    intoyunPlatformWriteDatapointEnum(dpID,value,true);
}

void intoyunPlatformWriteDatapointString(const uint16_t dpID, const char *value, bool dpCtrlType)
{
    int index = intoyunDiscoverProperty(dpID);
    if(index == -1)
    {
        return;
    }

    if(properties[index]->dataType != DATA_TYPE_STRING)
    {
        return;
    }
    else
    {
        if(strcmp(properties[index]->stringValue,value) != 0)
        {
            properties[index]->change = true;
            if(dpCtrlType)
            { //用户操作
                properties[index]->readFlag = RESULT_DATAPOINT_OLD;
            }
            else
            {
                properties[index]->readFlag = RESULT_DATAPOINT_NEW;
            }
            properties[index]->stringValue = (char *)malloc(strlen(value)+1);
            strncpy(properties[index]->stringValue,value,strlen(value)+1);
        }
        else
        {
            properties[index]->change = false;
            if(dpCtrlType)
            { //用户操作
                properties[index]->readFlag = RESULT_DATAPOINT_OLD;
            }
            else
            {
                properties[index]->readFlag = RESULT_DATAPOINT_NEW;
            }
        }
    }
}

void intoyunWriteDatapointString(const uint16_t dpID, const char *value)
{
    intoyunPlatformWriteDatapointString(dpID,value,true);
}

void intoyunPlatformWriteDatapointBinary(const uint16_t dpID, const uint8_t *value, uint16_t len, bool dpCtrlType)
{
    int index = intoyunDiscoverProperty(dpID);
    if(index == -1)
    {
        return;
    }

    if(properties[index]->dataType != DATA_TYPE_BINARY)
    {
        return;
    }
    else
    {
        if(memcmp(properties[index]->binaryValue.value,value,len) != 0)
        {
            properties[index]->change = true;
            if(dpCtrlType)
            { //用户操作
                properties[index]->readFlag = RESULT_DATAPOINT_OLD;
            }
            else
            {
                properties[index]->readFlag = RESULT_DATAPOINT_NEW;
            }

            properties[index]->binaryValue.value = (uint8_t *)malloc(len);
            for(uint16_t i=0;i<len;i++)
            {
                properties[index]->binaryValue.value[i] = value[i];
            }
            properties[index]->binaryValue.len = (uint16_t)len;

        }
        else
        {
            properties[index]->change = false;
            if(dpCtrlType)
            { //用户操作
                properties[index]->readFlag = RESULT_DATAPOINT_OLD;
            }
            else
            {
                properties[index]->readFlag = RESULT_DATAPOINT_NEW;
            }
        }
    }
}

void intoyunWriteDatapointBinary(const uint16_t dpID, const uint8_t *value, uint16_t len)
{
    intoyunPlatformWriteDatapointBinary(dpID,value,len,true);
}

int intoyunSendDatapointBool(const uint16_t dpID, bool value, bool confirmed, uint16_t timeout)
{
    int index = intoyunDiscoverProperty(dpID);

    if (index == -1)
    {
        return -1;
    }

    intoyunPlatformWriteDatapointBool(dpID, value, true);

    if(DP_TRANSMIT_MODE_AUTOMATIC == intoyunGetDatapointTransmitMode()) {
        return -1;
    }

    //只允许下发
    if ( properties[index]->permission == DP_PERMISSION_DOWN_ONLY)
    {
        return -1;
    }

    //数值未发生变化
    if (!properties[index]->change && properties[index]->policy == DP_POLICY_NONE)
    {
        return -1;
    }

    return intoyunSendSingleDatapoint(index,confirmed,timeout);
}

int intoyunSendDatapointNumberInt32(const uint16_t dpID, int32_t value, bool confirmed, uint16_t timeout)
{
    int index = intoyunDiscoverProperty(dpID);

    if (index == -1)
    {
        return -1;
    }

    intoyunPlatformWriteDatapointNumberInt32(dpID, value, true);

    if(DP_TRANSMIT_MODE_AUTOMATIC == intoyunGetDatapointTransmitMode()) {
        return -1;
    }

    //只允许下发
    if ( properties[index]->permission == DP_PERMISSION_DOWN_ONLY)
    {
        return -1;
    }

    //数值未发生变化
    if (!properties[index]->change && properties[index]->policy == DP_POLICY_NONE)
    {
        return -1;
    }

    return intoyunSendSingleDatapoint(index,confirmed,timeout);
}

int intoyunSendDatapointNumberDouble(const uint16_t dpID, double value, bool confirmed, uint16_t timeout)
{
    int index = intoyunDiscoverProperty(dpID);

    if (index == -1)
    {
        // not found, nothing to do
        return -1;
    }

    intoyunPlatformWriteDatapointNumberDouble(dpID, value, true);

    if(DP_TRANSMIT_MODE_AUTOMATIC == intoyunGetDatapointTransmitMode()) {
        return -1;
    }

    //只允许下发
    if ( properties[index]->permission == DP_PERMISSION_DOWN_ONLY)
    {
        return -1;
    }

    //数值未发生变化
    if (!properties[index]->change && properties[index]->policy == DP_POLICY_NONE)
    {
        return -1;
    }

    return intoyunSendSingleDatapoint(index,confirmed,timeout);
}

int intoyunSendDatapointEnum(const uint16_t dpID, int value, bool confirmed, uint16_t timeout)
{
    int index = intoyunDiscoverProperty(dpID);

    if (index == -1)
    {
        // not found, nothing to do
        return -1;
    }

    intoyunPlatformWriteDatapointEnum(dpID, value, true);

    if(DP_TRANSMIT_MODE_AUTOMATIC == intoyunGetDatapointTransmitMode()) {
        return -1;
    }


    //只允许下发
    if ( properties[index]->permission == DP_PERMISSION_DOWN_ONLY)
    {
        return -1;
    }

    //数值未发生变化
    if (!properties[index]->change && properties[index]->policy == DP_POLICY_NONE)
    {
        return -1;
    }

    return intoyunSendSingleDatapoint(index,confirmed,timeout);
}

int intoyunSendDatapointString(const uint16_t dpID, const char *value, bool confirmed, uint16_t timeout)
{
    int index = intoyunDiscoverProperty(dpID);

    if (index == -1)
    {
        // not found, nothing to do
        return -1;
    }

    intoyunPlatformWriteDatapointString(dpID, value, true);

    if(DP_TRANSMIT_MODE_AUTOMATIC == intoyunGetDatapointTransmitMode()) {
        return -1;
    }


    //只允许下发
    if (properties[index]->permission == DP_PERMISSION_DOWN_ONLY)
    {
        return -1;
    }

    //数值未发生变化
    if (!properties[index]->change && properties[index]->policy == DP_POLICY_NONE)
    {
        return -1;
    }

    return intoyunSendSingleDatapoint(index,confirmed,timeout);
}

int intoyunSendDatapointBinary(const uint16_t dpID, const uint8_t *value, uint16_t len, bool confirmed, uint16_t timeout)
{
    int index = intoyunDiscoverProperty(dpID);

    if (index == -1)
    {
        // not found, nothing to do
        return -1;
    }

    intoyunPlatformWriteDatapointBinary(dpID, value, len, true);

    if(DP_TRANSMIT_MODE_AUTOMATIC == intoyunGetDatapointTransmitMode()) {
        return -1;
    }


    //只允许下发
    if ( properties[index]->permission == DP_PERMISSION_DOWN_ONLY)
    {
        return -1;
    }

    //数值未发生变化
    if (!properties[index]->change && properties[index]->policy == DP_POLICY_NONE)
    {
        return -1;
    }

    return intoyunSendSingleDatapoint(index,confirmed,timeout);
}

void intoyunParseReceiveDatapoints(const uint8_t *payload, uint32_t len, uint8_t *customData)
{
    log_v("lorawan receive platform data length = %d\r\n",len);
    log_v("lorawan receive platform data:\r\n");
    log_v_dump((uint8_t *)payload,len);

    //dpid(1-2 bytes)+data type(1 byte)+data len(1-2 bytes)+data(n bytes)
    //大端表示，如果最高位是1，则表示两个字节，否则是一个字节
    int32_t index = 0;
    uint16_t dpID = 0;
    uint8_t dataType;
    uint16_t dataLength=0;

    if(payload[0] == CUSTOMER_DEFINE_DATA) //用户透传数据
    {
        (*customData) = CUSTOMER_DEFINE_DATA;
        return;
    }
    else
    {
        (*customData) = INTOYUN_DATAPOINT_DATA;
    }

    index++;

    while(index < len)
    {
        dpID = payload[index++] & 0xff;
        if(dpID >= 0x80) //数据点有2个字节
        {
            dpID = dpID & 0x7f; //去掉最高位
            dpID = (dpID << 8) | payload[index++];
        }

        dataType = payload[index++];

        switch(dataType)
        {
            case DATA_TYPE_BOOL:
            {
                dataLength = payload[index++] & 0xff;
                bool value = payload[index++];
                if(intoyunDiscoverProperty(dpID) != -1)//数据点存在
                {
                    intoyunPlatformWriteDatapointBool(dpID,value,false);
                }
            }
            break;

            case DATA_TYPE_NUM:
            {
                dataLength = payload[index++] & 0xff;
                int32_t value = payload[index++] & 0xff;
                if(dataLength == 2)
                {
                    value = (value << 8) | payload[index++];
                }
                else if(dataLength == 3)
                {
                    value = (value << 8) | payload[index++];
                    value = (value << 8) | payload[index++];
                }
                else if(dataLength == 4)
                {
                    value = (value << 8) | payload[index++];
                    value = (value << 8) | payload[index++];
                    value = (value << 8) | payload[index++];
                }

                uint8_t id = intoyunDiscoverProperty(dpID);
                if(properties[id]->numberProperty.resolution == 0) //此数据点为int
                {
                    value = value + properties[id]->numberProperty.minValue;

                    if(intoyunDiscoverProperty(dpID) != -1)//数据点存在
                    {
                        intoyunPlatformWriteDatapointNumberInt32(dpID,value,false);
                    }
                }
                else
                {
                    double dValue = (value/(double)_pow(10, properties[id]->numberProperty.resolution)) + properties[id]->numberProperty.minValue;
                    if(intoyunDiscoverProperty(dpID) != -1)//数据点存在
                    {
                        intoyunPlatformWriteDatapointNumberDouble(dpID,dValue,false);
                    }
                }

            }

            break;

            case DATA_TYPE_ENUM:
            {
                dataLength = payload[index++] & 0xff;
                int value = payload[index++] & 0xff;
                if(dataLength == 2)
                {
                    value = (value << 8) | payload[index++];
                }
                else if(dataLength == 3)
                {
                    value = (value << 8) | payload[index++];
                    value = (value << 8) | payload[index++];
                }
                else if(dataLength == 4)
                {
                    value = (value << 8) | payload[index++];
                    value = (value << 8) | payload[index++];
                    value = (value << 8) | payload[index++];
                }

                if(intoyunDiscoverProperty(dpID) != -1)//数据点存在
                {
                    intoyunPlatformWriteDatapointEnum(dpID,value,false);
                }
            }
            break;

            case DATA_TYPE_STRING:
            {
                dataLength = payload[index++] & 0xff;
                if(dataLength >= 0x80) //数据长度有2个字节
                {
                    dataLength = dataLength & 0x7f;
                    dataLength = (dataLength) << 8 | payload[index++];
                }
                char *str = (char *)malloc(dataLength+1);
                if(NULL != str)
                {
                    memset(str, 0, dataLength+1);
                    memcpy(str, &payload[index], dataLength);
                }

                index += dataLength;
                if(intoyunDiscoverProperty(dpID) != -1)//数据点存在
                {
                    intoyunPlatformWriteDatapointString(dpID,str,false);
                }
                free(str);
            }
            break;

            case DATA_TYPE_BINARY:
            {
                dataLength = payload[index++] & 0xff;
                if(dataLength >= 0x80) //数据长度有2个字节
                {
                    dataLength = dataLength & 0x7f;
                    dataLength = (dataLength) << 8 | payload[index++];
                }

                if(intoyunDiscoverProperty(dpID) != -1)//数据点存在
                {
                    intoyunPlatformWriteDatapointBinary(dpID, &payload[index], dataLength,false);
                }

                index += dataLength;
            }
            break;

            default:
                break;
        }
    }
}

//组织数据点数据
static uint16_t intoyunFormDataPointBinary(int property_index, uint8_t* buffer)
{
    int32_t index = 0;

    if(properties[property_index]->dpID < 0x80)
    {
        buffer[index++] = properties[property_index]->dpID & 0xFF;
    }
    else
    {
        buffer[index++] = (properties[property_index]->dpID >> 8) | 0x80;
        buffer[index++] = properties[property_index]->dpID & 0xFF;
    }

    switch(properties[property_index]->dataType)
    {
        case DATA_TYPE_BOOL:       //bool型
            buffer[index++] = 0x00;  //类型
            buffer[index++] = 0x01;  //长度
            buffer[index++] = (bool)(properties[property_index]->boolValue);
            break;

        case DATA_TYPE_NUM:        //数值型 int型
            {
                buffer[index++] = 0x01;
                int32_t value;
                if(properties[property_index]->numberProperty.resolution == 0)
                {
                    value = (int32_t)properties[property_index]->numberIntValue;
                }
                else
                {
                    value = (properties[property_index]->numberDoubleValue - properties[property_index]->numberProperty.minValue) \
                        * _pow(10, properties[property_index]->numberProperty.resolution);
                }

                if(value & 0xFFFF0000) {
                    buffer[index++] = 0x04;
                    buffer[index++] = (value >> 24) & 0xFF;
                    buffer[index++] = (value >> 16) & 0xFF;
                    buffer[index++] = (value >> 8) & 0xFF;
                    buffer[index++] = value & 0xFF;
                } else if(value & 0xFFFFFF00) {
                    buffer[index++] = 0x02;
                    buffer[index++] = (value >> 8) & 0xFF;
                    buffer[index++] = value & 0xFF;
                } else {
                    buffer[index++] = 0x01;
                    buffer[index++] = value & 0xFF;
                }
            }
            break;

        case DATA_TYPE_ENUM:       //枚举型
            buffer[index++] = 0x02;
            buffer[index++] = 0x01;
            buffer[index++] = (uint8_t)properties[property_index]->enumValue & 0xFF;
            break;

        case DATA_TYPE_STRING:     //字符串型
            {
                uint16_t strLength = strlen(properties[property_index]->stringValue);

                buffer[index++] = 0x03;
                if(strLength < 0x80)
                {
                    buffer[index++] = strLength & 0xFF;
                }
                else
                {
                    buffer[index++] = (strLength >> 8) | 0x80;
                    buffer[index++] = strLength & 0xFF;
                }
                memcpy(&buffer[index], properties[property_index]->stringValue, strLength);
                index+=strLength;
                break;
            }

        case DATA_TYPE_BINARY:     //二进制型
            {
                uint16_t len = properties[property_index]->binaryValue.len;
                buffer[index++] = DATA_TYPE_BINARY;
                if(len < 0x80) {
                    buffer[index++] = len & 0xFF;
                } else {
                    buffer[index++] = (len >> 8) | 0x80;
                    buffer[index++] = len & 0xFF;
                }
                memcpy(&buffer[index], properties[property_index]->binaryValue.value, len);
                index+=len;
                break;
            }

        default:
            break;
    }
    return index;
}

//组织单个数据点的数据
static uint16_t intoyunFormSingleDatapoint(int property_index, uint8_t *buffer, uint16_t len)
{
    int32_t index = 0;

    buffer[index++] = INTOYUN_DATAPOINT_DATA;
    index += intoyunFormDataPointBinary(property_index, buffer+index);
    return index;
}

// dpForm   false: 组织改变的数据点   true：组织全部的数据点
//组织所有数据点的数据
static uint16_t intoyunFormAllDatapoint(uint8_t *buffer, uint16_t len, bool dpForm)
{
    int32_t index = 0;

    buffer[index++] = INTOYUN_DATAPOINT_DATA;
    for (int i = 0; i < properties_count; i++)
    {
        //只允许下发  不上传
        if (properties[i]->permission == DP_PERMISSION_DOWN_ONLY)
        {
            continue;
        }

        //系统默认dpID  不上传
        if (properties[i]->dpID > 0xFF00)
        {
            continue;
        }

        if( dpForm || ((!dpForm) && properties[i]->change) )
        {
            index += intoyunFormDataPointBinary(i, (uint8_t *)buffer+index);
        }
    }
    return index;
}

//发送单个数据点的数据
int intoyunSendSingleDatapoint(const uint16_t dpID, bool confirmed, uint16_t timeout)
{
    uint8_t frameType = 0;
    //发送时间间隔到
    uint32_t current_millis = millis();
    int32_t elapsed_millis = current_millis - properties[dpID]->runtime;
    if (elapsed_millis < 0)
    {
        elapsed_millis =  0xFFFFFFFF - properties[dpID]->runtime + current_millis;
    }

    if (elapsed_millis >= properties[dpID]->lapse)
    {
        uint8_t buffer[256];
        uint16_t len;

        len = intoyunFormSingleDatapoint(dpID, buffer, sizeof(buffer));
        properties[dpID]->runtime = current_millis;
        if(confirmed){
            frameType = 0;
        }else{
            frameType = 1;
        }
        return intoyunTransmitData(frameType,2,buffer,len,timeout);
    }
    else
    {
        return -1;
    }
}

//发送所有数据点的数据
int intoyunSendDatapointAll(bool dpForm, bool confirmed, uint32_t timeout)
{
    uint8_t buffer[512];
    uint16_t len;
    uint8_t frameType = 0;

    len = intoyunFormAllDatapoint(buffer, sizeof(buffer), dpForm);

    log_v("send data length = %d\r\n",len);
    log_v("send data:");
    log_v_dump(buffer,len);

    if(confirmed){
        frameType = 0;
    }else{
        frameType = 1;
    }
    return intoyunTransmitData(frameType,2,buffer,len,timeout);
}

int intoyunSendCustomData(uint8_t type,uint8_t port, uint32_t timeout, const uint8_t *buffer, uint16_t len)
{
    uint8_t buf[256];
    uint16_t index = len+1;
    if(index > 256)
    {
        index = 256;
    }

    buf[0] = CUSTOMER_DEFINE_DATA;
    memcpy(&buf[1],buffer,index-1);
    return intoyunTransmitData(type,port,buf,index,timeout);
}

//发送数据
int intoyunTransmitData(uint8_t frameType,uint8_t port, const uint8_t *buffer, uint16_t len,uint16_t timeout)
{
    bool sendState = false;
    uint32_t _timeout = timeout;
    if(_timeout != 0){
        if(_timeout < LORAWAN_SEND_TIMEOUT){
            _timeout = LORAWAN_SEND_TIMEOUT;
        }
    }
    sendState = ProtocolSendPlatformData(frameType,port,buffer,len,_timeout);
    log_v("sendState =%d\r\n",sendState);
    log_v("_timeout = %d\r\n",_timeout);
    if(!sendState){//发送忙或者没有入网
        loraSendStatus = LORA_SEND_FAIL;
        return LORA_SEND_FAIL;
    }else{
        if(_timeout == 0){ //不阻塞 发送结果由事件方式返回
            loraSendStatus = LORA_SENDING;
            return LORA_SENDING; //发送中
        }else{
            uint32_t prevTime = millis();
            loraSendResult = 0;
            while(1){
                intoyunLoop();
                if(loraSendResult == 3){
                    loraSendStatus = LORA_SEND_SUCCESS;
                    return LORA_SEND_SUCCESS;
                }else if(loraSendResult == 4){
                    loraSendStatus = LORA_SEND_FAIL;
                    return LORA_SEND_FAIL;
                }
                if(millis() - prevTime > _timeout*1000){
                    loraSendStatus = LORA_SEND_FAIL;
                    return LORA_SEND_FAIL;
                }
            }
        }
    }
}

int intoyunSendAllDatapointManual(bool confirmed, uint32_t timeout)
{
    if(0 == intoyunGetPropertyCount()) {
        return -1;
    }

    if(DP_TRANSMIT_MODE_AUTOMATIC == intoyunGetDatapointTransmitMode()) {
        return -1;
    }
    return intoyunSendDatapointAll(true,confirmed,timeout);
    #if 0
    //发送时间间隔到
    uint32_t current_millis = millis();
    int32_t elapsed_millis = current_millis - g_datapoint_control.runtime;
    if (elapsed_millis < 0)
    {
        elapsed_millis =  0xFFFFFFFF - g_datapoint_control.runtime + current_millis;
    }

    //发送时间时间到
    if ( elapsed_millis >= g_datapoint_control.datapoint_transmit_lapse*1000 )
    {
        log_v("start send datapoint\r\n");

        g_datapoint_control.runtime = millis();
        intoyunPropertyChangeClear();

        return intoyunSendDatapointAll(true,type,timeout);
    }else{
        return -1;
    }
    #endif
}

void intoyunSendDatapointAutomatic(void)
{
    bool sendFlag = false;

    if(0 == intoyunGetPropertyCount()) {
        return;
    }

    if(DP_TRANSMIT_MODE_MANUAL == intoyunGetDatapointTransmitMode()) {
        return;
    }

    //当数值发生变化
    if(intoyunPropertyChanged())
    {
        sendFlag = true;
        intoyunSendDatapointAll(false,1,120);
    }
    else
    {
        //发送时间间隔到
        uint32_t current_millis = millis();
        int32_t elapsed_millis = current_millis - g_datapoint_control.runtime;
        if (elapsed_millis < 0)
        {
            elapsed_millis =  0xFFFFFFFF - g_datapoint_control.runtime + current_millis;
        }

        //发送时间时间到
        if ( elapsed_millis >= g_datapoint_control.datapoint_transmit_lapse*1000 )
        {
            sendFlag = true;
            intoyunSendDatapointAll(true,1,120);
        }
    }

    if(sendFlag)
    {
        g_datapoint_control.runtime = millis();
        intoyunPropertyChangeClear();
    }
}

int intoyunSendConfirmed(uint8_t port, uint8_t *buffer, uint16_t len, uint16_t timeout)
{
    return intoyunTransmitData(0,port,buffer,len,timeout);
}

int intoyunSendUnconfirmed(uint8_t port, uint8_t *buffer, uint16_t len, uint16_t timeout)
{
    return intoyunTransmitData(1,port,buffer,len,timeout);
}

int8_t intoyunQueryMacSendStatus(void)
{
    return loraSendStatus;
}

uint16_t intoyunMacReceive(uint8_t *buffer, uint16_t length, int *rssi)
{
    if(loraBuffer.available){
        loraBuffer.available = false;
        *rssi = loraBuffer.rssi;
        memcpy(buffer, loraBuffer.buffer, loraBuffer.bufferSize);
        return loraBuffer.bufferSize;
    }else{
        return 0;
    }
}

bool intoyunExecuteRestart(void)
{
    return ProtocolExecuteRestart();
}

bool intoyunExecuteRestore(void)
{
    return ProtocolExecuteRestore();
}

bool intoyunSetupSystemSleep(uint32_t timeout)
{
    return ProtocolSetupSystemSleep(timeout);
}

bool intoyunExecuteDFU(void)
{
    return ProtocolExecuteDFU();
}

void intoyunQueryInfo(char *moduleVersion, char *moduleType, char *deviceId, uint8_t *at_mode)
{
    device_info_t info;
    ProtocolQueryInfo(&info);

    log_v("moduleVer = %s\r\n",info.module_version);
    log_v("moduleType = %s\r\n",info.module_type);
    log_v("deviceId = %s\r\n",info.device_id);
    log_v("atmode = %d\r\n",info.at_mode);

    strncpy(moduleVersion,info.module_version,sizeof(info.module_version));
    strncpy(moduleType,info.module_type,sizeof(info.module_type));
    strncpy(deviceId,info.device_id,sizeof(info.device_id));
    *at_mode = info.at_mode;
}

void intoyunSetupDevice(char *productId, char *hardVer, char *softVer)
{
    ProtocolSetupDevice(productId,hardVer,softVer);
}

bool intoyunSetupProtocol(uint8_t mode)
{
    if(mode > 2){
        return false;
    }
    return ProtocolSetupProtocolMode(mode);
}

int8_t intoyunQueryMacClassType(void)
{
    return ProtocolQueryMacClassType();
}

bool intoyunSetupMacClassType(uint8_t type)
{
    return ProtocolSetupMacClassType(type);
}

int intoyunExecuteMacJoin(uint8_t type, uint32_t timeout)
{
    if(type > 3){
        return false;
    }
    int joinState;
    uint32_t _timeout = timeout;
    if(_timeout != 0){
        if(_timeout < LORAWAN_JOIN_TIMEOUT){
            _timeout = LORAWAN_JOIN_TIMEOUT;
        }
    }

    joinState = ProtocolExecuteMacJoin(type, _timeout);
    log_v("joinState = %d\r\n",joinState);
    if(joinState != 4){//断开连接
        lorawanJoinStatus = LORAWAN_JOIN_FAIL;
        return lorawanJoinStatus;
    }else {//入网中
        if(_timeout == 0){//退出 入网状态由事件返回
            lorawanJoinStatus = LORAWAN_JOINING;
            return lorawanJoinStatus;
        }else{
            uint32_t prevTime = millis();
            loraSendResult = 0;
            while(1){
                intoyunLoop();
                if(loraSendResult == 1){
                    lorawanJoinStatus = LORAWAN_JOIN_SUCCESS;
                    return lorawanJoinStatus;
                }else if(loraSendResult == 2){
                    lorawanJoinStatus = LORAWAN_JOIN_FAIL;
                    return lorawanJoinStatus;
                }
                if(millis() - prevTime > _timeout*1000){
                    lorawanJoinStatus = LORAWAN_JOIN_FAIL;
                    return lorawanJoinStatus;
                }
            }
        }
    }
}

int intoyunQueryConnected(void)
{
    return lorawanJoinStatus;
}

void intoyunExecuteDisconnect(void)
{
    ProtocolExecuteMacJoin(1, 0);
}

bool intoyunQueryDisconnected(void)
{
    if(lorawanJoinStatus == -1){ //已断开连接
        return true;
    }else{
        return false;
    }
}

bool intoyunQueryMacDeviceAddr(char *devAddr)
{
    return ProtocolQueryMacDeviceAddr(devAddr);
}

bool intoyunQueryMacDeviceEui(char *devEui)
{
    return ProtocolQueryMacDeviceEui(devEui);
}

bool intoyunQueryMacAppEui(char *appEui)
{
    return ProtocolQueryMacAppEui(appEui);
}

bool intoyunSetupMacOTAAParams(char *devEui, char *appEui, char *appKey)
{
    return ProtocolSetupMacOTAAParams(devEui,appEui,appKey);
}

bool intoyunSetupMacABPParams(char *devAddr, char *nwkSkey, char *appSkey)
{
    return ProtocolSetupMacABPParams(devAddr,nwkSkey,appSkey);
}

bool intoyunSetupMacPowerIndex(uint8_t index)
{
    return ProtocolSetupMacPowerIndex(index);
}

int8_t intoyunQueryMacDatarate(void)
{
    return ProtocolQueryMacDatarate();
}

bool intoyunSetupMacDatarate(uint8_t datarate)
{
    if(datarate > 5){
        return false;
    }
    return ProtocolSetupMacDatarate(datarate);
}

bool intoyunSetupMacAdr(bool enable)
{
    if(enable){
        return ProtocolSetupMacAdr(1);
    }else{
        return ProtocolSetupMacAdr(0);
    }
}

bool intoyunQueryMacAdr(void)
{
    if(ProtocolQueryMacAdr() == 1){
        return true;
    }else{
        return false;
    }
}

bool intoyunSetupMacDutyCyclePrescaler(uint16_t dutyCycle)
{
    return ProtocolSetupMacDutyCyclePrescaler(dutyCycle);
}

uint16_t intoyunQueryMacDutyCyclePrescaler(void)
{
    return ProtocolQueryMacDutyCyclePrescaler();
}

uint32_t intoyunQueryMacChannelFreq(uint8_t channelId)
{
    if(channelId > 15){
        return false;
    }
    return ProtocolQueryMacChannelFreq(channelId);
}

bool intoyunSetupMacChannelFreq(uint8_t channelId, uint32_t freq)
{
    if(channelId > 15 || freq > 525000000 || freq < 432000000){
        return false;
    }
    return ProtocolSetupMacChannelFreq(channelId,freq);
}

bool intoyunQueryMacChannelDRRange(uint8_t channelId, uint8_t *minDR, uint8_t *maxDR)
{
    if(channelId > 15 ){
        return false;
    }
    channel_params *drRange;
    if(ProtocolQueryMacChannelDRRange(channelId,drRange)){
        *minDR = drRange->minDR;
        *maxDR = drRange->maxDR;
        return true;
    }else{
        return false;
    }
}

bool intoyunSetupMacChannelDRRange(uint8_t channelId, uint8_t minDR, uint8_t maxDR)
{
    if(channelId > 15 || maxDR > 5){
        return false;
    }
    return ProtocolSetupMacChannelDRRange(channelId,minDR,maxDR);
}

bool intoyunQueryMacChannelEnable(uint8_t channelId)
{
    if(channelId > 15){
        return false;
    }

    if(ProtocolQueryMacChannelEnable(channelId) == 1){
        return true;
    }else{
        return false;
    }
}

bool intoyunSetupMacChannelEnable(uint8_t channelId, bool enable)
{
    uint8_t temp;
    if(channelId > 15){
        return false;
    }
    if(enable){
        temp = 1;
    }else{
        temp = 0;
    }
    return ProtocolSetupMacChannelEnable(channelId,temp);
}

uint8_t intoyunQueryMacConfirmedTrials(void)
{
    return ProtocolQueryMacConfirmedTrials();
}

bool intoyunSetupMacConfirmedTrials(uint8_t count)
{
    if(count > 8){
        return false;
    }
    return ProtocolSetupMacConfirmedTrials(count);
}

uint8_t intoyunQueryMacUncomfirmedTrials(void)
{
    return ProtocolQueryMacUncomfirmedTrials();
}

bool intoyunSetupMacUnconfirmedTrials(uint8_t count)
{
    if(count > 15){
        return false;
    }
    return ProtocolSetupMacUncomfirmedTrials(count);
}

uint8_t intoyunQueryMacJoinTrials(void)
{
    return ProtocolQueryMacJoinTrials();
}

bool intoyunSetupMacJoinTrials(uint8_t count)
{
    return ProtocolSetupMacJoinTrials(count);
}

uint8_t intoyunQueryMacMargin(void)
{
    return ProtocolQueryMacMargin();
}

uint8_t intoyunQueryMacGatewayNumber(void)
{
    return ProtocolQueryMacGatewayNumber();
}

uint8_t intoyunQueryMacSnr(void)
{
    return ProtocolQueryMacSnr();
}

int16_t intoyunQueryMacRssi(void)
{
    return ProtocolQueryMacRssi();
}

uint16_t intoyunQueryMacRX1Delay(void)
{
    return ProtocolQueryMacRX1Delay();
}

bool intoyunSetupMacRX1Delay(uint16_t delay)
{
    return ProtocolSetupMacRX1Delay(delay);
}

bool intoyunQueryMacRX2Params(uint8_t *datarate, uint32_t *freq)
{
    channel_params *rx2Params;
    if(ProtocolQueryMacRX2Params(rx2Params)){
        (*datarate) = rx2Params->datarate;
        (*freq) = rx2Params->freq;
        return true;
    }else{
        return false;
    }
}

bool intoyunSetupMacRX2Params(uint8_t datarate, uint32_t freq)
{
    if(freq > 525000000 || freq < 432000000 || datarate > 5){
        return false;
    }
    return ProtocolSetupMacRX2Params(datarate,freq);
}

bool intoyunSetupMacUplinkCount(uint32_t count)
{
    return ProtocoSetupMacUplinkCount(count);
}

int intoyunQueryMacUplinkCount(void)
{
    return ProtocolQueryMacUplinkCount();
}

bool intoyunSetupMacDownlinkCount(uint32_t count)
{
    return ProtocolSetupMacDownlinkCount(count);
}

int intoyunQueryMacDownlinkCount(void)
{
    return ProtocolQueryMacDownlinkCount();
}


int8_t intoyunQueryRadioSendStatus(void)
{
    return loraSendStatus;
}

bool intoyunSetupRadioRx(uint32_t rxTimeout)
{
    return ProtocolSetupRadioRx(rxTimeout);
}

uint16_t intoyunRadioRx(uint8_t *buffer, uint16_t length, int *rssi)
{
    if(loraBuffer.available){
        loraBuffer.available = false;
        *rssi = loraBuffer.rssi;
        memcpy(buffer, loraBuffer.buffer, loraBuffer.bufferSize);
        return loraBuffer.bufferSize;
    }else{
        return 0;
    }
}

bool intoyunSetupRadioFreq(uint32_t freq)
{
    if(freq > 525000000 || freq < 432000000)
    {
        return false;
    }
    return ProtocolSetupRadioFreq(freq);
}

bool intoyunExecuteRadioStartCad(void)
{
    return ProtocolExecuteRadioStartCad();
}

uint8_t intoyunQueryRadioSnr(void)
{
    return (uint8_t)ProtocolQueryRadioSnr();
}

int16_t intoyunQueryRadioRssi(void)
{
    return ProtocolQueryRadioRssi();
}

int intoyunQueryRadioFreq(void)
{
    return ProtocolQueryRadioFreq();
}

bool intoyunSetupRadioMaxPayloadLen(uint8_t payloadLen)
{
    if(payloadLen > 255){
        return false;
    }
    return ProtocolSetupRadioMaxPayloadLen(payloadLen);
}

int16_t intoyunQueryRadioMaxPayloadLen(void)
{
    return ProtocolQueryRadioMaxPayloadLen();
}

int8_t intoyunQueryRadioMode(void)
{
    return ProtocolQueryRadioMode();
}

bool intoyunSetupRadioMode(uint8_t mode)
{
    if(mode > 2){
        return false;
    }
    return ProtocolSetupRadioMode(mode);
}

bool intoyunSetupRadioSf(uint8_t sf)
{
    if(sf < 7 || sf >12)
    {
        return false;
    }
    return ProtocolSetupRadioSf(sf);
}

int8_t intoyunQueryRadioSf(void)
{
    return ProtocolQueryRadioSf();
}

bool intoyunSetupRadioBw(uint8_t bw)
{
    if(bw > 2)
    {
        return false;
    }
    return ProtocolSetupRadioBw(bw);
}

int8_t intoyunQueryRadioBw(void)
{
    return ProtocolQueryRadioBw();
}

bool intoyunSetupRadioCoderate(uint8_t coderate)
{
    if(coderate < 1 || coderate > 4)
    {
        return false;
    }
    return ProtocolSetupRadioCoderate(coderate);
}

int8_t intoyunQueryRadioCoderate(void)
{
    return ProtocolQueryRadioCoderate();
}

bool intoyunSetupRadioPreambleLen(uint16_t preambleLen)
{
    if(preambleLen < 6 || preambleLen > 65535)
    {
        return false;
    }
    return ProtocolSetupRadioPreambleLen(preambleLen);
}

int intoyunQueryRadioPreambleLen(void)
{
    return ProtocolQueryRadioPreambleLen();
}

bool intoyunQueryRadioFixLenOn(void)
{
    if(ProtocolQueryRadioFixLenOn() == 1){
        return true;
    }else{
        return false;
    }
}

bool intoyunSetupRadioFixLenOn(bool fixLenOn)
{
    uint8_t temp;
    if(fixLenOn){
        temp = 1;
    }else{
        temp = 0;
    }
    return ProtocolSetupRadioFixLenOn(temp);
}

bool intoyunSetupRadioCrcEnabled(bool enable)
{
    if(enable){
        return ProtocolSetupRadioCrcEnabled(1);
    }else{
        return ProtocolSetupRadioCrcEnabled(0);
    }
}

bool intoyunQueryRadioCrcEnabled(void)
{
    if(ProtocolQueryRadioCrcEnabled() == 1){
        return true;
    }else{
        return false;
    }
}

bool intoyunQueryRadioFreqHopOn(void)
{
    if(ProtocolQueryRadioFreqHopOn() == 1){
        return true;
    }else{
        return false;
    }
}

bool intoyunSetupRadioFreqHopOn(bool freqHopOn)
{
    uint8_t temp;
    if(freqHopOn){
        temp = 1;
    }else{
        temp = 0;
    }
    return ProtocolSetupRadioFreqHopOn(temp);
}

uint8_t intoyunQueryRadioHopPeriod(void)
{
    return ProtocolQueryRadioHopPeriod();
}

bool intoyunSetupRadioHopPeriod(uint8_t hopPeriod)
{
    return ProtocolSetupRadioFreqHopOn(hopPeriod);
}

bool intoyunSetupRadioIqInverted(bool iqInverted)
{
    if(iqInverted){
        return ProtocolSetupRadioIqInverted(1);
    }else{
        return ProtocolSetupRadioIqInverted(0);
    }
}

bool intoyunQueryRadioIqInverted(void)
{
    if(ProtocolQueryRadioIqInverted() == 1){
        return true;
    }else{
        return false;
    }
}

bool intoyunSetupRadioRxContinuous(bool rxContinuous)
{
    if(rxContinuous){
        return ProtocolSetupRadioRxContinuous(1);
    }else{
        return ProtocolSetupRadioRxContinuous(0);
    }
}

bool intoyunQueryRadioRxContinuous(void)
{
    if(ProtocolQueryRadioRxContinuous() == 1){
        return true;
    }else{
        return false;
    }
}

bool intoyunSetupRadioTxPower(int8_t txPower)
{
    if(txPower > 20)
    {
        return false;
    }
    return ProtocolSetupRadioTxPower(txPower);
}

int8_t intoyunQueryRadioTxPower(void)
{
    return ProtocolQueryRadioTxPower();
}

bool intoyunSetupRadioSymbTimeout(uint16_t symbTimeout)
{
    if(symbTimeout < 4 || symbTimeout > 1023)
    {
        return false;
    }
    return ProtocolSetupRadioSymbTimeout(symbTimeout);
}

int16_t intoyunQueryRadioSymbTimeout(void)
{
    return ProtocolQueryRadioSymbTimeout();
}

bool intoyunExecuteRadioSleep(void)
{
    return ProtocolExecuteRadioSleep();
}

bool intoyunSetupRadioFixPayloadLen(uint8_t payloadLen)
{
    if(payloadLen > 255){
        return false;
    }
    return ProtocolSetupRadioFixPayloadLen(payloadLen);
}

int16_t intoyunQueryRadioFixPayloadLen(void)
{
    return ProtocolQueryRadioFixPayloadLen();
}

bool intoyunSetupRadioWriteRegister(uint8_t addr, uint8_t val)
{
    if(addr > 0x70)
    {
        return false;
    }
    return ProtocolSetupRadioWriteRegister(addr,val);
}

int8_t intoyunQueryRadioReadRegister(uint8_t addr)
{
    if(addr > 0x70)
    {
        return false;
    }
    return ProtocolQueryRadioReadRegister(addr);
}

//发送数据
int intoyunRadioSendData(const uint8_t *buffer, uint8_t len, uint32_t timeout)
{
    bool sendState = false;
    uint32_t _timeout = timeout;
    if(_timeout != 0){
        if(_timeout < LORA_RADIO_SEND_TIMEOUT){
            _timeout = LORA_RADIO_SEND_TIMEOUT;
        }
    }

    sendState = ProtocolSendRadioData(buffer, len, _timeout);
    log_v("sendState=%d\r\n",sendState);
    if(!sendState){//发送忙
        loraSendStatus = LORA_SEND_FAIL;
        return LORA_SEND_FAIL;
    }else{
        if(_timeout == 0){ //不阻塞 发送结果由事件方式返回
            loraSendStatus = LORA_SENDING;
            return LORA_SENDING; //发送中
        }else{
            uint32_t prevTime = millis();
            loraSendResult = 0;
            while(1){
                intoyunLoop();
                if(loraSendResult == 1){
                    loraSendStatus = LORA_SEND_SUCCESS;
                    return LORA_SEND_SUCCESS;
                }else if(loraSendResult == 2){
                    loraSendStatus = LORA_SEND_FAIL;
                    return LORA_SEND_FAIL;
                }

                if(millis() - prevTime > _timeout){
                    loraSendStatus = LORA_SEND_FAIL;
                    return LORA_SEND_FAIL;
                }
            }
        }
    }
}
