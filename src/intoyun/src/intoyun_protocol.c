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

#include "intoyun_protocol.h"
#include "intoyun_datapoint.h"
#include "intoyun_interface.h"
#include "intoyun_config.h"
#include "intoyun_log.h"
#include "hal_interface.h"


#define MAX_SIZE        512  //!< max expected messages (used with RX)

//! check for timeout
#define TIMEOUT(t, ms)  ((ms != TIMEOUT_BLOCKING) && ((millis() - t) > ms))


pipe_t pipeRx; //定义串口数据接收缓冲区

static bool parserInitDone = false;
static bool cancelAllOperations = false;
lora_data_t loraBuffer;

static __inline int PipeInc(pipe_t *pipe, int i, int n)//默认n = 1
{
    i += n;
    if (i >= pipe->_s)
        i -= pipe->_s;
    return i;
}

static void PipeInit(pipe_t *pipe, int n, char *b)//默认b = NULL
{
    pipe->_a = b ? NULL : n ? malloc(n) : NULL;
    pipe->_r = 0;
    pipe->_w = 0;
    pipe->_b = b ? b : pipe->_a;
    pipe->_s = n;
}


static bool PipeWriteable(pipe_t *pipe)
{
    return PipeFree(pipe) > 0;
}

/** Return the number of free elements in the buffer
    \return the number of free elements
*/
static int PipeFree(pipe_t *pipe)
{
    int s = pipe->_r - pipe->_w;
    if (s <= 0)
        s += pipe->_s;
    return s - 1;
}


/* Add a single element to the buffer. (blocking)
   \param c the element to add.
   \return c
*/
static char PipePutc(pipe_t *pipe, char c)
{
    int i = pipe->_w;
    int j = i;
    i = PipeInc(pipe, i, 1);
    while (i == pipe->_r) // = !writeable()
        /* nothing / just wait */;
    pipe->_b[j] = c;
    pipe->_w = i;
    return c;
}


/** Get the number of values available in the buffer
    return the number of element available
*/
static int PipeSize(pipe_t *pipe)
{
    int s = pipe->_w - pipe->_r;
    if (s < 0)
        s += pipe->_s;
    return s;
}

/*! get elements from the buffered pipe
    \param p the elements extracted
    \param n the maximum number elements to extract
    \param t set to true if blocking, false otherwise
    \return number elements extracted
*/
static int PipeGet(pipe_t *pipe, char *p, int n, bool t )//默认t= false
{
    int c = n;
    while (c)
    {
        int f;
        for (;;) // wait for data
        {
            /* f = size(); */
            f = PipeSize(pipe);
            if (f)  break;        // free space
            if (!t) return n - c; // no space and not blocking
            /* nothing / just wait */;
        }
        // check available data
        if (c < f) f = c;
        int r = pipe->_r;
        int m = pipe->_s - r;
        // check wrap
        if (f > m) f = m;
        memcpy(p, &pipe->_b[r], f);
        pipe->_r = PipeInc(pipe, r, f);
        c -= f;
        p += f;
    }
    return n - c;
}

/** set the parsing index and return the number of available
    elments starting this position.
    \param ix the index to set.
    \return the number of elements starting at this position
*/
static int PipeSet(pipe_t *pipe, int ix)
{
    int sz = PipeSize(pipe);
    ix = (ix > sz) ? sz : ix;
    pipe->_o = PipeInc(pipe, pipe->_r, ix);
    return sz - ix;
}

/** get the next element from parsing position and increment parsing index
    \return the extracted element.
*/
static char PipeNext(pipe_t *pipe)
{
    int o = pipe->_o;
    char t = pipe->_b[o];
    pipe->_o = PipeInc(pipe, o, 1);
    return t;
}

static int SerialPipePutc(int c)
{
    uint8_t data = c;
    HAL_UartWrite(data);
    return c;
}

static int SerialPipePut(const void* buffer, int length, bool blocking)
{
    int n;
    const char* ptr = (const char*)buffer;

    for(n=0; n<length; n++)
    {
        SerialPipePutc(ptr[n]);
    }
    return length;
}

static void SerialPipeRxIrqBuf(uint8_t c)
{
    if(PipeWriteable(&pipeRx))
    {
        PipePutc(&pipeRx,c);
    }
}

//取消协议解析
static void ProtocolParserCancel(void)
{
    cancelAllOperations = true;
}

//恢复协议解析
static void ProtocolParserResume(void)
{
    cancelAllOperations = false;
}

//通过串口发送数据
static int ProtocolParserTransmit(const void* buf, int len)
{
    return SerialPipePut((const char*)buf, len, true);
}

//发送指令
static int ProtocolParserSend(const char* buf, int len)
{
    return ProtocolParserTransmit(buf, len);
}

static int ProtocolParserSendFormated(const char* format, ...)
{
    if (cancelAllOperations) return 0;

    char buf[MAX_SIZE];
    va_list args;
    va_start(args, format);
    int len = vsnprintf(buf,sizeof(buf), format, args);
    va_end(args);

    return ProtocolParserSend(buf, len);
}

//匹配查询响应的数据
static int ProtocolParserMatch(pipe_t *pipe, int len, const char* sta, const char* end)
{
    int o = 0;
    if (sta)
    {
        while (*sta)
        {
            if (++o > len) return WAIT;
            char ch = PipeNext(pipe);
            if (*sta++ != ch) return NOT_FOUND;
        }
    }
    if (!end) return o; // no termination
    // at least any char
    if (++o > len) return WAIT;
    PipeNext(pipe);
    // check the end
    int x = 0;
    while (end[x])
    {
        if (++o > len) return WAIT;
        char ch = PipeNext(pipe);
        x = (end[x] == ch) ? x + 1 :
            (end[0] == ch) ? 1 :
            0;
    }
    return o;
}


//查询对方主动发过来的数据
static int ProtocolParserFormated(pipe_t *pipe, int len, const char* fmt)
{
    int o = 0;
    int num = 0;
    if (fmt)
    {
        while (*fmt)
        {
            if (++o > len) return WAIT;
            char ch = PipeNext(pipe);
            if (*fmt == '%')
            {
                fmt++;
                if (*fmt == 'd')
                { // numeric
                    fmt ++;
                    while (ch >= '0' && ch <= '9')
                    {
                        if (++o > len) return WAIT;
                        ch = PipeNext(pipe);
                    }
                }
                else if (*fmt == 'n')
                { // data len
                    fmt ++;
                    num = 0;
                    while (ch >= '0' && ch <= '9')
                    {
                        num = num * 10 + (ch - '0');
                        if (++o > len) return WAIT;
                        ch = PipeNext(pipe);
                    }
                }
                else if (*fmt == 'c')
                { // char buffer (takes last numeric as length)
                    fmt ++;
                    while (--num)
                    {
                        if (++o > len) return WAIT;
                        ch = PipeNext(pipe);
                    }
                    continue;
                }
                else if (*fmt == 's')
                {
                    fmt ++;
                    if (ch != '\"') return NOT_FOUND;

                    do {
                        if (++o > len) return WAIT;
                        ch = PipeNext(pipe);
                    } while (ch != '\"');

                    if (++o > len) return WAIT;
                    ch = PipeNext(pipe);
                }
            }
            if (*fmt++ != ch) return NOT_FOUND;
        }
    }
    return o;
}


