#include "StatusLed.h"

/* private variables */
static const StatusLed_Driver* ledDriver;
#if STATUS_LED_MAX_NUM == -1
    static StatusLed* lastLed = STATUS_LED_NULL;
    
    #define __leds()            lastLed
    #define __next(LED)         LED = (LED)->Previous
#else
    static StatusLed leds[STATUS_LED_MAX_NUM] = {0};

    #define __leds()            leds
    #define __next(LED)         LED++
#endif // STATUS_LED_MAX_NUM == -1

#if STATUS_LED_IO_INIT
    #define __initPin(CONF)          if (ledDriver->initPin) { ledDriver->initPin(CONF); }
#else
    #define __initPin(CONF)
#endif

#if STATUS_LED_IO_DEINIT
    #define __deinitPin(CONF)        if (ledDriver->deinitPin) { ledDriver->deinitPin(CONF); }
#else
    #define __deinitPin(CONF)
#endif

#if STATUS_LED_ACTIVE_STATE
    #define __ledWritePin(LED)      ledDriver->writePin((LED)->Config, (StatusLed_PinState) ((LED)->State ^ (LED)->ActiveState))
#else
    #define __ledWritePin(LED)      ledDriver->writePin((LED)->Config, (StatusLed_PinState) (LED)->State)
#endif

/**
 * @brief set led driver to use
 * 
 * @param driver 
 */
void StatusLed_init(const StatusLed_Driver* driver) {
    ledDriver = driver;
}
/**
 * @brief handle led status and return next blink time
 * 
 * @return StatusLed_Timestamp 
 */
StatusLed_Timestamp StatusLed_handle(void) {
    if (!ledDriver) {
        return STATUS_LED_IDLE_TIME;
    }

    StatusLed_Timestamp timestamp = ledDriver->getTimestamp();
    StatusLed* pLed = __leds();
    StatusLed_Timestamp nextTimestamp = 0;

#if STATUS_LED_MAX_NUM == -1
    while (STATUS_LED_NULL != pLed)
#else
    StatusLed_LenType len = STATUS_LED_MAX_NUM;
    while (len-- > 0)
#endif
    {
    #if STATUS_LED_ENABLE_FLAG
        if (pLed->Enabled) {
    #endif // STATUS_LED_ENABLE_FLAG
    #if STATUS_LED_MAX_NUM > 0
        if (pLed->Configured) {
    #endif
        
        if (timestamp >= pLed->NextBlink 
        #if STATUS_LED_REPEAT
            && pLed->PatternIndex < pLed->Pattern->Len
        #endif
        ) {
            pLed->NextBlink = timestamp + pLed->Pattern->Cycles[pLed->PatternIndex].Times[pLed->State];
            // get next timestamp
            if (nextTimestamp <= pLed->NextBlink && nextTimestamp != 0) {
                nextTimestamp = pLed->NextBlink;
            }
            // toggle led state
            pLed->State = !pLed->State;
            __ledWritePin(pLed);
            if (!pLed->State) {
                if (++pLed->PatternIndex >= pLed->Pattern->Len) {
                #if STATUS_LED_CALLBACK
                    if (pLed->onFinish) {
                        pLed->onFinish(pLed);
                    }
                #endif
                #if STATUS_LED_REPEAT
                    if (pLed->Repeat) {
                #endif
                    pLed->PatternIndex = 0;
                #if STATUS_LED_REPEAT
                    }
                #endif
                }
            }
        }

    #if STATUS_LED_MAX_NUM > 0
        }
    #endif
    #if STATUS_LED_ENABLE_FLAG
        }
    #endif // STATUS_LED_ENABLE_FLAG
        __next(pLed);
    }
    return nextTimestamp ? nextTimestamp - timestamp : STATUS_LED_IDLE_TIME;
}
/**
 * @brief reset current pattern
 * @param led 
 */
void StatusLed_reset(StatusLed* led) {
    led->State = StatusLed_LedState_On;
    led->NextBlink = 0;
}
/**
 * @brief set new pattern for led
 * 
 * @param led 
 * @param pattern 
 */
void StatusLed_setPattern(StatusLed* led, const StatusLed_Pattern* pattern) {
    led->Pattern = pattern;
    StatusLed_reset(led);
}
/**
 * @brief return led pattern
 * 
 * @param led 
 * @return const StatusLed_Pattern* 
 */
const StatusLed_Pattern* StatusLed_getPattern(StatusLed* led) {
    return led->Pattern;
}
/**
 * @brief config led pin
 * 
 * @param led 
 * @param config 
 */
void StatusLed_setConfig(StatusLed* led, const StatusLed_PinConfig* config) {
    led->Config = config;
}
/**
 * @brief return led pin config
 * 
 * @param led 
 * @return const StatusLed_PinConfig* 
 */
const StatusLed_PinConfig* StatusLed_getConfig(StatusLed* led) {
    return led->Config;
}
#if STATUS_LED_MAX_NUM > 0
/**
 * @brief return led space in memory
 * 
 * @return StatusLed* 
 */
StatusLed* StatusLed_new(void) {
    uint8_t len = STATUS_LED_MAX_NUM;
    StatusLed* pLed = leds;
    while (len-- > 0) {
        if (!pLed->Configured) {
            return pLed;
        }
        pLed++;
    }
    return STATUS_LED_NULL;
}
#endif // STATUS_LED_MAX_NUM
/**
 * @brief add led to memory
 * 
 * @param led 
 * @param config 
 * @return StatusLed_Status 
 */
