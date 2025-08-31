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
#define MICROOS_VERSION_MAJOR "0.1.1"

// MICROOS FREQ
#define MICROOS_FREQ_HZ 1000

#define MICROOS_TASK_SIZE (10) // Maximum number of tasks supported
#define OS_DELAY_POOLSIZE (10) // Maximum number of delay tasks supported
#define OS_EVENT_POOLSIZE (10) // Event pool size

#ifdef __cplusplus
}
#endif

#endif