static int ProtocolParserGetOnePacket(pipe_t *pipe, char* buf, int len)
{
    int unkn = 0;
    int sz = PipeSize(pipe);
    int fr = PipeFree(pipe);
    if (len > sz)
        len = sz;
    while (len > 0)
    {
        static struct {
            const char* fmt;              int type;
        } lutF[] = {
            // %d:表示正常数值   %n:表示数据长度   %c:表示数据
            /* { "+RECDATA,%n:%c",           TYPE_PLUS  }, //平台下发的数据 */
            { "+RECRADIODATA,%d,%n:%c",         TYPE_PLUS  }, //不运行协议的数据
            { "+RECMACDATA,%d,%n:%c",           TYPE_PLUS  }, //平台下发的数据
        };

        static struct {
            const char* sta;       const char* end;    int type;
        } lut[] = {
            { "\r\nOK\r\n",        NULL,               TYPE_OK      },
            { "\r\nERROR\r\n",     NULL,               TYPE_ERROR   },
            { "\r\nFAIL\r\n",      NULL,               TYPE_FAIL    },
            { "+",                 "\r\n",             TYPE_PLUS    },
            { "> ",                NULL,               TYPE_PROMPT  }, //模组接收数据
            { "\r\nSEND OK\r\n",   NULL,               TYPE_OK      }, //数据发送成功
            { "\r\nBUSY\r\n",      NULL,               TYPE_BUSY    }, //发送忙
            { "\r\nSENDING\r\n",   NULL,               TYPE_SENDING }, //发送中
        };

        for (int i = 0; i < (int)(sizeof(lutF)/sizeof(*lutF)); i ++)
        {
            PipeSet(pipe,unkn);
            int ln = ProtocolParserFormated(pipe, len, lutF[i].fmt);
            if (ln == WAIT && fr)
                return WAIT;
            if ((ln != NOT_FOUND) && (unkn > 0))
                return TYPE_UNKNOWN | PipeGet(pipe, buf, unkn, false);
            if (ln > 0)
                return lutF[i].type  | PipeGet(pipe, buf, ln, false);
        }

        for (int i = 0; i < (int)(sizeof(lut)/sizeof(*lut)); i ++)
        {
            PipeSet(pipe,unkn);
            int ln = ProtocolParserMatch(pipe, len, lut[i].sta, lut[i].end);
            if (ln == WAIT && fr)
                return WAIT;
            if ((ln != NOT_FOUND) && (unkn > 0))
                return TYPE_UNKNOWN | PipeGet(pipe, buf, unkn, false);
            if (ln > 0)
                return lut[i].type | PipeGet(pipe, buf, ln, false);
        }
        // UNKNOWN
        unkn ++;
        len--;
    }
    return TYPE_UNKNOWN | PipeGet(pipe, buf, unkn, false); //应该返回TYPE_UNKNOWN 并且应该从缓存里面清掉。否则会一直接受到相同的数据 并且应该从缓存里面清掉。
}

//抓取一个包解析
static int ProtocolParserGetPacket(char* buffer, int length)
{
    return ProtocolParserGetOnePacket(&pipeRx, buffer, length);
}

//解析平台数据
static uint8_t ProtocolParserPlatformData(const uint8_t *buffer, uint16_t len)
{
    uint8_t customData = 0;
    #ifdef CONFIG_INTOYUN_DATAPOINT
    intoyunParseReceiveDatapoints(buffer,len,&customData);
    #endif
    return customData;
}

//等待响应
static int ProtocolParserWaitFinalResp(callbackPtr cb, void* param, uint32_t timeout_ms) //NULL NULL 5000
{
    if (cancelAllOperations) return WAIT;

    char buf[MAX_SIZE] = {0};
    uint32_t start = millis();
    do {
        int ret = ProtocolParserGetPacket(buf, sizeof(buf));
        if ((ret != WAIT) && (ret != NOT_FOUND))
        {
            int type = TYPE(ret);
            //handle unsolicited commands here
            if (type == TYPE_PLUS)
            {
                const char* cmd = buf+1;
                //log_v("cmd = %s\r\n",cmd);
                int event, eventParam;
                uint16_t platformDataLen;
                uint8_t *platformData;
                int rssi = 0;

                if(sscanf(cmd,"RECRADIOEVT:%d\r\n",(int*)&event) == 1)
                {
                    switch(event){
                    case 1:
                        eventParam = ep_lora_radio_tx_done;
                        loraSendResult = ep_lora_radio_tx_done;
                        break;
                    case 2:
                        eventParam = ep_lora_radio_tx_fail;
                        loraSendResult = ep_lora_radio_tx_fail;
                        break;
                    case 3:
                        eventParam = ep_lora_radio_rx_timeout;
                        break;
                    case 4:
                        eventParam = ep_lora_radio_rx_error;
                        break;
                    case 5:
                        eventParam = ep_lora_radio_module_wakeup;
                        break;
                    default:
                        break;
                    }
                    if(loraEventHandler != NULL){
                        loraEventHandler(event_lora_radio_status, eventParam, NULL, 0);
                    }
                }
                else if(sscanf(cmd,"RECMACEVT:%d\r\n",(int*)&event) == 1)
                {
                    switch(event){
                    case 1:
                        eventParam = ep_lorawan_join_success;
                        lorawanJoinStatus = LORAWAN_JOIN_SUCCESS;
                        loraSendResult = ep_lorawan_join_success;
                        break;
                    case 2:
                        eventParam = ep_lorawan_join_fail;
                        lorawanJoinStatus = LORAWAN_JOIN_FAIL;
                        loraSendResult = ep_lorawan_join_fail;
                        break;
                    case 3:
                        eventParam = ep_lorawan_send_success;
                        loraSendStatus = LORA_SEND_SUCCESS;
                        loraSendResult = ep_lorawan_send_success;
                        break;
                    case 4:
                        eventParam = ep_lorawan_send_fail;
                        loraSendStatus = LORA_SEND_FAIL;
                        loraSendResult = ep_lorawan_send_fail;
                        break;
                    case 5:
                        eventParam = ep_lorawan_module_wakeup;
                        break;
                    default:
                        break;
                    }
                    if(loraEventHandler != NULL){
                        loraEventHandler(event_lorawan_status, eventParam, NULL, 0);
                    }
                }
                else if(sscanf(cmd, "RECMACDATA,%d,%d", (int*)&rssi,(int*)&platformDataLen) == 2) //+RECDATA,rssi,<len>:<data>
                {
                    platformData = (uint8_t *)strchr(buf, ':');
                    //原始数据事件
                    loraEventHandler(event_cloud_data, ep_cloud_data_raw, platformData, platformDataLen);

                    uint8_t datapointType = ProtocolParserPlatformData(platformData+1, platformDataLen);
                    loraBuffer.rssi = rssi;
                    if(datapointType == CUSTOMER_DEFINE_DATA){
                        loraBuffer.available = true;
                        loraBuffer.bufferSize = platformDataLen;
                        memcpy(loraBuffer.buffer,platformData+1,platformDataLen);
                        if(loraEventHandler != NULL) {
                            loraEventHandler(event_cloud_data,ep_cloud_data_custom,platformData+1,platformDataLen); //数据的第一个字节为0x32　用户自定义数据
                        }
                    } else {
                        if(loraEventHandler != NULL){
                            loraEventHandler(event_cloud_data,ep_cloud_data_datapoint,NULL,0);
                        }
                    }
                }
                else if(sscanf(cmd, "RECRADIODATA,%d,%d", (int*)&rssi,(int*)&platformDataLen) == 2)
                {
                    platformData = (uint8_t *)strchr(buf, ':');
                    loraBuffer.available = true;
                    loraBuffer.rssi = rssi;
                    loraBuffer.bufferSize = platformDataLen;
                    memcpy(loraBuffer.buffer,platformData+1,platformDataLen);
                    if(loraEventHandler != NULL){
                        loraEventHandler(event_lora_radio_status,ep_lora_radio_rx_done,platformData+1,platformDataLen);
                    }
                }
            }
            /*******************************************/
            if (cb)
            {
                int len = LENGTH(ret);
                int ret = cb(type, buf, len, param);
                if (WAIT != ret)
                    return ret;
            }

            if (type == TYPE_OK)
                return RESP_OK;
            if (type == TYPE_BUSY)
                return RESP_BUSY;
            if (type == TYPE_SENDING)
                return RESP_SENDING;
            if (type == TYPE_FAIL)
                return RESP_FAIL;
            if (type == TYPE_ERROR)
                return RESP_ERROR;
            if (type == TYPE_PROMPT)
                return RESP_PROMPT;
            if (type == TYPE_ABORTED)
                return RESP_ABORTED; // This means the current command was ABORTED, so retry your command if critical.
        }
        // relax a bit
    }while (!TIMEOUT(start, timeout_ms) && !cancelAllOperations);

    return WAIT;
}

