/*
*  产品名称：
*  产品描述：
*  说    明： 该代码为平台自动根据产品内容生成的代码模板框架，
*             您可以在此框架下开发。此框架包括数据点的定义和读写。
*  模板版本： v1.4
*/

#include "project_config.h"
#include "intoyun_interface.h"
#include "user_interface.h"

//运行流程
static enum eDeviceState
{
    DEVICE_STATE_IDLE = 0,
    DEVICE_STATE_WAIT,
    DEVICE_STATE_JOIN,
    DEVICE_STATE_SEND,
    DEVICE_STATE_SLEEP
} deviceState;

//定义数据点ID
//格式：DPID_数据类型_数据点英文名. 如果英文名相同的，则在后面添加_1 _2 _3形式。
//说明：布尔型: BOOL. 数值型: INT32或者DOUBLE. 枚举型: ENUM. 字符串型: STRING. 透传型: BINARY
#define DPID_BOOL_SWITCH                      1  //布尔型     开关(可上送可下发)
#define DPID_DOUBLE_TEMPATURE                 2  //浮点型     温度(可上送可下发)
#define DPID_INT32_HUMIDITY                   3  //整型       湿度(可上送可下发)
#define DPID_ENUM_COLOR                       4  //枚举型     颜色模式(可上送可下发)
#define DPID_STRING_DISPLAY                   5  //字符串型   显示字符串(可上送可下发)
#define DPID_BINARY_LOCATION                  6  //透传型     位置(可上送可下发)

//定义数据点变量
//格式：数据类型+数据点英文名. 如果英文名相同的，则在后面添加_1 _2 _3形式。
//布尔型为bool
//枚举型为int
//整型为int32_t
//浮点型为double
//字符型为String
//透传型为uint8_t*型
//透传型长度为uint16_t型
bool dpBoolSwitch;              //开关
double dpDoubleTemperature;     //温度
int32_t dpInt32Humidity;        //湿度
int dpEnumColor;                //颜色模式
char dpStringDisplay[20];       //显示字符串
uint8_t dpBinaryLocation[8] = {1,2,3,4,5,6,7,8};      //位置
uint16_t dpBinaryLocationLen;   //位置长度

uint32_t timerID;

void system_event_callback(system_event_t event, int param, uint8_t *data, uint16_t datalen)
{
    if((event == event_cloud_data) && (param == ep_cloud_data_datapoint))
    {
        /*************此处修改和添加用户控制代码*************/
        //开关
        if(RESULT_DATAPOINT_NEW == Cloud.readDatapointBool(DPID_BOOL_SWITCH, &dpBoolSwitch))
        {
            //用户代码
        }

        //温度
        if(RESULT_DATAPOINT_NEW == Cloud.readDatapointNumberDouble(DPID_DOUBLE_TEMPATURE, &dpDoubleTemperature))
        {
            //用户代码
        }

        //湿度
        if(RESULT_DATAPOINT_NEW == Cloud.readDatapointNumberInt32(DPID_INT32_HUMIDITY, &dpInt32Humidity))
        {
            //用户代码
        }

        // 颜色模式
        if (RESULT_DATAPOINT_NEW == Cloud.readDatapointEnum(DPID_ENUM_COLOR, &dpEnumColor))
        {
            //用户代码
        }

        // 显示字符串
        if (RESULT_DATAPOINT_NEW == Cloud.readDatapointString(DPID_STRING_DISPLAY, dpStringDisplay))
        {
            //用户代码
        }

        // 位置信息
        if (RESULT_DATAPOINT_NEW == Cloud.readDatapointBinary(DPID_BINARY_LOCATION, dpBinaryLocation, &dpBinaryLocationLen))
        {
            //用户代码
        }
        /*******************************************************/
    }
    else if(event == event_lorawan_status)
    {
        switch(param)
        {
        case ep_lorawan_join_success: //激活成功
            Timer.stop(LED_TIMER_NUM);
            LedControl(true); //LED长亮
            deviceState = DEVICE_STATE_IDLE;
            break;

        case ep_lorawan_join_fail: //入网失败
            deviceState = DEVICE_STATE_JOIN;
            break;

        case ep_lorawan_send_success: //发送成功
            deviceState = DEVICE_STATE_IDLE;
            break;

        case ep_lorawan_send_fail: //发送失败
            deviceState = DEVICE_STATE_IDLE;
            break;

        case ep_lorawan_module_wakeup: //模组已唤醒
            deviceState = DEVICE_STATE_IDLE;
            break;

        default:
            break;
        }
    }
}

