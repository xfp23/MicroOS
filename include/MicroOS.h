#ifndef MICROOS_H
#define MICROOS_H

/**
 * @file MicroOS.h
 * @author (https://github.com/xfp23)
 * @brief Lightweight cooperative scheduler and event manager for embedded systems.
 *
 * @note
 *   - Single-instance design: all tasks and events run in the same MicroOS instance.
 *   - Each task has a unique ID, which can also represent task priority (lower ID = higher priority).
 *   - The maximum number of tasks is defined by MICROOS_TASK_SIZE (default: 10).
 *   - Static allocation only; no dynamic stack allocation to avoid heap fragmentation in resource-limited MCUs.
 *   - Event system uses a fixed pool defined by OS_EVENT_POOLSIZE.
 *   - Tick-driven scheduler: call MicroOS_TickHandler() from a periodic hardware timer ISR.
 *   - To speed up scheduling accuracy, ensure MICROOS_FREQ_HZ matches the hardware tick frequency.
 *   - OSdelay uses a static delay task pool; modify OS_DELAY_POOLSIZE to adjust pool size.
 *
 * @version 0.1.1
 * @date 2025-08-03
 * @copyright Copyright (c) 2025
 */

#include "stdint.h"
#include "stdbool.h"

#ifdef __cplusplus
extern "C"
{
#endif

// MicroOS version
#define MICROOS_VERSION_MAJOR "0.1.1"

// MICROOS FREQ
#define MICROOS_FREQ_HZ 1000

#define MICROOS_TASK_SIZE (10) // Maximum number of tasks supported
#define OS_DELAY_POOLSIZE (10) // Maximum number of delay tasks supported
#define OS_EVENT_POOLSIZE (10) // Event pool size

// Ticks -> MS
#define OS_TICKS_MS(tick) ((tick) * (1000 / MICROOS_FREQ_HZ))

// MS -> Ticks
#define OS_MS_TICKS(ms) ((ms) * (MICROOS_FREQ_HZ / 1000))

// Null pointer check macro
#define MICROOS_CHECK_PTR(ptr)    \
do                            \
{                             \
    if ((ptr) == NULL)        \
    {                         \
        return MICROOS_ERROR; \
    }                         \
} while (0)

// Error check macro
#define MIROOS_CHECK_ERR(err)       \
do                              \
{                               \
    MicroOS_Status_t ret = err; \
    if (ret != MICROOS_OK)      \
    {                           \
        return ret;             \
    }                           \
} while (0)

// Task ID check macro
#define MICROOS_CHECK_ID(id)              \
do                                    \
{                                     \
    if (id >= MICROOS_TASK_SIZE)      \
    {                                 \
        return MICROOS_INVALID_PARAM; \
    }                                 \
} while (0)

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

/**
 * @brief Registers a new event or updates an existing one.
 *
 * @param id            Unique event identifier.
 * @param EventFunction Callback function to be executed when the event is triggered.
 * @param Userdata      Pointer to user-defined data passed to the callback function.
 * @return MicroOS_Status_t Returns MICROOS_OK on success or an error code if the event pool is full.
 */
extern MicroOS_Status_t MicroOS_RegisterEvent(uint8_t id, MicroOS_EventFunction_t EventFunction, void *Userdata);

/**
 * @brief Deletes an event from the active event list.
 *
 * @param id Unique event identifier to be deleted.
 */
extern void MicroOS_DeleteEvent(uint8_t id);

/**
 * @brief Triggers an event, marking it to be executed in the scheduler loop.
 *
 * @param id Unique event identifier to trigger.
 * @return MicroOS_Status_t Returns MICROOS_OK if the event was found and triggered, otherwise MICROOS_ERROR.
 */
extern MicroOS_Status_t MicroOS_TriggerEvent(uint8_t id);

/**
 * @brief Suspends an event, preventing it from being executed even if triggered.
 *
 * @param id Unique event identifier to suspend.
 * @return MicroOS_Status_t Returns MICROOS_OK if the event was found and suspended, otherwise MICROOS_ERROR.
 */
extern MicroOS_Status_t MicroOS_SuspendEvent(uint8_t id);

/**
 * @brief Resumes a previously suspended event, allowing it to execute when triggered.
 *
 * @param id Unique event identifier to resume.
 * @return MicroOS_Status_t Returns MICROOS_OK if the event was found and resumed, otherwise MICROOS_ERROR.
 */
extern MicroOS_Status_t MicroOS_ResumeEvent(uint8_t id);

/**
 * @brief blocking delay
 *
 * @param Ticks Ticks Delay Ticks num  OS_MS_TICKS(ms)
 * @return MicroOS_Status_t  Status code
 */
extern MicroOS_Status_t MicroOS_delay(uint32_t Ticks);

/**
 * @brief Set a delay for a task (OSdelay)
 * @details To use OSdelay accurately, you must create a 1ms periodic task and call MicroOS_OSdelayDone within it.
 *          After the delay is no longer needed, you must call MicroOS_OSdelay_Remove to release the delay task.
 *          See project usage examples for details.
 * @param id Task ID
 * @param Ticks Delay Ticks num  OS_MS_TICKS(ms)
 * @return MicroOS_Status_t Status code
 */
extern MicroOS_Status_t MicroOS_OSdelay(uint8_t id, uint32_t Ticks);

/**
 * @brief Get the delay status of a task
 * @details This function should be called inside a 1ms periodic task to check if the delay has expired.
 *          Returns true if the delay has expired and the task can proceed, false otherwise.
 * @param id Task ID
 * @return true if delay has expired and task can run, false if still in delay period
 */
extern bool MicroOS_OSdelayDone(uint8_t id);

/**
 * @brief Remove the delay task with the specified ID
 * @details You must call this function to release the delay task after it is no longer needed, to avoid resource leaks.
 * @param id Task ID
 */
extern void MicroOS_OSdelay_Remove(uint8_t id);

/**
 * @brief MicroOS handle type (pointer to main instance)
 */
typedef volatile MicroOS_Task_t *MicroOS_Task_Handle_t;

/**
 * @brief Initialize the MicroOS instance
 * @return MicroOS_Status_t Status code
 */
extern MicroOS_Status_t MicroOS_Init(void);

/**
 * @brief Add a task to the scheduler
 * @param id Task ID (also represents priority; must be unique and less than MICROOS_TASK_SIZE)
 * @param TaskFunction Pointer to the task function
 * @param Userdata Pointer to user data
 * @param Period Task period in milliseconds
 * @return MicroOS_Status_t Status code
 */
extern MicroOS_Status_t MicroOS_AddTask(uint8_t id, MicroOS_TaskFunction_t TaskFunction, void *Userdata, uint32_t Period);

/**
 * @brief Start the MicroOS scheduler and begin running tasks
 */
extern void MicroOS_StartScheduler(void);

/**
 * @brief Tick handler, usually called in the system clock interrupt
 * @note This function increases the TickCount counter, which is used for task scheduling. Typically called in a 1ms timer interrupt.
 * @return MicroOS_Status_t Status code
 */
extern MicroOS_Status_t MicroOS_TickHandler(void);

/**
 * @brief Suspend the task with the specified ID
 * @param id Task ID
 * @return MicroOS_Status_t Status code
 */
extern MicroOS_Status_t MicroOS_SuspendTask(uint8_t id);

/**
 * @brief Resume the task with the specified ID
 * @param id Task ID
 * @return MicroOS_Status_t Status code
 */
extern MicroOS_Status_t MicroOS_ResumeTask(uint8_t id);

/**
 * @brief
 * Task sleep, set the task not to run within the specified time
 *
 * @param id
 * @param Ticks Sleep Ticks OS_MS_TICKS(ms)
 * @return MicroOS_Status_t Status code
 */
extern MicroOS_Status_t MicroOS_SleepTask(uint8_t id, uint32_t Ticks);

/**
 * @brief Wake up the task in advance, used in conjunction with sleep
 *
 * @param id task id
 * @return MicroOS_Status_t Status code
 */
extern MicroOS_Status_t MicroOS_WakeupTask(uint8_t id);

/**
 * @brief Delete the task with the specified ID
 * @param id Task ID
 * @return MicroOS_Status_t Status code
 */
extern MicroOS_Status_t MicroOS_DeleteTask(uint8_t id);

#ifdef __cplusplus
}
#endif

#endif // !MICROOS_H