StatusLed_Status StatusLed_add(StatusLed* led, const StatusLed_PinConfig* config) {
    // check for null
    if (STATUS_LED_NULL == led) {
        return StatusLed_Status_Null;
    }
    // add new led to list
    led->State = StatusLed_LedState_On;
    led->Pattern = STATUS_LED_PATTERN_NULL;
    led->PatternIndex = 0;
    led->NextBlink = 0;
    StatusLed_setConfig(led, config);
    // init IOs
    __initPin(config);
#if STATUS_LED_MAX_NUM == -1
    // add key to linked list
    led->Previous = lastLed;
    lastLed = led;
#endif // KEY_MAX_NUM == -1
    led->Configured = 1;
    led->Enabled = 1;
    return StatusLed_Status_Ok;
}
/**
 * @brief remove led from list
 * 
 * @param remove 
 * @return StatusLed_Status 
 */
StatusLed_Status StatusLed_remove(StatusLed* remove) {
#if STATUS_LED_MAX_NUM == -1
    StatusLed* pLed = lastLed;
    // check last key first
    if (remove == pLed) {
        // deinit IO
        __deinitPin(remove->Config);
        // remove key dropped from link list
        pLed->Previous = remove->Previous;
        remove->Previous = STATUS_LED_NULL;
        remove->Configured = 0;
        remove->Enabled = 0;
        return StatusLed_Status_Ok;
    }
    while (STATUS_LED_NULL != pLed) {
        if (remove == pLed->Previous) {
            // deinit IO
            __deinitPin(remove->Config);
            // remove key dropped from link list
            pLed->Previous = remove->Previous;
            remove->Previous = STATUS_LED_NULL;
            remove->Configured = 0;
            remove->Enabled = 0;
            return StatusLed_Status_Ok;
        }
        pLed = pLed->Previous;
    }
#else
    uint8_t len = STATUS_LED_MAX_NUM;
    StatusLed* pLed = leds;
    while (len-- > 0) {
        if (remove == pLed && pLed->Configured) {
            pLed->Configured = 0;
            pLed->Enabled = 0;
            return 1;
        }
        pLed++;
    }
#endif // STATUS_LED_MAX_NUM == -1
    return StatusLed_Status_NotFound;
}
/**
 * @brief find a led by pin config
 * 
 * @param config 
 * @return StatusLed* 
 */
StatusLed* StatusLed_find(const StatusLed_PinConfig* config) {
#if STATUS_LED_MAX_NUM == -1
    StatusLed* pLed = lastLed;
    while (STATUS_LED_NULL != pLed) {
        if (config == pLed->Config) {
            return pLed;
        }
        pLed = pLed->Previous;
    }
#else
    uint8_t len = STATUS_LED_MAX_NUM;
    StatusLed* pLed = leds;
    while (len-- > 0) {
        if (config == pLed->Config) {
            return pLed;
        }
        pLed++;
    }
#endif // STATUS_LED_MAX_NUM == -1
    return STATUS_LED_NULL;
}

#if STATUS_LED_CALLBACK
/**
 * @brief set on finish callback
 * 
 * @param led 
 * @param fn 
 */
void StatusLed_onFinish(StatusLed* led, StatusLed_CallbackFn fn) {
    led->onFinish = fn;
}
#endif // STATUS_LED_CALLBACK

#if STATUS_LED_ACTIVE_STATE
/**
 * @brief set led active state
 * 
 * @param led 
 * @param state 
 */
void StatusLed_setActiveState(StatusLed* led, StatusLed_ActiveState state) {
    led->ActiveState = state;
}
/**
 * @brief 
 * 
 * @param led 
 * @return StatusLed_ActiveState 
 */
StatusLed_ActiveState StatusLed_getActiveState(StatusLed* led) {
    return (StatusLed_ActiveState) led->ActiveState;
}
#endif

#if STATUS_LED_ENABLE_FLAG
/**
 * @brief set enable/disable led
 * 
 * @param led 
 * @param enabled 
 */
void StatusLed_setEnabled(StatusLed* led, uint8_t enabled) {
    led->Enabled = enabled;
}
/**
 * @brief get enable/disable led
 * 
 * @param led 
 * @return uint8_t 
 */
uint8_t StatusLed_isEnabled(StatusLed* led) {
    return led->Enabled;
}
#endif

#if STATUS_LED_ARGS
/**
 * @brief set user arguments
 * 
 * @param args 
 */
void StatusLed_setArgs(StatusLed* led, void* args) {
    led->Args = args;
}
/**
 * @brief return user arguments
 * 
 * @param led 
 * @return void* 
 */
void* StatusLed_getArgs(StatusLed* led) {
    return led->Args;
}
#endif

#if STATUS_LED_REPEAT
/**
 * @brief set repeat mode
 * 
 * @param led 
 * @param repeat 
 */
void StatusLed_setRepeat(StatusLed* led, StatusLed_RepeatMode repeat) {
    led->Repeat = repeat;
}
/**
 * @brief get repeat mode
 * 
 * @param led 
 * @return StatusLed_RepeatMode 
 */
StatusLed_RepeatMode StatusLed_getRepeat(StatusLed* led) {
    return led->Repeat;
}
#endif
