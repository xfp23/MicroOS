#include "MicroOSQueue.h"
#include "MicroOS_com.h"
#include "string.h"


MicroOS_Status_t MicroOSQueue_Init(MicroOSQueue_Obj_t *obj)
{
    MICROOS_CHECK_PTR(obj);

    memset(obj,0,sizeof(MicroOSQueue_Obj_t));

    return MICROOS_OK;
}

MicroOS_Status_t MicroOSQueue_Reset(MicroOSQueue_Obj_t *obj)
{
    MICROOS_CHECK_PTR(obj);

    memset(obj,0,sizeof(MicroOSQueue_Obj_t));

    return MICROOS_OK;
}

bool MicroOSQueue_IsEmpty(MicroOSQueue_Obj_t *obj)
{
    return (obj->head == obj->tail);
}



bool MicroOSQueue_IsFull(MicroOSQueue_Obj_t *obj)
{
    return ((obj->tail - obj->head) >= MICROOS_QUEUE_DEPTH);
}



MicroOS_Status_t MicroOSQueue_Push(MicroOSQueue_Obj_t *obj,const void *data,size_t size)
{

    MICROOS_CHECK_PTR(obj);
    MICROOS_CHECK_PTR(data);


    if(size > MICROOS_QUEUE_SINGLE_MSG_SIZE)
    {
        return MICROOS_ERROR;
    }


    if(MicroOSQueue_IsFull(obj))
    {
        return MICROOS_QUEUE_FULL;
    }


    uint32_t index = obj->tail % MICROOS_QUEUE_DEPTH;


    obj->buffer[index].len = size;


    memcpy(obj->buffer[index].data,data,size);


    obj->tail++;


    return MICROOS_OK;
}



MicroOS_Status_t MicroOSQueue_Pop(MicroOSQueue_Obj_t *obj,void *data,size_t *size)
{

    MICROOS_CHECK_PTR(obj);
    MICROOS_CHECK_PTR(data);
    MICROOS_CHECK_PTR(size);


    if(MicroOSQueue_IsEmpty(obj))
    {
        return MICROOS_QUEUE_EMPTY;
    }


    uint32_t index = obj->head % MICROOS_QUEUE_DEPTH;


    *size = obj->buffer[index].len;


    memcpy(data,obj->buffer[index].data,*size);


    obj->head++;


    return MICROOS_OK;
}
