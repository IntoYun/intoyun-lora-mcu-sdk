#include "intoyun_interface.h"
#include "stm32f1xx_hal.h"

#define PRODUCT_ID                       "y4NFFyDE9uq6H202"//产品ID
#define PRODUCT_SECRET                   "ab697b0dc1716d24cfc49b071668e766"//产品秘钥
#define HARDWARE_VERSION                 "V1.0.0"          //硬件版本号
#define SOFTWARE_VERSION                 "V1.0.0"          //软件版本号

#define LED_PIN             GPIO_PIN_0
#define LED_GPIO_PORT       GPIOB
#define LED_ON	 	          HAL_GPIO_WritePin(LED_GPIO_PORT,LED_PIN, GPIO_PIN_RESET)
#define LED_OFF		          HAL_GPIO_WritePin(LED_GPIO_PORT,LED_PIN, GPIO_PIN_SET)
#define LED_TOG		          HAL_GPIO_TogglePin(LED_GPIO_PORT,LED_PIN)

#define WAKEUP_PIN          GPIO_PIN_9
#define WAKEUP_GPIO_PORT    GPIOB

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


void GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    __GPIOB_CLK_ENABLE();
    GPIO_InitStruct.Pin = LED_PIN | WAKEUP_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
    HAL_GPIO_Init(LED_GPIO_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(LED_GPIO_PORT, LED_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(WAKEUP_GPIO_PORT, WAKEUP_PIN, GPIO_PIN_SET);
}

void WakeupPinWrite(uint8_t val)
{
    if(val == 0){
        HAL_GPIO_WritePin(WAKEUP_GPIO_PORT, WAKEUP_PIN, GPIO_PIN_RESET);
    }else{
        HAL_GPIO_WritePin(WAKEUP_GPIO_PORT, WAKEUP_PIN, GPIO_PIN_SET);
    }
}

void WakeupModule(void)
{
    WakeupPinWrite(0);//唤醒模组
    delay(1);
    WakeupPinWrite(1);
}


void LoRaWanEventProcess(uint8_t eventType,lorawan_event_type_t event, int rssi, uint8_t *data, uint32_t len)
{
    if(eventType == event_lorawan_status){
        switch(event)
        {
        case ep_lorawan_join_success://入网成功 已连接平台
            deviceState = DEVICE_STATE_SEND;
            log_v("event lorawan join success\r\n");
            break;

        case ep_lorawan_join_fail: //入网失败
            deviceState = DEVICE_STATE_IDLE;
            log_v("event lorawan join fail\r\n");
            break;

        case ep_lorawan_send_success:
            /* sleepEnable = true; */
            deviceState = DEVICE_STATE_SLEEP;
            log_v("event lorawan send success\r\n");
            break;

        case ep_lorawan_send_fail:
            /* sleepEnable = true; */
            deviceState = DEVICE_STATE_SLEEP;
            log_v("event lorawan send fail\r\n");
            break;
        case ep_lorawan_module_wakeup:
            log_v("event lorawan system wakeup\r\n");
            break;

        case ep_lorawan_datapoint: //处理平台数据
            log_v("event lorawan receive platform datapoint\r\n");
            //灯泡控制
            if (RESULT_DATAPOINT_NEW == Cloud.readDatapointBool(DPID_BOOL_SWITCH, &dpBoolLightSwitch)){
                log_v("datapoint switch = %d\r\n",dpBoolLightSwitch);
                if(dpBoolLightSwitch){
                    /* 开灯 */
                    LED_ON;
                    Cloud.writeDatapointBool(DPID_BOOL_LIGHT_STATUS, true);
                    Cloud.writeDatapointBool(DPID_BOOL_SWITCH, true);
                }else{
                    //关灯
                    LED_OFF;
                    Cloud.writeDatapointBool(DPID_BOOL_LIGHT_STATUS, false);
                    Cloud.writeDatapointBool(DPID_BOOL_SWITCH, false);
                }
            }
            //速度控制
            if (RESULT_DATAPOINT_NEW == Cloud.readDatapointNumberInt32(DPID_NUMBER_SPEED, &dpNumberSpeed)){
                log_v("datapoint speed = %d\r\n",dpNumberSpeed);
            }

            if(RESULT_DATAPOINT_NEW == Cloud.readDatapointNumberDouble(DPID_NUMBER_TEMPERATURE,&dpNumberTemperature)){
                log_v("datapoint tempature = %f\r\n",dpNumberTemperature);
            }
            //颜色模式
            if (RESULT_DATAPOINT_NEW == Cloud.readDatapointEnum(DPID_ENUM_LIGHT_MODE, &dpEnumLightMode)){
                log_v("datapoint corlor mode = %d\r\n",dpEnumLightMode);
            }
            //字符串显示
            if (RESULT_DATAPOINT_NEW == Cloud.readDatapointString(DPID_STRING_LCD_DISPLAY, dpStringLcdDisplay)){
                log_v("datapoint string = %s\r\n",dpStringLcdDisplay);
            }
            //二进制数据
            if(RESULT_DATAPOINT_NEW == Cloud.readDatapointBinary(DPID_BINARY,dpBinaryVal,9)){
                log_v("datapoint binary = ");
                for(uint8_t i = 0; i < 9; i++){
                    log_v("0x%x ",dpBinaryVal[i]);
                }
                log_v("\r\n");
            }
            break;

        case ep_lorawan_custom_data: //接受到透传数据
            log_v("event lorawan receive custom data\r\n");
            break;

        default:
            break;
        }
    }
}

void userInit(void)
{
    GPIO_Init();
    //添加数据点定义
    Cloud.defineDatapointEnum(DPID_ENUM_LIGHT_MODE, DP_PERMISSION_UP_DOWN, 2, DP_POLICY_NONE, 0);                         //颜色模式
    Cloud.defineDatapointNumber(DPID_NUMBER_TEMPERATURE, DP_PERMISSION_UP_ONLY, -100, 100, 2, 22.34, DP_POLICY_NONE, 0);  //温度
    Cloud.defineDatapointBool(DPID_BOOL_SWITCH, DP_PERMISSION_UP_DOWN, false, DP_POLICY_NONE, 0);                         //灯泡开关
    Cloud.defineDatapointBool(DPID_BOOL_LIGHT_STATUS, DP_PERMISSION_UP_ONLY, false, DP_POLICY_NONE, 0);                   //灯泡开关
    Cloud.defineDatapointNumber(DPID_NUMBER_SPEED, DP_PERMISSION_UP_DOWN, 0, 1000, 0, 55, DP_POLICY_NONE, 0);             //速度
    Cloud.defineDatapointString(DPID_STRING_LCD_DISPLAY, DP_PERMISSION_UP_DOWN, "hello world!!!", DP_POLICY_NONE, 0);     //字符显示
    Cloud.defineDatapointBinary(DPID_BINARY, DP_PERMISSION_UP_DOWN, dpBinaryVal,9, DP_POLICY_NONE, 0);               //二进制数据

    delay(500);
    log_v("lorawan slave mode\r\n");
    delay(100);
    System.setDeviceInfo(PRODUCT_ID,HARDWARE_VERSION,SOFTWARE_VERSION);
    System.setEventCallback(LoRaWanEventProcess);
    delay(10);

    LoRaWan.setDatarate(3);
    LoRaWan.setChannelDRRange(2,3,3);
    LoRaWan.setRX2Params(3,434665000);
    LoRaWan.setChannelStatus(0,0);
    LoRaWan.setChannelStatus(1,0);
    //OTAA入网
    Cloud.connect(3,0);
}

void userHandle(void)
{
    switch(deviceState)
    {
    case DEVICE_STATE_JOIN:
        log_v("lorawan joining\r\n");
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
