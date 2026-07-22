#ifndef MICROOS_H
#define MICROOS_H

/**
 * @file MicroOS.h
 * @author (https://xfp23.github.io)
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
 * @brief Initialize the MicroOS instance
 * @return MicroOS_Status_t Status code
 */
extern MicroOS_Status_t MicroOS_Init(void);

/**
 * @brief Registers a new event or updates an existing one.
 *
 * @param id            Unique event identifier.
 * @param name          Event name
 * @param EventFunction Callback function to be executed when the event is triggered.
 * @return MicroOS_Status_t Returns MICROOS_OK on success or an error code if the event pool is full.
 */
extern MicroOS_Status_t MicroOS_RegisterEvent(uint8_t id, char *name,MicroOS_EventFunction_t EventFunction);

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
 * @param Userdata      Pointer to user-defined data passed to the callback function.
 * @return MicroOS_Status_t Returns MICROOS_OK if the event was found and triggered, otherwise MICROOS_ERROR.
 */
extern MicroOS_Status_t MicroOS_TriggerEvent(uint8_t id,const void *Userdata);

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
 * @param id Task ID
 * @param OSdelayFunction  Delayed callback function
 * @param Userdata Data pointer provided by the user
 * @param Ticks Delay Ticks num  OS_MS_TICKS(ms)
 * @return MicroOS_Status_t Status code
 */
extern MicroOS_Status_t MicroOS_OSdelay(uint8_t id,MicroOS_OSdelayFunction_t OSdelayFunction,const void *Userdata, uint32_t Ticks);

/**
 * @brief Add a task to the scheduler
 * @param id Task ID (also represents priority; must be unique and less than MICROOS_TASK_SIZE)
 * @param Taskname Task name
 * @param TaskFunction Pointer to the task function
 * @param Userdata Pointer to user data
 * @param Period Task period in milliseconds
 * @return MicroOS_Status_t Status code
 */
extern MicroOS_Status_t MicroOS_AddTask(uint8_t id, char *Taskname,MicroOS_TaskFunction_t TaskFunction, void *Userdata, uint32_t Ticks);

/**
 * @brief Start the MicroOS scheduler and begin running tasks
 */
extern void MicroOS_StartScheduler(void);

/**
 * @brief Tick handler, usually called in the system clock interrupt
 * @note This function increases the TickCount counter, which is used for task scheduling. Typically called in a 1ms timer interrupt.
 */
extern void MicroOS_TickHandler(void);

/**
 * @brief Get MicroOS Tick count
 * 
 * @return uint32_t Tick count
 */
extern uint32_t MicroOS_GetTick();

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

#ifdef __cplusplus
}
#endif

#endif // !MICROOS_H
