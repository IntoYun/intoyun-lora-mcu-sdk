/*
*  产品名称：
*  产品描述：
*  说    明： 该代码为平台自动根据产品内容生成的代码模板框架，
*             您可以在此框架下开发。此框架包括数据点的定义和读写。
*  模板版本： v1.4
*/

#include "project_config.h"
#include "intoyun_interface.h"

static enum eDeviceState
{
    DEVICE_STATE_JOIN,
    DEVICE_STATE_SEND,
    DEVICE_STATE_IDLE,
    DEVICE_STATE_SLEEP,
}deviceState;

#define DPID_ENUM_LIGHT_MODE             1    //颜色模式
#define DPID_NUMBER_TEMPERATURE          2    //温度
#define DPID_BOOL_SWITCH                 3    //灯泡开关
#define DPID_BOOL_LIGHT_STATUS           4    //灯泡状态
#define DPID_NUMBER_SPEED                5    //速度
#define DPID_STRING_LCD_DISPLAY          6    //字符显示
#define DPID_BINARY                      7    //二进制

//定义接收云平台数据变量
bool dpBoolLightSwitch = false;       //开关命令
int32_t dpNumberSpeed = 100;        //速度
int dpEnumLightMode = 1;          //颜色模式
char dpStringLcdDisplay[50] = "hello world!!!";  //字符显示
double dpNumberTemperature = 12.34;   //温度
uint8_t dpBinaryVal[9] = {0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99}; //二进制数据
uint16_t dpBinaryLen;

void system_event_callback( system_event_t event, int param, uint8_t *data, uint16_t datalen)
{
    switch(event)
    {
    case event_lorawan_status:
        switch(param)
        {
        case ep_lorawan_join_success://入网成功 已连接平台
            deviceState = DEVICE_STATE_SEND;
            break;

        case ep_lorawan_join_fail: //入网失败
            deviceState = DEVICE_STATE_IDLE;
            break;

        case ep_lorawan_send_success:
            deviceState = DEVICE_STATE_SLEEP;
            break;

        case ep_lorawan_send_fail:
            deviceState = DEVICE_STATE_SLEEP;
            break;
        case ep_lorawan_module_wakeup:
            break;
        default:
            break;
        }
        break;

    case event_cloud_data:
        switch(param){
        case ep_cloud_data_datapoint: //处理平台数据
            //灯泡控制
            if (RESULT_DATAPOINT_NEW == Cloud.readDatapointBool(DPID_BOOL_SWITCH, &dpBoolLightSwitch)){
            }
            //速度控制
            if (RESULT_DATAPOINT_NEW == Cloud.readDatapointNumberInt32(DPID_NUMBER_SPEED, &dpNumberSpeed)){
            }

            if(RESULT_DATAPOINT_NEW == Cloud.readDatapointNumberDouble(DPID_NUMBER_TEMPERATURE,&dpNumberTemperature)){
            }
            //颜色模式
            if (RESULT_DATAPOINT_NEW == Cloud.readDatapointEnum(DPID_ENUM_LIGHT_MODE, &dpEnumLightMode)){
            }
            //字符串显示
            if (RESULT_DATAPOINT_NEW == Cloud.readDatapointString(DPID_STRING_LCD_DISPLAY, dpStringLcdDisplay)){
            }
            //二进制数据
            if(RESULT_DATAPOINT_NEW == Cloud.readDatapointBinary(DPID_BINARY,dpBinaryVal,&dpBinaryLen)){
            }
            break;

        case ep_cloud_data_custom: //接受到透传数据
            break;

        default:
            break;
        }
        break;
    default:
        break;
    }
}

void userInit(void)
{
    //添加数据点定义
    Cloud.defineDatapointEnum(DPID_ENUM_LIGHT_MODE, DP_PERMISSION_UP_DOWN, 2);                         //颜色模式
    Cloud.defineDatapointNumber(DPID_NUMBER_TEMPERATURE, DP_PERMISSION_UP_ONLY, -100, 100, 2, 22.34);  //温度
    Cloud.defineDatapointBool(DPID_BOOL_SWITCH, DP_PERMISSION_UP_DOWN, false);                         //灯泡开关
    Cloud.defineDatapointBool(DPID_BOOL_LIGHT_STATUS, DP_PERMISSION_UP_ONLY, false);                   //灯泡开关
    Cloud.defineDatapointNumber(DPID_NUMBER_SPEED, DP_PERMISSION_UP_DOWN, 0, 1000, 0, 55);             //速度
    Cloud.defineDatapointString(DPID_STRING_LCD_DISPLAY, DP_PERMISSION_UP_DOWN, "hello world!!!");     //字符显示
    Cloud.defineDatapointBinary(DPID_BINARY, DP_PERMISSION_UP_DOWN, dpBinaryVal,9);               //二进制数据
    delay(500);
    System.setDeviceInfo(PRODUCT_ID_DEF,HARDWARE_VERSION_DEF,SOFTWARE_VERSION_DEF);
    System.setEventCallback(system_event_callback);
    delay(10);

    //设置速率
    LoRaWan.setDataRate(DR_3);
    //设置通道2的速率范围
    LoRaWan.setChannelDRRange(2,DR_3,DR_3);
    //关闭通道0
    LoRaWan.setChannelStatus(0,false);
    //关闭通道1
    LoRaWan.setChannelStatus(1,false);
    Cloud.connect(JOIN_OTAA,0);
}

void userHandle(void)
{
    switch(deviceState)
    {
    case DEVICE_STATE_JOIN:
        deviceState = DEVICE_STATE_IDLE;
        break;

    case DEVICE_STATE_SEND:
        //处理需要上送到云平台的数据
        if(dpNumberTemperature > 100){
            dpNumberTemperature = 0;
        }else{
            dpNumberTemperature += 0.1;
        }
        Cloud.writeDatapointEnum(DPID_ENUM_LIGHT_MODE,dpEnumLightMode);
        Cloud.writeDatapointNumberDouble(DPID_NUMBER_TEMPERATURE, dpNumberTemperature);
        Cloud.writeDatapointNumberInt32(DPID_NUMBER_SPEED, dpNumberSpeed);
        Cloud.writeDatapointString(DPID_STRING_LCD_DISPLAY,dpStringLcdDisplay);
        Cloud.writeDatapointBinary(DPID_BINARY,dpBinaryVal,9);
        Cloud.sendDatapointAll(false,0);
        deviceState = DEVICE_STATE_IDLE;
        break;

    case DEVICE_STATE_IDLE:
        break;

    case DEVICE_STATE_SLEEP:
        delay(100);
        System.sleepModule(15);
        delay(30000);
        deviceState = DEVICE_STATE_SEND;
        break;

    default:
        break;
    }
}

int main(void)
{
    System.init();
    userInit();
    while(1)
    {
        System.loop();
        userHandle();
    }
}