//-------AT指令协议数据处理接口-------
bool ProtocolParserInit(void)
{
    PipeInit(&pipeRx,PIPE_MAX_SIZE,NULL);

    if(!parserInitDone)
    {
        cancelAllOperations = false;

        bool continue_cancel = false;
        bool retried_after_reset = false;

        int i = 10;
        while (i--)
        {
            if (cancelAllOperations)
            {
                continue_cancel = true;
                ProtocolParserResume(); // make sure we can talk to the modem
            }

            ProtocolParserSendFormated("AT\r\n");
            int r = ProtocolParserWaitFinalResp(NULL,NULL,1000);
            if(RESP_OK == r)
            {
                break;
            }
            else if (i==0 && !retried_after_reset)
            {
                retried_after_reset = true; // only perform reset & retry sequence once
                i = 10;
            }
        }

        if (i < 0)
        {
            continue_cancel = true;
            log_v("[ No Reply from Modem ]\r\n");
        }

        if (continue_cancel)
        {
            ProtocolParserCancel();
            return false; //串口不通 通讯失败
        }

        ProtocolParserSendFormated("ATE0\r\n"); //关闭回显
        delay(50);
        log_v("protocol parser init done\r\n");
        parserInitDone = true;

        return true;
    }
    else
    {
        return true;
    }
}

//接收串口数据存入缓冲区
void ProtocolPutPipe(uint8_t c)
{
    SerialPipeRxIrqBuf(c);
}

//模组主动下发的数据
void ProtocolModuleActiveSendHandle(void)
{
    ProtocolParserWaitFinalResp(NULL, NULL, 0);
}

//发送数据点数据
bool ProtocolSendPlatformData(uint8_t frameType, uint8_t port, const uint8_t *buffer, uint16_t length, uint32_t timeout)
{
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+SENDMACDATA=%d,%d,%d,%d\r\n",frameType,port,timeout,length);
        if (RESP_OK == ProtocolParserWaitFinalResp(NULL, NULL,5000)){
            if (RESP_PROMPT == ProtocolParserWaitFinalResp(NULL, NULL,5000)){
                SerialPipePut(buffer,(int)length,false);
                if (RESP_SENDING == ProtocolParserWaitFinalResp(NULL, NULL,5000)){
                    return true;
                }
            }
        }
    }
    return false;
}

//发送P2P数据
bool ProtocolSendRadioData(const uint8_t *buffer, uint16_t length,uint32_t timeout)
{
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+SENDRADIODATA=%d,%d\r\n",timeout,length);
        if (RESP_OK == ProtocolParserWaitFinalResp(NULL, NULL,5000)){
            if (RESP_PROMPT == ProtocolParserWaitFinalResp(NULL, NULL,5000)){
                SerialPipePut(buffer,(int)length,false);
                if (RESP_SENDING == ProtocolParserWaitFinalResp(NULL, NULL,5000)){
                    return true;
                }
            }
        }
    }
    return false;
}

//将模组重启
bool ProtocolExecuteRestart(void)
{
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+RST\r\n");
        if (RESP_OK == ProtocolParserWaitFinalResp(NULL, NULL,5000))
        {
            return true;
        }
    }
    return false;
}

//将模组恢复出厂
bool ProtocolExecuteRestore(void)
{
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+RESTORE\r\n");
        if (RESP_OK == ProtocolParserWaitFinalResp(NULL, NULL,5000))
        {
            return true;
        }
    }
    return false;
}

bool ProtocolSetupSystemSleep(char *pin, uint8_t edgeTriggerMode, uint32_t timeout)
{
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+SLEEP=\"%s\",%d,%d\r\n",pin, edgeTriggerMode, timeout);
        if (RESP_OK == ProtocolParserWaitFinalResp(NULL, NULL,5000))
        {
            return true;
        }
    }
    return false;
}

bool ProtocolExecuteDFU(void)
{
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+DFU\r\n");
        if (RESP_OK == ProtocolParserWaitFinalResp(NULL, NULL,5000))
        {
            return true;
        }
    }
    return false;
}

//获取模块信息回调
int ProtocolQueryInfoCallback(int type, const char* buf, int len, device_info_t *info)
{
    if (info && (type == TYPE_PLUS))
    {
        if (sscanf(buf, "+INFO:\"%[^\"]\",\"%[^\"]\",\"%[^\"]\",%d\r\n", info->module_version,info->module_type,info->device_id,(int*)&info->at_mode) == 4)
        {
        }
    }
    return WAIT;
}

//获取模块信息
bool ProtocolQueryInfo(device_info_t *info)
{
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+INFO?\r\n");
        if (RESP_OK == ProtocolParserWaitFinalResp((callbackPtr)ProtocolQueryInfoCallback, info, 5000))
        {
            return true;
        }
    }
    return false;
}

//设置设备信息
bool ProtocolSetupDevice(char *product_id, char *hardware_version, char *software_version)
{
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+DEVICE=\"%s\",\"%s\",\"%s\"\r\n",product_id,hardware_version,software_version);
        if (RESP_OK == ProtocolParserWaitFinalResp(NULL,NULL,5000))
        {
            return true;
        }
    }
    return false;
}

bool ProtocolSetupProtocolMode(uint8_t mode)
{
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+SETPROTOCOL=%d\r\n",mode);
        if (RESP_OK == ProtocolParserWaitFinalResp(NULL, NULL,5000))
        {
            return true;
        }
    }
    return false;
}

int ProtocolQueryMacClassTypeCallback(int type, const char* buf, int len, int *mode)
{
    if (mode && (type == TYPE_PLUS))
    {
        int classType;
        if (sscanf(buf, "+MACCLASS:%d\r\n", &classType) == 1)
        {
            *mode = classType;
        }
    }
    return WAIT;
}

//查询class类型
int8_t ProtocolQueryMacClassType(void)
{
    int classType;
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+MACCLASS?\r\n");
        if (RESP_OK == ProtocolParserWaitFinalResp((callbackPtr)ProtocolQueryMacClassTypeCallback, &classType,5000))
        {
            return (int8_t)classType;
        }
    }
    return -1;
}

bool ProtocolSetupMacClassType(uint8_t classType)
{
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+MACCLASS=%d\r\n",classType);
        if (RESP_OK == ProtocolParserWaitFinalResp(NULL, NULL,5000))
        {
            return true;
        }
    }
    return false;
}

int ProtocolExecuteMacJoinCallback(int type, const char* buf, int len, int *status)
{
    if (status && (type == TYPE_PLUS))
    {
        int sendStauts;
        if (sscanf(buf, "+MACJOIN:%d\r\n", &sendStauts) == 1)
        {
            *status = sendStauts;
        }
    }
    return WAIT;
}

int ProtocolExecuteMacJoin(uint8_t type, uint32_t timeout)
{
    if (parserInitDone)
    {
        int status;
        ProtocolParserSendFormated("AT+MACJOIN=%d,%d\r\n",type,timeout);
        if (RESP_OK == ProtocolParserWaitFinalResp((callbackPtr)ProtocolExecuteMacJoinCallback, &status,5000))
        {
            return status; //1非lorawan模式 2 断开连接 3 入网忙4 入网中
        }
    }
    return -1;
}

int ProtocolQueryMacDeviceAddrCallback(int type, const char* buf, int len, char *devaddr)
{
    if (devaddr && (type == TYPE_PLUS))
    {
        if (sscanf(buf, "+MACDEVADDR:\"%[^\"]\"\r\n", devaddr) == 1)
        {
        }
    }
    return WAIT;
}

