#ifndef MICROOS_COM_H
#define MICROOS_COM_H

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

#ifdef __cplusplus
}
#endif

#endif