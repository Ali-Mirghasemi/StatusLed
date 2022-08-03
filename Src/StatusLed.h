/**
 * @file StatusLed.h
 * @author Ali Mirghasemi (ali.mirghasemi@gmail.com)
 * @brief This library help you to show system status over a led
 * @version 0.1
 * @date 2022-08-01
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#ifndef _STATUS_LED_H_
#define _STATUS_LED_H_


#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/******************************************************************************/
/*                                Configuration                               */
/******************************************************************************/

/**
 * @brief enable configure led active state
 */
#define STATUS_LED_ACTIVE_STATE             1
/**
 * @brief enable callback at end of led pattern
 */
#define STATUS_LED_CALLBACK                 1
/**
 * @brief enable deinit function in driver
 */
#define STATUS_LED_DEINIT                   0
/**
 * @brief enable StatusLed_IO in pin config 
 */
#define STATUS_LED_CONFIG_IO                1
/**
 * @brief hold status led GPIO
 */
typedef void* StatusLed_IO;
/**
 * @brief hold status led pin
 */
typedef uint16_t StatusLed_Pin;
/**
 * @brief number of leds, -1 means unlimited, it'll use linked list
 */
#define STATUS_LED_MAX_NUM                  -1
/**
 * @brief enable status led user arguments
 */
#define STATUS_LED_ARGS                     1
/**
 * @brief enable control led without change pattern
 */
#define STATUS_LED_ENABLE_FLAG              1 
/**
 * @brief system timestamp in milliseconds
 */
typedef uint32_t StatusLed_Timestamp;
/**
 * @brief status led cycle time in milliseconds
 */
typedef uint16_t StatusLed_CycleTime;
/**
 * @brief status led len type
 */
typedef uint8_t StatusLed_LenType;

/******************************************************************************/

#define STATUS_LED_NULL                     ((StatusLed*) 0)
#define STATUS_LED_PATTERN_NULL             ((StatusLed_Pattern*) 0)
#define STATUS_LED_CONFIG_NULL              ((StatusLed_PinConfig*) 0)

/**
 * @brief hold pin configuration that use for handle led
 */
typedef struct {
#if STATUS_LED_CONFIG_IO
    StatusLed_IO     	    IO;
#endif
    StatusLed_Pin           Pin;
} StatusLed_PinConfig;
/**
 * @brief hold led state
 */
typedef enum {
    StatusLed_LedState_On       = 0,
    StatusLed_LedState_Off      = 1,
} StatusLed_LedState;
/**
 * @brief hold pin state
 */
typedef enum {
    StatusLed_PinState_Low      = 0,
    StatusLed_PinState_High     = 1,
} StatusLed_PinState;
/**
 * @brief result of functions
 */
typedef enum {
    StatusLed_Status_Ok         = 0,
    StatusLed_Status_NoSpace    = 1,
    StatusLed_Status_Null       = 2,
    StatusLed_Status_NotFound   = 3,
} StatusLed_Status;
/**
 * @brief logic of led
 * Active Low for Open-Drain leds
 * Active High for Push-Pull leds
 * remember if you have leds with different logic must enable STATUS_LED_ACTIVE_STATE
 */
typedef enum {
    StatusLed_ActiveState_Low     = 0,
    StatusLed_ActiveState_High    = 1,
} StatusLed_ActiveState;
/**
 * @brief status led repeat mode
 */
typedef enum {
    StatusLed_Repeat_Off     = 0,
    StatusLed_Repeat_On      = 1,
} StatusLed_RepeatMode;
/* predefined data types */
struct __StatusLed;
typedef struct __StatusLed StatusLed;

/**
 * @brief initialize pin in output mode, remember if your pin is push-pull, or open-drain 
 * must configured in init function
 * this function call when new led add to queue
 */
typedef void (*StatusLed_InitPinFn)(const StatusLed_PinConfig* config);
/**
 * @brief de-initialize pin and change pin to reset mode
 * this function call on remove led
 */
typedef void (*StatusLed_DeInitPinFn)(const StatusLed_PinConfig* config);
/**
 * @brief this function write output value on pin
 * 0 -> LOW, 1 -> HIGH
 */
