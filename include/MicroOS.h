#ifndef MICROOS_H
#define MICROOS_H

/**
 * @file MicroOS.h
 * @author (https://github.com/xfp23)
 * @brief Lightweight system scheduler for embedded systems
 * @note This scheduler only supports single-instance operation; all tasks run in the same MicroOS instance.
 *       1. Each task has a unique ID, which can represent task priority. The smaller the ID, the higher the priority.
 *       2. The number of supported tasks is defined by MICROOS_TASK_SIZE (default: 10).
 *       3. No dynamic stack allocation for tasks; static allocation is used due to embedded resource constraints and heap fragmentation issues.
 *       4. To speed up the scheduler, increase the frequency of MicroOS_TickHandler calls in the system clock interrupt.
 *       5. To avoid heap fragmentation caused by OSdelay, a delay task pool is used. To expand the OSdelay task pool size, modify OS_DELAY_TASKSIZE.
 *       6. Set the frequency of MICROOS_FREQ_HZ according to the time base given to this library
 * @version 0.0.3
 * @date 2025-07-31
 * @copyright Copyright (c) 2025
 */
#include "stdint.h"
#include "stdbool.h"

#ifdef __cplusplus
extern "C"
{
#endif

// MicroOS version
#define MICROOS_VERSION_MAJOR "0.0.3"
	
// MICROOS FREQ
#define MICROOS_FREQ_HZ 1000

#define MICROOS_TASK_SIZE (10)  // Maximum number of tasks supported
#define OS_DELAY_TASKSIZE (32) // Maximum number of delay tasks supported

// Ticks -> MS
#define OS_TICKS_MS(tick)   ((tick) * (1000 / MICROOS_FREQ_HZ))

// MS -> Ticks
#define OS_MS_TICKS(ms)     ((ms) * (MICROOS_FREQ_HZ / 1000))


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
    if (id >= MICROOS_TASK_SIZE)       \
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
} MicroOS_Task_t;

/**
 * @brief MicroOS main instance structure
 */
typedef struct
{
    MicroOS_Task_t Tasks[MICROOS_TASK_SIZE]; /**< Array of scheduled tasks */
    uint32_t TickCount;                     /**< MicroOS tick counter */
    uint32_t MaxTasks;                      /**< Maximum number of tasks supported */
    uint8_t CurrentTaskId;                  /**< Current running task ID */
    uint8_t TaskNum;                        /**< Number of tasks added */
} MicroOS_t;

/**
 * @brief Structure representing a delay task for OSdelay
 */
typedef struct MicroOS_OSdelay_Task_t
{
    uint8_t id;                          /**< Delay task ID */
    uint32_t ms;                         /**< Delay time in milliseconds */
    bool IsTimeout;                      /**< Timeout status */
    struct MicroOS_OSdelay_Task_t *next; /**< Pointer to next delay task in pool */
} MicroOS_OSdelay_Task_t;

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
typedef volatile MicroOS_t *MicroOS_Handle_t;

/**
 * @brief MicroOS main instance handle
 * @note This is the only instance of MicroOS; all tasks run in this instance.
 */
extern MicroOS_Handle_t const MicroOS_handle;

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
extern MicroOS_Status_t MicroOS_SleepTask(uint8_t id,uint32_t Ticks);

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
