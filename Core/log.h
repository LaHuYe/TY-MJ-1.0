#ifndef LOG_H_
#define LOG_H_

#define LOG_CRITICAL    0X00
#define LOG_ERROR       0x01
#define LOG_WARNING     0X02
#define LOG_NOTIC       0X03
#define LOG_DEBUG       0x04
#define LOG_DUMP        0x05

/* GAgent 日志等级 通过该宏设置*/
#define LOG_LOGLEVEL         LOG_DEBUG//LOG_NOTIC
#define LOG_TIMERLEVEL       LOG_NOTIC//LOG_NOTIC
#define LOG_MEMLEVEL         LOG_DEBUG//linux版本不打印剩余内存
#define LOG_TRACELEVEL       LOG_NOTIC

#ifdef ELK_ENABLE
void rlog(uint8 level, const uint8 *message,...);
#endif

/*这个函数是纯粹的打印,不受等级影响*/
#define Log(format, args...)  printf( format, ##args ) /*这个地方替换成平台相关的打印函数*/

#define logAssert()    do{Log("[func:%s][line:%d]param is illegal.\r\n", __FUNCTION__, __LINE__);}while(0)

/*key 日志打印接口*/
#define keyPrintf(level,format, args...)\
{\
    if(LOG_LOGLEVEL>=level) \
    {\
        Log( "[key] "format, ##args);\
    }\
}

/*LED 日志打印接口*/
#define ledPrintf(level,format, args...)\
{\
    if(LOG_LOGLEVEL>=level) \
    {\
        Log( "[led] "format, ##args);\
    }\
}

/*adc 日志打印接口*/
#define adcPrintf(level,format, args...)\
{\
    if(LOG_LOGLEVEL>=level) \
    {\
        Log( "[adc] "format, ##args);\
    }\
}

/*bat 日志打印接口*/
#define batPrintf(level,format, args...)\
{\
    if(LOG_LOGLEVEL>=level) \
    {\
        Log( "[bat] "format, ##args);\
    }\
}

/*pressure 日志打印接口*/
#define pressurePrintf(level,format, args...)\
{\
    if(LOG_LOGLEVEL>=level) \
    {\
        Log( "[pressure] "format, ##args);\
    }\
}

/*motor 日志打印接口*/
#define motorPrintf(level,format, args...)\
{\
    if(LOG_LOGLEVEL>=level) \
    {\
        Log( "[pump] "format, ##args);\
    }\
}

/*app 日志打印接口*/
#define appPrintf(level,format, args...)\
{\
    if(LOG_LOGLEVEL>=level) \
    {\
        Log( "[app] "format, ##args);\
    }\
}

/*ngf 日志打印接口*/
#define ngfPrintf(level,format, args...)\
{\
    if(LOG_LOGLEVEL>=level) \
    {\
        Log( "NGF "format, ##args);\
    }\
}

//IWDG 日志打印接口
#define iwdgPrintf(level,format, args...)\
{\
    if(LOG_LOGLEVEL>=level) \
    {\
        Log( "IWDG "format, ##args);\
    }\
}

#endif