void userInit(void)
{
    //定义数据点事件
    System.setEventCallback(system_event_callback);
    //发送产品信息
    System.setDeviceInfo(PRODUCT_ID_DEF,HARDWARE_VERSION_DEF,SOFTWARE_VERSION_DEF);
    //根据网关参数具体设置
    LoRaWan.setDataRate(DR_3);
    LoRaWan.setChannelStatus(0, false);       //关闭通道0 频率固定:433175000
    LoRaWan.setChannelStatus(1, false);       //关闭通道1 频率固定:433375000
    LoRaWan.setChannelStatus(2, true);        //打开通道2 频率固定:433375000
    LoRaWan.setChannelFreq(2, 433575000);     //设置通道2频率
    LoRaWan.setChannelDRRange(2, DR_3, DR_3); //设置通道2速率范围
    //定义产品数据点
    Cloud.defineDatapointBool(DPID_BOOL_SWITCH, DP_PERMISSION_UP_DOWN, false);                     //开关
    Cloud.defineDatapointNumber(DPID_DOUBLE_TEMPATURE, DP_PERMISSION_UP_ONLY, 0, 100, 2, 12.34);   //温度
    Cloud.defineDatapointNumber(DPID_INT32_HUMIDITY, DP_PERMISSION_UP_ONLY, 0, 100, 0, 0);         //湿度
    Cloud.defineDatapointEnum(DPID_ENUM_COLOR, DP_PERMISSION_UP_DOWN, 1);                          //颜色模式
    Cloud.defineDatapointString(DPID_STRING_DISPLAY, DP_PERMISSION_UP_DOWN, "hello! intoyun!");    //显示字符串
    Cloud.defineDatapointBinary(DPID_BINARY_LOCATION, DP_PERMISSION_UP_DOWN, dpBinaryLocation, 8); //位置信息
    //如果选择的是Class A则去掉
    LoRaWan.setMacClassType(CLASS_C); //设置为C类
    /*************此处修改和添加用户初始化代码**************/
    timerID = timerGetId();
    deviceState = DEVICE_STATE_JOIN;
    userInterfaceInit();
    /*******************************************************/
}

void userHandle(void)
{
    /*************此处修改和添加用户处理代码****************/
    switch(deviceState)
    {
        case DEVICE_STATE_JOIN:
            deviceState = DEVICE_STATE_WAIT;
            Cloud.connect(JOIN_OTAA, 0);
            Timer.start(LED_TIMER_NUM); //激活中 led闪烁
            break;

        case DEVICE_STATE_SEND:
            deviceState = DEVICE_STATE_WAIT;
            //更新数据点数据（数据点具备：上送属性）
            Cloud.writeDatapointBool(DPID_BOOL_SWITCH, dpBoolSwitch);
            Cloud.writeDatapointNumberDouble(DPID_DOUBLE_TEMPATURE, dpDoubleTemperature);
            Cloud.writeDatapointNumberInt32(DPID_INT32_HUMIDITY, dpInt32Humidity);
            Cloud.writeDatapointEnum(DPID_ENUM_COLOR, dpEnumColor);
            Cloud.writeDatapointString(DPID_STRING_DISPLAY, dpStringDisplay);
            Cloud.writeDatapointBinary(DPID_BINARY_LOCATION, dpBinaryLocation, 8);
            //发送数据点数据，建议不频繁上送数据
            Cloud.sendDatapointAll(IS_SEND_CONFIRMED_DEF, 0);
            break;

        case DEVICE_STATE_SLEEP:
            //通讯模式为CLASS_A才允许休眠 如果选择C类则去掉
            System.sleepModule(60);
            break;
            /************************/

        case DEVICE_STATE_WAIT:   //在处理发送
            break;

        case DEVICE_STATE_IDLE:   //空闲状态
            if(timerIsEnd(timerID, 600000))  //处理间隔  用户可自行更改
            {
                timerID = timerGetId();
                deviceState = DEVICE_STATE_SEND;
            }
            break;

        default:
            break;
    }
    /*******************************************************/
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
