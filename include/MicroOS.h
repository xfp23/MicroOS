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

#include "MicroOS_types.h"
#include "MicroOS_com.h"
#include "MicroOS_conf.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief The MicroOS obj handler
 * 
 */
extern volatile MicroOS_Task_Handle_t const MicroOS_Task_Handle;

/**
 * @brief Initialize the MicroOS instance
 * @return MicroOS_Status_t Status code
 */
extern MicroOS_Status_t MicroOS_Init(void);

#if MICROOS_EVENTENABLE
/**
 * @brief Registers a new event or updates an existing one.
 *
 * @param id            Unique event identifier.
 * @param name          Event name
 * @param EventFunction Callback function to be executed when the event is triggered.
 * @param Userdata      Pointer to user-defined data passed to the callback function.
 * @return MicroOS_Status_t Returns MICROOS_OK on success or an error code if the event pool is full.
 */
extern MicroOS_Status_t MicroOS_RegisterEvent(uint8_t id, char *name,MicroOS_EventFunction_t EventFunction, void *Userdata);

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
#endif

#if MICROOS_TASKENABLE
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
 * @brief Add a task to the scheduler
 * @param id Task ID (also represents priority; must be unique and less than MICROOS_TASK_SIZE)
 * @param Taskname Task name
 * @param TaskFunction Pointer to the task function
 * @param Userdata Pointer to user data
 * @param Period Task period in milliseconds
 * @return MicroOS_Status_t Status code
 */
extern MicroOS_Status_t MicroOS_AddTask(uint8_t id, char *Taskname,MicroOS_TaskFunction_t TaskFunction, void *Userdata, uint32_t Period);

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
 * @brief Rest the task record Ticks
 *
 * @param id task id
 * @return MicroOS_Status_t Status code
 */
extern MicroOS_Status_t MicroOS_ResetTask(uint8_t id);

/**
 * @brief Delete the task with the specified ID
 * @param id Task ID
 * @return MicroOS_Status_t Status code
 */
extern MicroOS_Status_t MicroOS_DeleteTask(uint8_t id);
#endif

#ifdef __cplusplus
}
#endif

#endif // !MICROOS_H
