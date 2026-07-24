#ifndef MICROOS_CONF_H
#define MICROOS_CONF_H

/**
 * @file    MicroOS_conf.h
 * @author  https://xfp23.github.io
 * @brief   MicroOS configuration file.
 * @version 2.0.1
 * @date    2025-08-31
 *
 * @details
 * Compile-time configuration for MicroOS.
 * All options are statically configured before compilation.
 */

#ifdef __cplusplus
extern "C"
{
#endif

/*==============================================================================
 * Version
 *============================================================================*/

/** MicroOS version */
#define MICROOS_VERSION_MAJOR                 "2.0.1"


/*==============================================================================
 * System Configuration
 *============================================================================*/

/** System tick frequency (Hz) */
#define MICROOS_FREQ_HZ                       1000U


/*==============================================================================
 * Task Module
 *============================================================================*/

/** Maximum number of scheduler tasks */
#define MICROOS_TASK_SIZE                     10U

/** Delay object pool size */
#define MICROOS_OSDELAY_POOL_SIZE               10U


/*==============================================================================
 * Event Module
 *============================================================================*/

/** Maximum number of registered events */
#define MICROOS_EVENT_POOL_SIZE               10U


/*==============================================================================
 * Message Event Module
 *============================================================================*/

/** Enable Message Event module (0: Disable, 1: Enable) */
#define MICROOS_MESSAGEEVENT_ENABLE           1U

/** Maximum number of Message Events */
#define MICROOS_MESSAGEEVENT_SIZE             5U


/*==============================================================================
 * Queue Module
 *============================================================================*/

/** Queue depth (number of messages) */
#define MICROOS_QUEUE_DEPTH                   5U

/** Maximum payload size of a single message (bytes) */
#define MICROOS_QUEUE_SINGLE_MSG_SIZE         8U


/*==============================================================================
 * Subscription Module
 *============================================================================*/

/** Enable Subscription module (0: Disable, 1: Enable) */
#define MICROOS_SUBSCRIPTION_ENABLE           1U

/** Maximum number of topics */
#define MICROOS_TOPIC_SIZE                    5U

/** Maximum number of subscribers per topic */
#define MICROOS_SUBSCRIBER_NUM                3U


#ifdef __cplusplus
}
#endif

#endif /* MICROOS_CONF_H */