bool ProtocolQueryMacDeviceAddr(char *devAddr)
{
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+MACDEVADDR?\r\n");
        if (RESP_OK == ProtocolParserWaitFinalResp((callbackPtr)ProtocolQueryMacDeviceAddrCallback, devAddr, 5000))
        {
            return true;
        }
    }
    return false;
}

int ProtocolQueryMacDeviceEuiCallback(int type, const char* buf, int len, char *deveui)
{
    if (deveui && (type == TYPE_PLUS))
    {
        if (sscanf(buf, "+MACDEVEUI:\"%[^\"]\"\r\n", deveui) == 1)
        {
        }
    }
    return WAIT;
}

bool ProtocolQueryMacDeviceEui(char *deveui)
{
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+MACDEVEUI?\r\n");
        if (RESP_OK == ProtocolParserWaitFinalResp((callbackPtr)ProtocolQueryMacDeviceEuiCallback, deveui, 5000))
        {
            return true;
        }
    }
    return false;
}

int ProtocolQueryMacAppEuiCallback(int type, const char* buf, int len, char *appeui)
{
    if (appeui && (type == TYPE_PLUS))
    {
        if (sscanf(buf, "+MACAPPEUI:\"%[^\"]\"\r\n", appeui) == 1)
        {
        }
    }
    return WAIT;
}

bool ProtocolQueryMacAppEui(char *appeui)
{
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+MACAPPEUI?\r\n");
        if (RESP_OK == ProtocolParserWaitFinalResp((callbackPtr)ProtocolQueryMacAppEuiCallback, appeui, 5000))
        {
            return true;
        }
    }
    return false;
}

bool ProtocolSetupMacOTAAParams(char *devEui, char *appEui, char *appKey)
{
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+MACOTAAPARAMS=\"%s\",\"%s\",\"%s\"\r\n",devEui,appEui,appKey);
        if (RESP_OK == ProtocolParserWaitFinalResp(NULL,NULL,5000))
        {
            return true;
        }
    }
    return false;
}

bool ProtocolSetupMacABPParams(char *devAddr, char *nwkSkey, char *appSkey)
{
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+MACABPPARAMS=\"%s\",\"%s\",\"%s\"\r\n",devAddr,nwkSkey,appSkey);
        if (RESP_OK == ProtocolParserWaitFinalResp(NULL,NULL,5000))
        {
            return true;
        }
    }
    return false;
}

bool ProtocolSetupMacPowerIndex(uint8_t index)
{
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+MACPWRINDEX=%d\r\n",index);
        if (RESP_OK == ProtocolParserWaitFinalResp(NULL, NULL,5000))
        {
            return true;
        }
    }
    return false;
}

int ProtocolQueryMacDatarateCallback(int type, const char* buf, int len, int *datarate)
{
    if (datarate && (type == TYPE_PLUS))
    {
        int macDatarate;
        if (sscanf(buf, "+MACDR:%d\r\n", &macDatarate) == 1)
        {
            *datarate = macDatarate;
        }
    }
    return WAIT;
}

int8_t ProtocolQueryMacDatarate(void)
{
    int datarate;
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+MACDR?\r\n");
        if (RESP_OK == ProtocolParserWaitFinalResp((callbackPtr)ProtocolQueryMacDatarateCallback, &datarate,5000))
        {
            return (int8_t)datarate;
        }
    }
    return -1;
}

bool ProtocolSetupMacDatarate(uint8_t datarate)
{
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+MACDR=%d\r\n",datarate);
        if (RESP_OK == ProtocolParserWaitFinalResp(NULL, NULL,5000))
        {
            return true;
        }
    }
    return false;
}

int ProtocolQueryMacAdrCallback(int type, const char* buf, int len, int *adrEnabled)
{
    if (adrEnabled && (type == TYPE_PLUS))
    {
        int macAdrEnabled;
        if (sscanf(buf, "+MACADR:%d\r\n", &macAdrEnabled) == 1)
        {
            *adrEnabled = macAdrEnabled;
        }
    }
    return WAIT;
}

int8_t ProtocolQueryMacAdr(void)
{
    int adrEnabled;
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+MACADR?\r\n");
        if (RESP_OK == ProtocolParserWaitFinalResp((callbackPtr)ProtocolQueryMacAdrCallback, &adrEnabled,5000))
        {
            return (int8_t)adrEnabled;
        }
    }
    return -1;
}

bool ProtocolSetupMacAdr(uint8_t adrEnabled)
{
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+MACADR=%d\r\n",adrEnabled);
        if (RESP_OK == ProtocolParserWaitFinalResp(NULL, NULL,5000))
        {
            return true;
        }
    }
    return false;
}

bool ProtocolSetupMacDutyCyclePrescaler(uint16_t dutyCycle)
{
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+MACDCYCLEPS=%d\r\n",dutyCycle);
        if (RESP_OK == ProtocolParserWaitFinalResp(NULL, NULL,5000))
        {
            return true;
        }
    }
    return false;
}

int ProtocolQueryMacDutyCyclePrescalerCallback(int type, const char* buf, int len, int *dutyCycle)
{
    if (dutyCycle && (type == TYPE_PLUS))
    {
        int macDutyCycle;
        if (sscanf(buf, "+MACDCYCLEPS:%d\r\n", &macDutyCycle) == 1)
        {
            *dutyCycle = macDutyCycle;
        }
    }
    return WAIT;
}

uint16_t ProtocolQueryMacDutyCyclePrescaler(void)
{
    int dutyCycle;
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+MACDCYCLEPS?\r\n");
        if (RESP_OK == ProtocolParserWaitFinalResp((callbackPtr)ProtocolQueryMacDutyCyclePrescalerCallback, &dutyCycle,5000))
        {
            return (uint16_t)dutyCycle;
        }
    }
    return 0;
}

int ProtocolQueryMacChannelFreqCallback(int type, const char* buf, int len, int *freq)
{
    if (freq && (type == TYPE_PLUS))
    {
        int macChannelFreq;
        if (sscanf(buf, "+MACCHFREQ:%d\r\n", &macChannelFreq) == 1)
        {
            *freq = macChannelFreq;
        }
    }
    return WAIT;
}

uint32_t ProtocolQueryMacChannelFreq(uint8_t channelId)
{
    int channelFreq;
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+MACCHFREQ?%d\r\n",channelId);
        if (RESP_OK == ProtocolParserWaitFinalResp((callbackPtr)ProtocolQueryMacChannelFreqCallback, &channelFreq,5000))
        {
            return (uint32_t)channelFreq;
        }
    }
    return 0;
}

bool ProtocolSetupMacChannelFreq(uint8_t channelId, uint32_t freq)
{
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+MACCHFREQ=%d,%d\r\n",channelId,freq);
        if (RESP_OK == ProtocolParserWaitFinalResp(NULL, NULL,5000))
        {
            return true;
        }
    }
    return false;
}

int ProtocolQueryMacChannelDRRangeCallback(int type, const char* buf, int len, channel_params *drRange)
{
    if (drRange && (type == TYPE_PLUS))
    {
        if (sscanf(buf, "+MACCHDRRANGE:%d,%d\r\n", (int*)&drRange->minDR,(int*)&drRange->maxDR) == 2)
        {
        }
    }
    return WAIT;
}

bool ProtocolQueryMacChannelDRRange(uint8_t channelId, channel_params *drRange)
{
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+MACCHDRRANGE?%d\r\n",channelId);
        if (RESP_OK == ProtocolParserWaitFinalResp((callbackPtr)ProtocolQueryMacChannelDRRangeCallback, drRange, 5000))
        {
            return true;
        }
    }
    return false;
}

bool ProtocolSetupMacChannelDRRange(uint8_t channelId, uint8_t minDR, uint8_t maxDR)
{
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+MACCHDRRANGE=%d,%d,%d\r\n",channelId,minDR,maxDR);
        if (RESP_OK == ProtocolParserWaitFinalResp(NULL, NULL,5000))
        {
            return true;
        }
    }
    return false;
}

