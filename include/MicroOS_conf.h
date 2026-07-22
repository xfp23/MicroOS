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
extern "C"
{
#endif

/* -------------------- MicroOS Version -------------------- */
#define MICROOS_VERSION_MAJOR "1.2.0" ///< MicroOS version

/* -------------------- MicroOS System Frequency -------------------- */
#define MICROOS_FREQ_HZ 1000 ///< System clock frequency (Hz)

/* -------------------- Task Module Configuration -------------------- */
#define MICROOS_TASK_SIZE 10 ///< Maximum number of tasks
#define OS_DELAY_POOLSIZE 10 ///< Delay task pool size

/* -------------------- Event Module Configuration -------------------- */
#define OS_EVENT_POOLSIZE 10 ///< Event pool size

#ifdef __cplusplus
}
#endif

#endif // MICROOS_CONF_H
