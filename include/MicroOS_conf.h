#ifndef MICROOS_CONF_H
#define MICROOS_CONF_H

/**
 * @file MicroOS_conf.h
 * @author https://xfp23.github.io
 * @brief MicroOS configuration file
 * @version 1.2.0
 * @date 2025-08-31
 *
 * @details
 * Configure MicroOS compile-time options.
 * All parameters are statically configured before compilation.
 */

#ifdef __cplusplus
extern "C"
{
#endif

/*======================================================================
 * MicroOS Version
 *====================================================================*/

/** MicroOS version */
#define MICROOS_VERSION_MAJOR          "1.2.0"


/*======================================================================
 * System Configuration
 *====================================================================*/

/** System tick frequency (Hz) */
#define MICROOS_FREQ_HZ                1000U


/*======================================================================
 * Task Module Configuration
 *====================================================================*/

/** Maximum number of scheduler tasks */
#define MICROOS_TASK_SIZE              10U

/** OSdelay object pool size */
#define OS_DELAY_POOLSIZE              10U


/*======================================================================
 * Event Module Configuration
 *====================================================================*/

/** Maximum number of registered events */
#define OS_EVENT_POOLSIZE              10U


/*======================================================================
 * Message Event Module Configuration
 *====================================================================*/

/** Enable Message Event module (0: Disable, 1: Enable) */
#define MICROOS_MESSAGEEVENT_ENABLE    1U

/** Maximum number of Message Events */
#define MICROOS_MESSAGEEVENT_SIZE      10U


/*======================================================================
 * Queue Module Configuration
 *====================================================================*/

/** Queue capacity (number of messages) */
#define MICROOS_QUEUE_DEPTH            256U

/** Maximum payload size of a single message (bytes) */
#define MICROOS_QUEUE_MSG_SIZE         32U


#ifdef __cplusplus
}
#endif

#endif /* MICROOS_CONF_H */