int ProtocolQueryMacChannelEnableCallback(int type, const char* buf, int len, int *chEnable)
{
    if (chEnable && (type == TYPE_PLUS))
    {
        int macChannelEnable;
        if (sscanf(buf, "+MACCH:%d\r\n", &macChannelEnable) == 1)
        {
            *chEnable = macChannelEnable;
        }
    }
    return WAIT;
}

int8_t ProtocolQueryMacChannelEnable(uint8_t channelId)
{
    int channelEnable;
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+MACCH?\r\n");
        if (RESP_OK == ProtocolParserWaitFinalResp((callbackPtr)ProtocolQueryMacChannelEnableCallback, &channelEnable,5000))
        {
            return (int8_t)channelEnable;
        }
    }
    return -1;
}

bool ProtocolSetupMacChannelEnable(uint8_t channelId, uint8_t enable)
{
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+MACCH=%d,%d\r\n",channelId,enable);
        if (RESP_OK == ProtocolParserWaitFinalResp(NULL, NULL,5000))
        {
            return true;
        }
    }
    return false;
}

int ProtocolQueryMacConfirmedTrialsCallback(int type, const char* buf, int len, int *cnt)
{
    if (cnt && (type == TYPE_PLUS))
    {
        int macCnt;
        if (sscanf(buf, "+MACCOMFTRIALS:%d\r\n", &macCnt) == 1)
        {
            *cnt= macCnt;
        }
    }
    return WAIT;

}

uint8_t ProtocolQueryMacConfirmedTrials(void)
{
    int confimedTrials;
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+MACCOMFTRIALS?\r\n");
        if (RESP_OK == ProtocolParserWaitFinalResp((callbackPtr)ProtocolQueryMacConfirmedTrialsCallback, &confimedTrials,5000))
        {
            return (uint8_t)confimedTrials;
        }
    }
    return 0;

}

bool ProtocolSetupMacConfirmedTrials(uint8_t count)
{
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+MACCOMFTRIALS=%d\r\n",count);
        if (RESP_OK == ProtocolParserWaitFinalResp(NULL, NULL,5000))
        {
            return true;
        }
    }
    return false;
}

int ProtocolQueryMacUnconfirmedTrialsCallback(int type, const char* buf, int len, int *cnt)
{
    if (cnt && (type == TYPE_PLUS))
    {
        int macCnt;
        if (sscanf(buf, "+MACUNCOMFTRIALS:%d\r\n", &macCnt) == 1)
        {
            *cnt= macCnt;
        }
    }
    return WAIT;
}

uint8_t ProtocolQueryMacUncomfirmedTrials(void)
{
    int uncomfirmedTrials;
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+MACUNCOMFTRIALS?\r\n");
        if (RESP_OK == ProtocolParserWaitFinalResp((callbackPtr)ProtocolQueryMacUnconfirmedTrialsCallback, &uncomfirmedTrials,5000))
        {
            return (uint8_t)uncomfirmedTrials;
        }
    }
    return 0;
}

bool ProtocolSetupMacUncomfirmedTrials(uint8_t count)
{
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+MACUNCOMFTRIALS=%d\r\n",count);
        if (RESP_OK == ProtocolParserWaitFinalResp(NULL, NULL,5000))
        {
            return true;
        }
    }
    return false;
}

int ProtocolQueryMacJoinTrialsCallback(int type, const char* buf, int len, int *cnt)
{
    if (cnt && (type == TYPE_PLUS))
    {
        int macCnt;
        if (sscanf(buf, "+MACJOINTRIALS:%d\r\n", &macCnt) == 1)
        {
            *cnt= macCnt;
        }
    }
    return WAIT;
}

uint8_t ProtocolQueryMacJoinTrials(void)
{
    int joinTrials;
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+MACJOINTRIALS?\r\n");
        if (RESP_OK == ProtocolParserWaitFinalResp((callbackPtr)ProtocolQueryMacJoinTrialsCallback, &joinTrials,5000))
        {
            return (uint8_t)joinTrials;
        }
    }
    return 0;
}

bool ProtocolSetupMacJoinTrials(uint8_t count)
{
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+MACJOINTRIALS=%d\r\n",count);
        if (RESP_OK == ProtocolParserWaitFinalResp(NULL, NULL,5000))
        {
            return true;
        }
    }
    return false;
}

int ProtocolQueryMacMarginCallback(int type, const char* buf, int len, int *margin)
{
    if (margin && (type == TYPE_PLUS))
    {
        int macMargin;
        if (sscanf(buf, "+MACMRGN:%d\r\n", &margin) == 1)
        {
            *margin = macMargin;
        }
    }
    return WAIT;
}

uint8_t ProtocolQueryMacMargin(void)
{
    int margin;
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+MACMRGN?\r\n");
        if (RESP_OK == ProtocolParserWaitFinalResp((callbackPtr)ProtocolQueryMacMarginCallback, &margin,5000))
        {
            return (uint8_t)margin;
        }
    }
    return 0;
}

int ProtocolQueryMacGatewayNumberCallback(int type, const char* buf, int len, int *gwNumber)
{
    if (gwNumber && (type == TYPE_PLUS))
    {
        int macGwNbmer;
        if (sscanf(buf, "+MACGWNB:%d\r\n", &macGwNbmer) == 1)
        {
            *gwNumber = macGwNbmer;
        }
    }
    return WAIT;
}

uint8_t ProtocolQueryMacGatewayNumber(void)
{
    int gatewayNb;
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+MACGWNB?\r\n");
        if (RESP_OK == ProtocolParserWaitFinalResp((callbackPtr)ProtocolQueryMacGatewayNumberCallback, &gatewayNb,5000))
        {
            return (uint8_t)gatewayNb;
        }
    }
    return 0;
}

int ProtocolQueryMacSnrCallback(int type, const char* buf, int len, int *snr)
{
    if (snr && (type == TYPE_PLUS))
    {
        int macSnr;
        if (sscanf(buf, "+MACSNR:%d\r\n", &macSnr) == 1)
        {
            *snr= macSnr;
        }
    }
    return WAIT;
}

uint8_t ProtocolQueryMacSnr(void)
{
    int snr;
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+MACSNR?\r\n");
        if (RESP_OK == ProtocolParserWaitFinalResp((callbackPtr)ProtocolQueryMacSnrCallback, &snr,5000))
        {
            return (uint8_t)snr;
        }
    }
    return 0;
}

int ProtocolQueryMacRssiCallback(int type, const char* buf, int len, int *rssi)
{
    if (rssi && (type == TYPE_PLUS))
    {
        int macRssi;
        if (sscanf(buf, "+MACRSSI:%d\r\n", &macRssi) == 1)
        {
            *rssi = macRssi;
        }
    }
    return WAIT;
}

int ProtocolQueryMacRssi(void)
{
    int rssi;
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+MACRSSI?\r\n");
        if (RESP_OK == ProtocolParserWaitFinalResp((callbackPtr)ProtocolQueryMacRssiCallback, &rssi,5000))
        {
            return rssi;
        }
    }
    return 0;
}

int ProtocolQueryMacRX1DelayCallback(int type, const char* buf, int len, int *rx1Ms)
{
    if (rx1Ms && (type == TYPE_PLUS))
    {
        int macRx1Ms;
        if (sscanf(buf, "+MACRX1DELAY:%d\r\n", &macRx1Ms) == 1)
        {
            *rx1Ms = macRx1Ms;
        }
    }
    return WAIT;
}

uint32_t ProtocolQueryMacRX1Delay(void)
{
    int rx1Ms;
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+MACRX1DELAY?\r\n");
        if (RESP_OK == ProtocolParserWaitFinalResp((callbackPtr)ProtocolQueryMacRX1DelayCallback, &rx1Ms,5000))
        {
            return (uint32_t)rx1Ms;
        }
    }
    return 0;
}

