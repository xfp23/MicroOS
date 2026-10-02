#ifndef MICROOS_UTILS_H
#define MICROOS_UTILS_H

/**
 * @file MicroOS_utils.h
 * @author (https://xfp23.github.io)
 * @brief Define Utils Macros
 * @version \ref MICROOS_VERSION_MAJOR
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

#define MICROOS_CHECK_OSTIMER_ID(id)      \
    do                                    \
    {                                     \
        if (id >= MICROOS_OSTIMER_SIZE)   \
        {                                 \
            return MICROOS_INVALID_PARAM; \
        }                                 \
    } while (0)

#define MICROOS_CHECK_OSTIMER_VALID(id)   \
    do                                    \
    {                                     \
        if (!OSTimer.timer[id].is_valid) \
        {                                 \
            return MICROOS_BUSY;          \
        }                                 \
    } while(0)

#ifdef __cplusplus
}
#endif

#endif
