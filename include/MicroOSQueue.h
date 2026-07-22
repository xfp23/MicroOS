#ifndef MicroOSQueue_H
#define MicroOSQueue_H

#include "MicroOSQueue_types.h"
#include "MicroOS_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Init MicroOSQueue
 * 
 * @param obj 
 * @return MicroOS_Status_t 
 */
MicroOS_Status_t MicroOSQueue_Init(MicroOSQueue_Obj_t *obj);

/**
 * @brief Push queue
 * 
 * @param obj a queue object
 * @param data User push data
 * @param size data len
 * @return MicroOS_Status_t 
 */
MicroOS_Status_t MicroOSQueue_Push(MicroOSQueue_Obj_t *obj,const void *data,uint16_t size);

/**
 * @brief Pop queue
 * 
 * @param obj a queue object
 * @param data Copy buffer
 * @param size Copy len buffer
 * @return MicroOS_Status_t 
 */
MicroOS_Status_t MicroOSQueue_Pop(MicroOSQueue_Obj_t *obj,void *data,uint16_t *size);

/**
 * @brief the queue object is empty
 * 
 * @param obj a queue object
 * @return true is empty
 * @return false not empty
 */
bool MicroOSQueue_IsEmpty(MicroOSQueue_Obj_t *obj);

/**
 * @brief the queue object is full
 * 
 * @param obj a queue object
 * @return true is full
 * @return false not full
 */
bool MicroOSQueue_IsFull(MicroOSQueue_Obj_t *obj);

/**
 * @brief Reset the queue
 * 
 * @param obj a queue object
 * @return MicroOS_Status_t 
 */
MicroOS_Status_t MicroOSQueue_Reset(MicroOSQueue_Obj_t *obj);


#ifdef __cplusplus
}
#endif

#endif