bool ProtocolSetupMacRX1Delay(uint32_t rx1Ms)
{
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+MACRX1DELAY=%d\r\n",rx1Ms);
        if (RESP_OK == ProtocolParserWaitFinalResp(NULL, NULL,5000))
        {
            return true;
        }
    }
    return false;
}

int ProtocolQueryMacRX2ParamsCallback(int type, const char* buf, int len, channel_params *params)
{
    if (params && (type == TYPE_PLUS))
    {
        if (sscanf(buf, "+MACRX2PARAMS:%d,%d\r\n", (int*)&params->datarate,(int*)&params->freq) == 2)
        {
        }
    }
    return WAIT;
}

bool ProtocolQueryMacRX2Params(channel_params *rx2Params)
{
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+MACRX2PARAMS?\r\n");
        if (RESP_OK == ProtocolParserWaitFinalResp((callbackPtr)ProtocolQueryMacRX2ParamsCallback, rx2Params, 5000))
        {
            return true;
        }
    }
    return false;
}

bool ProtocolSetupMacRX2Params(uint8_t datarate, uint32_t freq)
{
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+MACRX2PARAMS=%d,%d\r\n",datarate,freq);
        if (RESP_OK == ProtocolParserWaitFinalResp(NULL, NULL,5000))
        {
            return true;
        }
    }
    return false;
}


bool ProtocoSetupMacUplinkCount(uint32_t count)
{
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+MACUPCOUNT=%d\r\n",count);
        if (RESP_OK == ProtocolParserWaitFinalResp(NULL, NULL,5000))
        {
            return true;
        }
    }
    return false;
}

int ProtocolQueryMacUplinkCountCallback(int type, const char* buf, int len, int *uplinkCnt)
{
    if (uplinkCnt && (type == TYPE_PLUS))
    {
        int macUplinkCnt;
        if (sscanf(buf, "+MACUPCOUNT:%d\r\n", &macUplinkCnt) == 1)
        {
            *uplinkCnt = macUplinkCnt;
        }
    }
    return WAIT;
}

int ProtocolQueryMacUplinkCount(void)
{
    int uplinkCnt;
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+MACUPCOUNT?\r\n");
        if (RESP_OK == ProtocolParserWaitFinalResp((callbackPtr)ProtocolQueryMacUplinkCountCallback, &uplinkCnt,5000))
        {
            return uplinkCnt;
        }
    }
    return -1;
}

bool ProtocolSetupMacDownlinkCount(uint32_t count)
{
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+MACDOWNCOUNT=%d\r\n",count);
        if (RESP_OK == ProtocolParserWaitFinalResp(NULL, NULL,5000))
        {
            return true;
        }
    }
    return false;
}

int ProtocolQueryMacDownlinkCountCallback(int type, const char* buf, int len, int *downlinkCnt)
{
    if (downlinkCnt && (type == TYPE_PLUS))
    {
        int macDownlinkCnt;
        if (sscanf(buf, "+MACUPLINK:%d\r\n", &macDownlinkCnt) == 1)
        {
            *downlinkCnt = macDownlinkCnt;
        }
    }
    return WAIT;
}

int ProtocolQueryMacDownlinkCount(void)
{
    int downlinkCnt;
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+MACDOWNCOUNT?\r\n");
        if (RESP_OK == ProtocolParserWaitFinalResp((callbackPtr)ProtocolQueryMacDownlinkCountCallback, &downlinkCnt,5000))
        {
            return (int)downlinkCnt;
        }
    }
    return -1;
}


bool ProtocolSetupRadioRx(uint32_t rxTimeout)
{
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+RADIORX=%d\r\n",rxTimeout);
        if (RESP_OK == ProtocolParserWaitFinalResp(NULL, NULL,5000))
        {
            return true;
        }
    }
    return false;
}

bool ProtocolExecuteRadioStartCad(void)
{
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+RADIOCAD\r\n");
        if (RESP_OK == ProtocolParserWaitFinalResp(NULL, NULL,5000))
        {
            return true;
        }
    }
    return false;
}

int ProtocolQueryRadioSnrCallback(int type, const char* buf, int len, int *snr)
{
    if (snr && (type == TYPE_PLUS))
    {
        int radioSnr;
        if (sscanf(buf, "+RADIOSNR:%d\r\n", &radioSnr) == 1)
        {
            *snr = radioSnr;
        }
    }
    return WAIT;
}

int ProtocolQueryRadioSnr(void)
{
    int snr;
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+RADIOSNR?\r\n");
        if (RESP_OK == ProtocolParserWaitFinalResp((callbackPtr)ProtocolQueryRadioSnrCallback, &snr,5000))
        {
        }
    }
    return snr;
}

int ProtocolQueryRadioRssiCallback(int type, const char* buf, int len, int *rssi)
{
    if (rssi && (type == TYPE_PLUS))
    {
        int radioRssi;
        if (sscanf(buf, "+RADIORSSI:%d\r\n", &radioRssi) == 1)
        {
            *rssi = radioRssi;
        }
    }
    return WAIT;
}

int ProtocolQueryRadioRssi(void)
{
    int rssi;
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+RADIORSSI?\r\n");
        if (RESP_OK == ProtocolParserWaitFinalResp((callbackPtr)ProtocolQueryRadioRssiCallback, &rssi,5000))
        {
        }
    }
    return rssi;
}

int ProtocolQueryRadioFreqCallback(int type, const char* buf, int len, int *freq)
{
    if (freq && (type == TYPE_PLUS))
    {
        int radioFreq;
        if (sscanf(buf, "+RADIOFREQ:%d\r\n", &radioFreq) == 1)
        {
            *freq = radioFreq;
        }
    }
    return WAIT;
}

int ProtocolQueryRadioFreq(void)
{
    int freq;
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+RADIOFREQ?\r\n");
        if (RESP_OK == ProtocolParserWaitFinalResp((callbackPtr)ProtocolQueryRadioFreqCallback, &freq,5000))
        {
            return freq;
        }
    }
    return -1;
}

bool ProtocolSetupRadioFreq(uint32_t freq)
{
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+RADIOFREQ=%d\r\n",freq);
        if (RESP_OK == ProtocolParserWaitFinalResp(NULL, NULL,5000))
        {
            return true;
        }
    }
    return false;
}

bool ProtocolSetupRadioMaxPayloadLen(uint8_t maxPayloadLen)
{
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+RADIOMAXPAYLOADLEN=%d\r\n",maxPayloadLen);
        if (RESP_OK == ProtocolParserWaitFinalResp(NULL, NULL,5000))
        {
            return true;
        }
    }
    return false;
}

int ProtocolQueryRadioMaxPayloadLenCallback(int type, const char* buf, int len, int *payloadLen)
{
    if (payloadLen && (type == TYPE_PLUS))
    {
        int radioPayloadLen;
        if (sscanf(buf, "+RADIOPAYLOAD:%d\r\n", &radioPayloadLen) == 1)
        {
            *payloadLen = radioPayloadLen;
        }
    }
    return WAIT;
}

int16_t ProtocolQueryRadioMaxPayloadLen(void)
{
    int payloadLen;
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+RADIOMAXPAYLOADLEN?\r\n");
        if (RESP_OK == ProtocolParserWaitFinalResp((callbackPtr)ProtocolQueryRadioMaxPayloadLenCallback, &payloadLen,5000))
        {
            return (uint16_t)payloadLen;
        }
    }
    return -1;
}

int ProtocolQueryRadioModeCallback(int type, const char* buf, int len, int *mode)
{
    if (mode && (type == TYPE_PLUS))
    {
        int radioMode;
        if (sscanf(buf, "+RADIOMODE:%d\r\n", &radioMode) == 1)
        {
            *mode = radioMode;
        }
    }
    return WAIT;
}

