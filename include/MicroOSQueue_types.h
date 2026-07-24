/**
 * @file MicroOSQueue_types.h
 * @author https://xfp23.github.io
 * @brief queue types
 * @version 0.1
 * @date 2026-07-22
 *
 * @copyright Copyright (c) 2026
 *
 */
#ifndef MicroOSQueue_TYPES_H
#define MicroOSQueue_TYPES_H

#include "MicroOS_conf.h"
#include "stdint.h"
#include "string.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Statically allocated, copy-based byte ring buffer.
 *
 * @note Each stored item is framed with a length header (see
 *       MicroOSQueue.h / MicroOSQueue_LEN_BYTES) so that variable-length
 *       items can share the same raw buffer without losing their
 *       boundaries.
 */

typedef struct 
{
    size_t len;
    uint8_t data[MICROOS_QUEUE_SINGLE_MSG_SIZE];
} MicroOSQueue_Message_t;


typedef struct
{
    MicroOSQueue_Message_t buffer[MICROOS_QUEUE_DEPTH];

    volatile uint32_t tail;
    volatile uint32_t head;

} MicroOSQueue_Obj_t;

#ifdef __cplusplus
}
#endif

#endif
