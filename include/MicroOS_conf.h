#ifndef MICROOS_CONF_H
#define MICROOS_CONF_H

/**
 * @file microos_conf.h
 * @author xfp23
 * @brief MicroOS configuration header file
 * @version 1.0.0
 * @date 2025-08-31
 * 
 * Description:
 * - Configure MicroOS version, frequency, task, and event features
 * - Enable or disable task/event modules via macros
 */

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------- MicroOS Version -------------------- */
#define MICROOS_VERSION_MAJOR    "1.1.0"    ///< MicroOS version

/* -------------------- MicroOS System Frequency -------------------- */
#define MICROOS_FREQ_HZ          1000        ///< System clock frequency (Hz)

/* -------------------- MicroOS Feature Enable -------------------- */
#define MICROOS_TASKENABLE       1           ///< Enable task module (1: enable, 0: disable)
#define MICROOS_EVENTENABLE      1           ///< Enable event module (1: enable, 0: disable)

/* -------------------- Task Module Configuration -------------------- */
#if MICROOS_TASKENABLE
    #define MICROOS_TASK_SIZE    10        ///< Maximum number of tasks
    #define OS_DELAY_POOLSIZE    10        ///< Delay task pool size
#else
    #define MICROOS_TASK_SIZE    0
    #define OS_DELAY_POOLSIZE    0
#endif

/* -------------------- Event Module Configuration -------------------- */
#if MICROOS_EVENTENABLE
    #define OS_EVENT_POOLSIZE    10        ///< Event pool size
#else
    #define OS_EVENT_POOLSIZE    0
#endif

#ifdef __cplusplus
}
#endif

#endif // MICROOS_CONF_H