int8_t ProtocolQueryRadioMode(void)
{
    int mode;
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+RADIOMODE?\r\n");
        if (RESP_OK == ProtocolParserWaitFinalResp((callbackPtr)ProtocolQueryRadioModeCallback, &mode,5000))
        {
            return (int8_t)mode;
        }
    }
    return -1;
}

bool ProtocolSetupRadioMode(uint8_t mode)
{
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+RADIOMODE=%d\r\n",mode);
        if (RESP_OK == ProtocolParserWaitFinalResp(NULL, NULL,5000))
        {
            return true;
        }
    }
    return false;
}

bool ProtocolSetupRadioSf(uint8_t sf)
{
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+RADIOSF=%d\r\n",sf);
        if (RESP_OK == ProtocolParserWaitFinalResp(NULL, NULL,5000))
        {
            return true;
        }
    }
    return false;
}

int ProtocolQueryRadioSfCallback(int type, const char* buf, int len, int *sf)
{
    if (sf && (type == TYPE_PLUS))
    {
        int radioSf;
        if (sscanf(buf, "+RADIOSF:%d\r\n", &radioSf) == 1)
        {
            *sf = radioSf;
        }
    }
    return WAIT;
}

int8_t ProtocolQueryRadioSf(void)
{
    int sf;
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+RADIOSF?\r\n");
        if (RESP_OK == ProtocolParserWaitFinalResp((callbackPtr)ProtocolQueryRadioSfCallback, &sf,5000))
        {
            return (int8_t)sf;
        }
    }
    return -1;
}

bool ProtocolSetupRadioBw(uint8_t bw)
{
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+RADIOBW=%d\r\n",bw);
        if (RESP_OK == ProtocolParserWaitFinalResp(NULL, NULL,5000))
        {
            return true;
        }
    }
    return false;
}

int ProtocolQueryRadioBwCallback(int type, const char* buf, int len, int *bw)
{
    if (bw && (type == TYPE_PLUS))
    {
        int radioBw;
        if (sscanf(buf, "+RADIOBW:%d\r\n", &radioBw) == 1)
        {
            *bw = radioBw;
        }
    }
    return WAIT;
}

int8_t ProtocolQueryRadioBw(void)
{
    int bw;
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+RADIOBW?\r\n");
        if (RESP_OK == ProtocolParserWaitFinalResp((callbackPtr)ProtocolQueryRadioBwCallback, &bw,5000))
        {
            return (int8_t)bw;
        }
    }
    return -1;
}

bool ProtocolSetupRadioCoderate(uint8_t cr)
{
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+RADIOCR=%d\r\n",cr);
        if (RESP_OK == ProtocolParserWaitFinalResp(NULL, NULL,5000))
        {
            return true;
        }
    }
    return false;
}

int ProtocolQueryRadioCoderateCallback(int type, const char* buf, int len, int *cr)
{
    if (cr && (type == TYPE_PLUS))
    {
        int radioCr;
        if (sscanf(buf, "+RADIOCR:%d\r\n", &radioCr) == 1)
        {
            *cr = radioCr;
        }
    }
    return WAIT;
}

int8_t ProtocolQueryRadioCoderate(void)
{
    int cr;
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+RADIOCR?\r\n");
        if (RESP_OK == ProtocolParserWaitFinalResp((callbackPtr)ProtocolQueryRadioCoderateCallback, &cr,5000))
        {
            return (int8_t)cr;
        }
    }
    return -1;
}

bool ProtocolSetupRadioPreambleLen(uint16_t preambleLen)
{
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+RADIOPREAMBLELEN=%d\r\n",preambleLen);
        if (RESP_OK == ProtocolParserWaitFinalResp(NULL, NULL,5000))
        {
            return true;
        }
    }
    return false;
}

int ProtocolQueryRadioPreambleLenCallback(int type, const char* buf, int len, int *preambleLen)
{
    if (preambleLen && (type == TYPE_PLUS))
    {
        int radioPreambleLen;
        if (sscanf(buf, "+RADIOPREAMBLELEN:%d\r\n", &radioPreambleLen) == 1)
        {
            *preambleLen = radioPreambleLen;
        }
    }
    return WAIT;
}

int ProtocolQueryRadioPreambleLen(void)
{
    int preambleLen;
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+RADIOPREAMBLELEN?\r\n");
        if (RESP_OK == ProtocolParserWaitFinalResp((callbackPtr)ProtocolQueryRadioPreambleLenCallback, &preambleLen,5000))
        {
            return preambleLen;
        }
    }
    return -1;
}

int ProtocolQueryRadioFixLenOnCallback(int type, const char* buf, int len, int *fixLenOn)
{
    if (fixLenOn && (type == TYPE_PLUS))
    {
        int radioFixLenOn;
        if (sscanf(buf, "+RADIOFIXLENON:%d\r\n", &radioFixLenOn) == 1)
        {
            *fixLenOn = radioFixLenOn;
        }
    }
    return WAIT;

}

int8_t ProtocolQueryRadioFixLenOn(void)
{
    int fixLenOn;
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+RADIOFIXLENON?\r\n");
        if (RESP_OK == ProtocolParserWaitFinalResp((callbackPtr)ProtocolQueryRadioFixLenOnCallback, &fixLenOn,5000))
        {
            return (int8_t)fixLenOn;
        }
    }
    return -1;
}

bool ProtocolSetupRadioFixLenOn(uint8_t fixLenOn)
{
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+RADIOFIXLENON=%d\r\n",fixLenOn);
        if (RESP_OK == ProtocolParserWaitFinalResp(NULL, NULL,5000))
        {
            return true;
        }
    }
    return false;
}

bool ProtocolSetupRadioCrcEnabled(uint8_t crcEnabled)
{
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+RADIOCRC=%d\r\n",crcEnabled);
        if (RESP_OK == ProtocolParserWaitFinalResp(NULL, NULL,5000))
        {
            return true;
        }
    }
    return false;
}

int ProtocolQueryRadioCrcEnabledCallback(int type, const char* buf, int len, int *crcEnabled)
{
    if (crcEnabled && (type == TYPE_PLUS))
    {
        int radioCrcEnabled;
        if (sscanf(buf, "+RADIOCRC:%d\r\n", &radioCrcEnabled) == 1)
        {
            *crcEnabled = radioCrcEnabled;
        }
    }
    return WAIT;
}

int8_t ProtocolQueryRadioCrcEnabled(void)
{
    int crcEnabled;
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+RADIOCRC?\r\n");
        if (RESP_OK == ProtocolParserWaitFinalResp((callbackPtr)ProtocolQueryRadioCrcEnabledCallback, &crcEnabled,5000))
        {
            return (int8_t)crcEnabled;
        }
    }
    return -1;
}

int ProtocolQueryRadioFreqHopOnCallback(int type, const char* buf, int len, int *freqHopOn)
{
    if (freqHopOn && (type == TYPE_PLUS))
    {
        int radioFreqHopOn;
        if (sscanf(buf, "+RADIOFREQHOPON:%d\r\n", &radioFreqHopOn) == 1)
        {
            *freqHopOn = radioFreqHopOn;
        }
    }
    return WAIT;
}

int8_t ProtocolQueryRadioFreqHopOn(void)
{
    int freqHopOn;
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+RADIOFREQHOPON?\r\n");
        if (RESP_OK == ProtocolParserWaitFinalResp((callbackPtr)ProtocolQueryRadioFreqHopOnCallback, &freqHopOn,5000))
        {
            return (int8_t)freqHopOn;
        }
    }
    return -1;
}

bool ProtocolSetupRadioFreqHopOn(uint8_t hopOn)
{
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+RADIOFREQHOPON=%d\r\n",hopOn);
        if (RESP_OK == ProtocolParserWaitFinalResp(NULL, NULL,5000))
        {
            return true;
        }
    }
    return false;
}

