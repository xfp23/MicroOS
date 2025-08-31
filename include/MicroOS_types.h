#ifndef MICROOS_TYPES_H
#define MICROOS_TYPES_H

/**
 * @file MicroOS_types.h
 * @author (https://github.com/xfp23)
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
} MicroOS_Status_t;

/**
 * @brief Structure representing a scheduled task
 */
typedef struct
{
    bool IsUsed;                  // Indicates if the task is currently in use
    bool IsRunning;               // Indicates if the task is currently running
    bool IsSleeping;              // Indicates if the task is currently sleeping
    uint32_t SleepTicks;          // Number of ticks the task is sleeping
    uint32_t Period;              // Task period in milliseconds
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
 * @brief Structure representing a delay task for OSdelay
 */
typedef struct MicroOS_OSdelay_Sub_t
{
    uint8_t id;                         /**< Delay task ID */
    volatile uint32_t ms;               /**< Delay time in milliseconds */
    volatile bool IsTimeout;            /**< Timeout status */
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
    bool IsRunning;                 // Whether to run
    bool IsUsed;                    // Whether to used
    volatile uint16_t TriggerCount; // Number of triggers
    void (*EventFunction)(void *data);
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
} MicroOS_Event_t;


#ifdef __cplusplus
}
#endif

#endif