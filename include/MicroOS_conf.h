#ifndef MICROOS_CONF_H
#define MICROOS_CONF_H

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

#ifdef __cplusplus
extern "C"
{
#endif

// MicroOS version
#define MICROOS_VERSION_MAJOR "1.0.0"

// MICROOS FREQ
#define MICROOS_FREQ_HZ 1000

// MICROOS TASK
#define MICROOS_TASKENABLE (0) // enable microOS task

// MICROOS EVENT 
#define MICROOS_EVENTENABLE (1)

#if MICROOS_TASKENABLE
#define MICROOS_TASK_SIZE (10) // Maximum number of tasks supported
#define OS_DELAY_POOLSIZE (0) // Maximum number of delay tasks supported
#else
#define MICROOS_TASK_SIZE (0) // Maximum number of tasks supported
#define OS_DELAY_POOLSIZE (0) // Maximum number of delay tasks supported
#endif

#if MICROOS_EVENTENABLE
#define OS_EVENT_POOLSIZE (10) // Event pool size
#else
#define OS_EVENT_POOLSIZE (0) // Event pool size
#endif


#ifdef __cplusplus
}
#endif

#endif