typedef void (*StatusLed_WritePinFn)(const StatusLed_PinConfig* config, StatusLed_PinState state);
/**
 * @brief this function return system timestamp in milliseconds
 */
typedef StatusLed_Timestamp (*StatusLed_GetTimestampFn)(void); 
/**
 * @brief this callback call when led finish pattern
 */
typedef void (*StatusLed_CallbackFn)(StatusLed* led);
/**
 * @brief hold a single On-Off times
 */
typedef union {
    StatusLed_CycleTime         Times;
    struct {
        StatusLed_CycleTime     Off;
        StatusLed_CycleTime     On;
    };
} StatusLed_Cycle;
/**
 * @brief hold a led pattern
 */
typedef struct {
    const StatusLed_Cycle*      Cycles;
    StatusLed_LenType           Len;
} StatusLed_Pattern;
/**
 * @brief hold minimum function for StatusLed lib to work
 * user must pass atleast init and write functions to status led library
 */
typedef struct {
    StatusLed_GetTimestampFn    getTimestamp;
    StatusLed_InitPinFn         initPin;
    StatusLed_WritePinFn        writePin;
    #if STATUS_LED_DEINIT
        StatusLed_DeInitPinFn   deinitPin;
    #endif
} StatusLed_Driver;
/**
 * @brief hold status led parameters
 */
typedef struct __StatusLed {
#if STATUS_LED_MAX_NUM == -1
    struct __StatusLed*             Previous;
#endif
#if STATUS_LED_ARGS
    void*                           Args;             /**< user arguments */
#endif
#if STATUS_LED_CALLBACK
    StatusLed_CallbackFn            onFinish;        /**< callback function */
#endif
    const StatusLed_PinConfig*      Config;          /**< pin configuration */
    const StatusLed_Pattern*        Pattern;         /**< pattern */
    StatusLed_LenType               PatternIndex;    /**< pattern length */
    StatusLed_Timestamp             NextBlink;       /**< next blink time */
    uint8_t                         State       : 1; /**< state */
    uint8_t                         ActiveState : 1; /**< active state */
    uint8_t                         Repeat      : 1; /**< repeat mode */
    uint8_t                         Enabled     : 1; /**< enabled led */
    uint8_t                         Configured  : 1; /**< configured */
    uint8_t                         Reserved    : 3; /**< reserved */
} StatusLed;


void StatusLed_init(StatusLed_Driver* driver);
StatusLed_Timestamp StatusLed_handle(void);

void StatusLed_reset(StatusLed* led);
void StatusLed_setPattern(StatusLed* led, const StatusLed_Pattern* pattern);
const StatusLed_Pattern* StatusLed_getPattern(StatusLed* led);

void StatusLed_setConfig(StatusLed* led, const StatusLed_PinConfig* config);
const StatusLed_PinConfig* StatusLed_getConfig(StatusLed* led);

#if STATUS_LED_MAX_NUM > 0
    StatusLed* StatusLed_new(void);
#endif // STATUS_LED_MAX_NUM

StatusLed_Status StatusLed_add(StatusLed* led, const StatusLed_PinConfig* config);
StatusLed_Status StatusLed_remove(StatusLed* remove);
StatusLed*       StatusLed_find(const StatusLed_PinConfig* config);

#if STATUS_LED_CALLBACK
    void StatusLed_onFinish(StatusLed* led, StatusLed_CallbackFn fn);
#endif // STATUS_LED_CALLBACK

#if STATUS_LED_ACTIVE_STATE
    void StatusLed_setActiveState(StatusLed* led, StatusLed_ActiveState state);
    StatusLed_ActiveState StatusLed_getActiveState(StatusLed* led);
#endif

#if STATUS_LED_ENABLE_FLAG
    void StatusLed_setEnabled(StatusLed* led, uint8_t enabled);
    uint8_t StatusLed_isEnabled(StatusLed* led);
#endif

#if STATUS_LED_ARGS
    void StatusLed_setArgs(StatusLed* led, void* args);
    void* StatusLed_getArgs(StatusLed* led);
#endif

#ifdef __cplusplus
};
#endif

#endif // _STATUS_LED_H_