int ProtocolQueryRadioHopPeriodCallback(int type, const char* buf, int len, int *hopPeriod)
{
    if (hopPeriod && (type == TYPE_PLUS))
    {
        int radioHopPeriod;
        if (sscanf(buf, "+RADIOIQ:%d\r\n", &radioHopPeriod) == 1)
        {
            *hopPeriod = radioHopPeriod;
        }
    }
    return WAIT;
}

uint8_t ProtocolQueryRadioHopPeriod(void)
{
    int hopPeriod;
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+RADIOHOPPERIOD?\r\n");
        if (RESP_OK == ProtocolParserWaitFinalResp((callbackPtr)ProtocolQueryRadioHopPeriodCallback, &hopPeriod,5000))
        {
            return (uint8_t)hopPeriod;
        }
    }
    return 0;
}

bool ProtocolSetupRadioHopPeriod(uint8_t hopPeriod)
{
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+RADIOHOPPERIOD=%d\r\n",hopPeriod);
        if (RESP_OK == ProtocolParserWaitFinalResp(NULL, NULL,5000))
        {
            return true;
        }
    }
    return false;

}

bool ProtocolSetupRadioIqInverted(uint8_t iqInverted)
{
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+RADIOIQ=%d\r\n",iqInverted);
        if (RESP_OK == ProtocolParserWaitFinalResp(NULL, NULL,5000))
        {
            return true;
        }
    }
    return false;
}

int ProtocolQueryRadioIqInvertedCallback(int type, const char* buf, int len, int *iqInverted)
{
    if (iqInverted && (type == TYPE_PLUS))
    {
        int radioIqverted;
        if (sscanf(buf, "+RADIOIQ:%d\r\n", &radioIqverted) == 1)
        {
            *iqInverted = radioIqverted;
        }
    }
    return WAIT;
}

int8_t ProtocolQueryRadioIqInverted(void)
{
    int iqInverted;
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+RADIOIQ?\r\n");
        if (RESP_OK == ProtocolParserWaitFinalResp((callbackPtr)ProtocolQueryRadioIqInvertedCallback, &iqInverted,5000))
        {
            return (int8_t)iqInverted;
        }
    }
    return -1;
}

bool ProtocolSetupRadioRxContinuous(uint8_t rxContinuous)
{
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+RADIORXMODE=%d\r\n",rxContinuous);
        if (RESP_OK == ProtocolParserWaitFinalResp(NULL, NULL,5000))
        {
            return true;
        }
    }
    return false;
}

int ProtocolQueryRadioRxContinuousCallback(int type, const char* buf, int len, int *rxContinuous)
{
    if (rxContinuous && (type == TYPE_PLUS))
    {
        int radioRxContinuous;
        if (sscanf(buf, "+RADIORXMODE:%d\r\n", &radioRxContinuous) == 1)
        {
            *rxContinuous = radioRxContinuous;
        }
    }
    return WAIT;
}

int8_t ProtocolQueryRadioRxContinuous(void)
{
    int rxContinuous;
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+RADIORXMODE?\r\n");
        if (RESP_OK == ProtocolParserWaitFinalResp((callbackPtr)ProtocolQueryRadioRxContinuousCallback, &rxContinuous,5000))
        {
            return (int8_t)rxContinuous;
        }
    }
    return -1;
}

bool ProtocolSetupRadioTxPower(uint8_t txPower)
{
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+RADIOPWR=%d\r\n",txPower);
        if (RESP_OK == ProtocolParserWaitFinalResp(NULL, NULL,5000))
        {
            return true;
        }
    }
    return false;
}

int ProtocolQueryRadioTxPowerCallback(int type, const char* buf, int len, int *txPower)
{
    if (txPower && (type == TYPE_PLUS))
    {
        int radioTxPower;
        if (sscanf(buf, "+RADIOPWR:%d\r\n", &radioTxPower) == 1)
        {
            *txPower = radioTxPower;
        }
    }
    return WAIT;
}

int8_t ProtocolQueryRadioTxPower(void)
{
    int txPower;
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+RADIOPWR?\r\n");
        if (RESP_OK == ProtocolParserWaitFinalResp((callbackPtr)ProtocolQueryRadioTxPowerCallback, &txPower,5000))
        {
            return (int8_t)txPower;
        }
    }
    return -1;
}

bool ProtocolSetupRadioFixPayloadLen(uint8_t payloadLen)
{
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+RADIOFIXPAYLOADLEN=%d\r\n",payloadLen);
        if (RESP_OK == ProtocolParserWaitFinalResp(NULL, NULL,5000))
        {
            return true;
        }
    }
    return false;
}

int ProtocolQueryRadioFixPayloadLenCallback(int type, const char* buf, int len, int *payloadLen)
{
    if (payloadLen && (type == TYPE_PLUS))
    {
        int radioPayloadLen;
        if (sscanf(buf, "+RADIOFIXPAYLOADLEN:%d\r\n", &radioPayloadLen) == 1)
        {
            *payloadLen = radioPayloadLen;
        }
    }
    return WAIT;
}

int16_t ProtocolQueryRadioFixPayloadLen(void)
{
    int payloadLen;
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+RADIOFIXPAYLOADLEN?\r\n");
        if (RESP_OK == ProtocolParserWaitFinalResp((callbackPtr)ProtocolQueryRadioFixPayloadLenCallback, &payloadLen,5000))
        {
            return (uint16_t)payloadLen;
        }
    }
    return -1;
}

bool ProtocolSetupRadioSymbTimeout(uint16_t symbTimeout)
{
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+RADIOSYMBTIMEOUT=%d\r\n",symbTimeout);
        if (RESP_OK == ProtocolParserWaitFinalResp(NULL, NULL,5000))
        {
            return true;
        }
    }
    return false;
}

int ProtocolQueryRadioSymbTimeoutCallback(int type, const char* buf, int len, int *symbTimeout)
{
    if (symbTimeout && (type == TYPE_PLUS))
    {
        int radioSymbTimeout;
        if (sscanf(buf, "+RADIOSYMBTIMEOUT:%d\r\n", &radioSymbTimeout) == 1)
        {
            *symbTimeout = radioSymbTimeout;
        }
    }
    return WAIT;
}

int16_t ProtocolQueryRadioSymbTimeout(void)
{
    int symbTimeout;
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+RADIOSYMBTIMEOUT?\r\n");
        if (RESP_OK == ProtocolParserWaitFinalResp((callbackPtr)ProtocolQueryRadioSymbTimeoutCallback, &symbTimeout,5000))
        {
            return (int16_t)symbTimeout;
        }
    }
    return -1;
}

bool ProtocolExecuteRadioSleep(void)
{
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+RADIOSLEEP\r\n");
        if (RESP_OK == ProtocolParserWaitFinalResp(NULL, NULL,5000))
        {
            return true;
        }
    }
    return false;
}


bool ProtocolSetupRadioWriteRegister(uint8_t addr, uint8_t val)
{
    if (parserInitDone)
    {
        ProtocolParserSendFormated("AT+RADIOREG=%d,%d\r\n",addr,val);
        if (RESP_OK == ProtocolParserWaitFinalResp(NULL, NULL,5000))
        {
            return true;
        }
    }
    return false;
}

int ProtocolQueryRadioReadRegisterCallback(int type, const char* buf, int len, int *regVal)
{
    if (regVal && (type == TYPE_PLUS))
    {
        int radioRegVal;
        if (sscanf(buf, "+RADIOREG:%d\r\n", &radioRegVal) == 1)
        {
            *regVal = radioRegVal;
        }
    }
    return WAIT;
}

int8_t ProtocolQueryRadioReadRegister(uint8_t addr)
{
    if (parserInitDone)
    {
        int regVal;
        ProtocolParserSendFormated("AT+RADIOREG?%d\r\n",addr);
        if (RESP_OK == ProtocolParserWaitFinalResp((callbackPtr)ProtocolQueryRadioReadRegisterCallback, &regVal, 5000))
        {
            return (int8_t)regVal;
        }
    }
    return -1;
}
