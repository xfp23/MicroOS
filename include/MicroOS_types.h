#ifndef MICROOS_TYPES_H
#define MICROOS_TYPES_H

/**
 * @file MicroOS_types.h
 * @author (https://xfp23.github.io)
 * @brief Define types
 * @version 0.1
 * @date 2025-08-31
 *
 * @copyright Copyright (c) 2025
 *
 */
#include "stdint.h"
#include "stdbool.h"
#include "MicroOS_conf.h"
#include "MicroOSQueue_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Task function prototype
 * @param Userdata Pointer to user data
 */
typedef void (*MicroOS_TaskFunction_t)(void *Userdata);

/**
 * @brief Event function prototype
 * @param Userdata Pointer to user data
 */
typedef void (*MicroOS_EventFunction_t)(void *Userdata);

/**
 * @brief OSdelay function prototype
 * @param Userdata Pointer to user data
 */
typedef void (*MicroOS_OSdelayFunction_t)(void *Userdata);

/**
 * @brief EventQueue function prototype
 * @param Userdata Queue message
 * 
 */
typedef void (*MicroOSQueue_EventFunction_t)(const MicroOSQueue_Message_t* QueueMsg);

typedef void (*MicroOS_SubscriberFunction_t)(void *Userdata);
/**
 * @brief MicroOS status codes
 */
typedef enum
{
    MICROOS_OK = 0,          /**< Operation successful */
    MICROOS_ERROR,           /**< General error */
    MICROOS_TIMEOUT,         /**< Timeout occurred */
    MICROOS_INVALID_PARAM,   /**< Invalid parameter */
    MICROOS_NOT_INITIALIZED, /**< MicroOS not initialized */
    MICROOS_BUSY,            /**< MicroOS is busy */
    MICROOS_QUEUE_FULL,
    MICROOS_QUEUE_EMPTY,
} MicroOS_Status_t;

/**
 * @brief Structure representing a scheduled task
 */
typedef struct
{
    bool IsUsed;                  // Indicates if the task is currently in use
    bool IsRunning;               // Indicates if the task is currently running
    bool IsSleeping;              // Indicates if the task is currently sleeping
    char *name;                   // Task name
    uint32_t SleepTicks;          // Number of ticks the task is sleeping
    uint32_t Tick;                // Task period in milliseconds
    uint32_t LastRunTime;         // Last run time in ticks
    void (*TaskFunction)(void *); // Pointer to the task function
    void *Userdata;               // Pointer to user data
} MicroOS_Task_Sub_t;

/**
 * @brief MicroOS main instance structure
 */
typedef struct
{
    MicroOS_Task_Sub_t Tasks[MICROOS_TASK_SIZE]; /**< Array of scheduled tasks */
    uint32_t TickCount;                          /**< MicroOS tick counter */
    uint32_t MaxTasks;                           /**< Maximum number of tasks supported */
    uint8_t CurrentTaskId;                       /**< Current running task ID */
    uint8_t TaskNum;                             /**< Number of tasks added */
} MicroOS_Task_t;

/**
 * @brief MicroOS handle type (pointer to main instance)
 */
typedef volatile MicroOS_Task_t *MicroOS_Task_Handle_t;

/**
 * @brief Structure representing a delay task for OSdelay
 */
typedef struct MicroOS_OSdelay_Sub_t
{
    uint8_t id;              /**< Delay task ID */
    volatile uint32_t tick;  /**< Delay time in milliseconds */
    volatile bool IsTimeout; /**< Timeout status */
    void (*OSdelayFunction)(void *);
    void *Userdata;
    struct MicroOS_OSdelay_Sub_t *next; /**< Pointer to next delay task in pool */
} MicroOS_OSdelay_Sub_t;

typedef struct
{
    MicroOS_OSdelay_Sub_t delay_pool[OS_DELAY_POOLSIZE];
    MicroOS_OSdelay_Sub_t *free_delay;
    MicroOS_OSdelay_Sub_t *active_delay;
    uint8_t OSdelayNum;

} MicroOS_OSdelay_t;

typedef struct MicroOS_Event_Sub_t
{
    uint8_t id;                     // event unique id
    char *name;                     // event name
    bool IsRunning;                 // Whether to run
    bool IsUsed;                    // Whether to used
    volatile bool Triggered;     // riggers
    void (*EventFunction)(void* data);
    void *Userdata;
    struct MicroOS_Event_Sub_t *next; // next node
} MicroOS_Event_Sub_t;

typedef struct
{
    MicroOS_Event_Sub_t EventPools[OS_EVENT_POOLSIZE]; // event pool
    MicroOS_Event_Sub_t *free_event;                   // idle events
    MicroOS_Event_Sub_t *active_event;                 // active events
    uint8_t CurrentEventId;                            // Current event ID
    uint8_t EventNum;                                  // number of surviving events
    // MicroOSQueue_Obj_t Event_queue;                     // Event queue
} MicroOS_Event_t;

typedef struct {
    // (O1)查找,数组索引就是ID，因为消息需要memecpy就已经很重了，如果再加个O(n),会浪费cpu
    bool IsUsed;                  // Indicates if the task is currently in use
    bool IsRunning;               // Indicates if the task is currently running
    char *name;                   // Task name
    void (*MessageEventFunction)(const MicroOSQueue_Message_t *);
    MicroOSQueue_Message_t Userdata;               // Pointer to user data
    MicroOSQueue_Obj_t queue;
}MicroOS_MessageEvent_Sub_t;

typedef struct
{
    MicroOS_MessageEvent_Sub_t Event[MICROOS_MESSAGEEVENT_SIZE]; /**< Array of scheduled Message */
    uint32_t MaxMessage;                           /**< Maximum number of Message supported */
    uint8_t CurrentMessageEventId;               /**< Current running Message ID */
    uint8_t MessageNum;                             /**< Number of Message added */
} MicroOS_MessageEvent_t;

typedef struct
{
    char *name;
    bool IsUsed;
    bool IsRunning;
    MicroOS_SubscriberFunction_t callback;
} MicroOS_Subscriber_t; // 订阅者
 
typedef struct
{
    bool IsUsed;
    bool IsRunning;
    volatile bool IsPending;
 
    char *name;              
    volatile void *Userdata;
    MicroOS_Subscriber_t subscribers[MICROOS_SUBSCRIBER_NUM];
} MicroOS_Topic_t; // 主题
 
typedef struct
{
    // O(1) 查找：没有独立 ID，数组下标本身就是 ID
    MicroOS_Topic_t topics[MICROOS_TOPIC_SIZE];
    uint8_t TopicCount;                          
} MicroOS_PubSub_t; // 发布订阅管理对象

#ifdef __cplusplus
}
#endif

#endif 
