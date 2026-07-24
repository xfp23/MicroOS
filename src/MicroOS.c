/**
 * @file MicroOS.c
 * @author https://xfp23.github.io
 * @brief MicroOS Source file
 * @version 2.0.1
 * @date 2026-07-24
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#include "MicroOS.h"
#include "stdlib.h"
#include "string.h"

static MicroOS_Task_t MicroOS = {0}; // 任务对象

static MicroOS_Task_Handle_t const MicroOS_Task_Handle = &MicroOS;

static MicroOS_OSdelay_t OSdelay = {0}; // delay对象

static void MicroOS_OSdelay_Init(void);

static void MicroOS_OSdelay_Tick(void);

static MicroOS_Event_t OSEvent = {0}; // 事件对象

static void MicroOS_OSEvent_Init(void);

static void MicroOS_DispatchAllEvents(void);

static void MicroOS_OSdelay_Remove(uint8_t id);

static void MicroOS_OSdelay_StartScheduler(void);

#if MICROOS_MESSAGEEVENT_ENABLE

static MicroOS_MessageEvent_t OSMessageEvent = {0};

static void MicroOS_MessageEvent_Init(void);

static void MicroOS_MessageEventDispatch(void);
#endif

#if MICROOS_SUBSCRIPTION_ENABLE

static MicroOS_PubSub_t OSPubSub = {0};

static void MicroOS_PubSub_Init(void);

static void MicroOS_TopicDispatch(void);

#endif

MicroOS_Status_t MicroOS_Init()
{

    MicroOS_Task_Handle->MaxTasks = MICROOS_TASK_SIZE;
    MicroOS_Task_Handle->TaskNum = 0;
    MicroOS_Task_Handle->TickCount = 0;
    MicroOS_Task_Handle->CurrentTaskId = 0;
    MicroOS_OSdelay_Init();
    MicroOS_OSEvent_Init();
    MicroOS_PubSub_Init();
#if MICROOS_MESSAGEEVENT_ENABLE
    MicroOS_MessageEvent_Init();
#endif
    return MICROOS_OK;
}

void MicroOS_StartScheduler(void)
{

    while (1)
    {

        MicroOS_DispatchAllEvents();

        MicroOS_OSdelay_StartScheduler();

#if MICROOS_MESSAGEEVENT_ENABLE
        MicroOS_MessageEventDispatch();
#endif

#if MICROOS_SUBSCRIPTION_ENABLE
        MicroOS_TopicDispatch();
#endif

        for (uint8_t i = 0; i < MICROOS_TASK_SIZE; i++)
        {
            volatile MicroOS_Task_Sub_t *t = &MicroOS_Task_Handle->Tasks[i];

            if (!t->IsUsed || !t->IsRunning)
                continue;

            uint32_t currentTime = MicroOS_Task_Handle->TickCount;

            if (t->IsSleeping && (currentTime - t->LastRunTime) >= t->SleepTicks)
            {
                t->IsSleeping = false;
                t->SleepTicks = 0;
            }
            if (t->IsSleeping)
                continue;

            if ((uint32_t)(currentTime - t->LastRunTime) >= t->Tick)
            {
                MicroOS_Task_Handle->CurrentTaskId = i;
                t->TaskFunction(t->Userdata);
                t->LastRunTime = currentTime;
            }
        }
    }
}

void MicroOS_TickHandler(void)
{

    MicroOS_Task_Handle->TickCount++;
    MicroOS_OSdelay_Tick();
}

uint32_t MicroOS_GetTick(void)
{
    return MicroOS_Task_Handle->TickCount;
}

MicroOS_Status_t MicroOS_delay(uint32_t Ticks)
{

    if (Ticks == 0)
    {
        return MICROOS_INVALID_PARAM;
    }
    uint32_t startTick = MicroOS_Task_Handle->TickCount;
    while ((MicroOS_Task_Handle->TickCount - startTick) < Ticks)
    {
    }
    return MICROOS_OK;
}

MicroOS_Status_t MicroOS_AddTask(uint8_t id, char *Taskname, MicroOS_TaskFunction_t TaskFunction, void *Userdata, uint32_t Tick)
{
    MICROOS_CHECK_ID(id);
    MICROOS_CHECK_PTR(TaskFunction);

    if (MicroOS_Task_Handle->TaskNum >= MICROOS_TASK_SIZE)
        return MICROOS_ERROR;

    if (!MicroOS_Task_Handle->Tasks[id].IsUsed)
    {
        MicroOS_Task_Handle->TaskNum++;
    }
    MicroOS_Task_Handle->Tasks[id].name = Taskname;
    MicroOS_Task_Handle->Tasks[id].TaskFunction = TaskFunction;
    MicroOS_Task_Handle->Tasks[id].Userdata = Userdata;
    MicroOS_Task_Handle->Tasks[id].Tick = Tick;
    MicroOS_Task_Handle->Tasks[id].LastRunTime = 0;
    MicroOS_Task_Handle->Tasks[id].IsRunning = true;
    MicroOS_Task_Handle->Tasks[id].IsUsed = true;
    MicroOS_Task_Handle->Tasks[id].IsSleeping = false;

    return MICROOS_OK;
}

MicroOS_Status_t MicroOS_SuspendTask(uint8_t id)
{
    MICROOS_CHECK_ID(id);
    if (!MicroOS_Task_Handle->Tasks[id].IsUsed)
    {
        return MICROOS_NOT_INITIALIZED;
    }

    MicroOS_Task_Handle->Tasks[id].IsRunning = false;
    return MICROOS_OK;
}

MicroOS_Status_t MicroOS_ResetTask(uint8_t id)
{
    MICROOS_CHECK_ID(id);
    if (!MicroOS_Task_Handle->Tasks[id].IsUsed)
    {
        return MICROOS_NOT_INITIALIZED;
    }
    uint32_t nowTime = MicroOS_Task_Handle->TickCount;

    MicroOS_Task_Handle->Tasks[id].LastRunTime = nowTime;
    MicroOS_Task_Handle->Tasks[id].IsRunning = 0;
    MicroOS_Task_Handle->Tasks[id].SleepTicks = 0;
    return MICROOS_OK;
}

MicroOS_Status_t MicroOS_ResumeTask(uint8_t id)
{
    MICROOS_CHECK_ID(id);

    if (!MicroOS_Task_Handle->Tasks[id].IsUsed)
    {
        return MICROOS_NOT_INITIALIZED;
    }
    MicroOS_Task_Handle->Tasks[id].IsRunning = true;
    return MICROOS_OK;
}

MicroOS_Status_t MicroOS_DeleteTask(uint8_t id)
{
    MICROOS_CHECK_ID(id);

    if (MicroOS_Task_Handle->Tasks[id].IsRunning)
    {
        MicroOS_Task_Handle->Tasks[id].IsRunning = false;
        MicroOS_Task_Handle->TaskNum--;
    }

    memset((void *)&MicroOS_Task_Handle->Tasks[id], 0, sizeof(MicroOS_Task_Sub_t));

    return MICROOS_OK;
}

MicroOS_Status_t MicroOS_SleepTask(uint8_t id, uint32_t Ticks)
{
    MICROOS_CHECK_ID(id);

    if (Ticks == 0)
    {
        return MICROOS_INVALID_PARAM;
    }

    if (!MicroOS_Task_Handle->Tasks[id].IsUsed)
    {
        return MICROOS_NOT_INITIALIZED;
    }

    MicroOS_Task_Handle->Tasks[id].IsSleeping = true;
    MicroOS_Task_Handle->Tasks[id].SleepTicks = Ticks;
    MicroOS_Task_Handle->Tasks[id].LastRunTime = MicroOS_Task_Handle->TickCount;

    return MICROOS_OK;
}

MicroOS_Status_t MicroOS_WakeupTask(uint8_t id)
{
    MICROOS_CHECK_ID(id);

    if (!MicroOS_Task_Handle->Tasks[id].IsUsed)
    {
        return MICROOS_NOT_INITIALIZED;
    }

    MicroOS_Task_Handle->Tasks[id].IsSleeping = false;
    MicroOS_Task_Handle->Tasks[id].SleepTicks = 0;

    return MICROOS_OK;
}

// 初始化任务池
static void MicroOS_OSdelay_Init(void)
{
    // 初始化任务池
    for (unsigned int i = 0; i < MICROOS_OSDELAY_POOL_SIZE - 1; i++)
    {
        OSdelay.delay_pool[i].next = &OSdelay.delay_pool[i + 1];
    }
    // 最后一个节点指向 NULL
    OSdelay.delay_pool[MICROOS_OSDELAY_POOL_SIZE - 1].next = NULL;
    OSdelay.free_delay = &OSdelay.delay_pool[0]; // 空闲任务池
    OSdelay.active_delay = NULL;                 // 活动任务池
}

// 添加/更新任务
MicroOS_Status_t MicroOS_OSdelay(uint8_t id, MicroOS_OSdelayFunction_t OSdelayFunction, const void *Userdata, uint32_t Ticks)
{
    MICROOS_CHECK_PTR(OSdelayFunction);

    MicroOS_OSdelay_Sub_t *p = OSdelay.active_delay;

    // 检查是否已有该 ID
    while (p)
    {
        if (p->id == id)
        {
            p->tick = Ticks;
            p->IsTimeout = false;
            p->OSdelayFunction = OSdelayFunction;
            p->Userdata = (void *)Userdata;
            return MICROOS_OK;
            ;
        }
        p = p->next;
    }

    // 没有则从 OSdelay.free_delay 取节点
    if (!OSdelay.free_delay)
        return MICROOS_BUSY; // 没空闲节点了

    MicroOS_OSdelay_Sub_t *node = OSdelay.free_delay;
    OSdelay.free_delay = OSdelay.free_delay->next;

    node->id = id;
    node->tick = Ticks;
    node->IsTimeout = false;
    node->OSdelayFunction = OSdelayFunction;
    node->Userdata = (void *)Userdata;

    node->next = OSdelay.active_delay;
    OSdelay.active_delay = node;

    return MICROOS_OK;
}

// Tick 处理
static void MicroOS_OSdelay_Tick(void)
{
    MicroOS_OSdelay_Sub_t *p = OSdelay.active_delay;
    while (p)
    {
        if (p->tick > 0)
        {
            p->tick--;
            if (p->tick == 0)
            {
                p->IsTimeout = true;
            }
        }
        p = p->next;
    }
}

static void MicroOS_OSdelay_Remove(uint8_t id)
{
    MicroOS_OSdelay_Sub_t **pp = &OSdelay.active_delay;
    while (*pp)
    {
        if ((*pp)->id == id)
        {
            MicroOS_OSdelay_Sub_t *tmp = *pp;
            *pp = (*pp)->next;

            // 清零节点
            memset((void *)tmp, 0, sizeof(MicroOS_OSdelay_Sub_t));

            // 放回 OSdelay.free_delay
            tmp->next = OSdelay.free_delay;
            OSdelay.free_delay = tmp;
            return;
        }
        else
        {
            pp = &(*pp)->next;
        }
    }
}

static void MicroOS_OSdelay_StartScheduler(void)
{
    MicroOS_OSdelay_Sub_t *p = OSdelay.active_delay;

    while (p)
    {
        if (p->IsTimeout)
        {
            p->OSdelayFunction(p->Userdata);
            p->IsTimeout = false;
            p->Userdata = NULL;
            MicroOS_OSdelay_Remove(p->id);
        }

        p = p->next;
    }
}

static void MicroOS_OSEvent_Init(void)
{
    // 初始化链表
    for (uint8_t i = 0; i < MICROOS_EVENT_POOL_SIZE - 1; i++)
    {
        OSEvent.EventPools[i].next = &OSEvent.EventPools[i + 1];
    }

    OSEvent.EventPools[MICROOS_EVENT_POOL_SIZE - 1].next = NULL;
    OSEvent.active_event = NULL;
    OSEvent.free_event = &OSEvent.EventPools[0]; // 空闲事件链表
}

MicroOS_Status_t MicroOS_RegisterEvent(uint8_t id, char *name, MicroOS_EventFunction_t EventFunction, const void *Userdata)
{
    MICROOS_CHECK_PTR(EventFunction);
    MicroOS_Event_Sub_t *p = OSEvent.active_event;
    while (p)
    {
        if (p->id == id)
        {
            p->name = name;
            p->EventFunction = EventFunction;
            p->IsRunning = true;
            p->Userdata = (void *)Userdata;
            p->Triggered = false;
            p->IsUsed = true;
            return MICROOS_OK;
        }
        p = p->next;
    }

    if (!OSEvent.free_event)
        return MICROOS_BUSY; // 事件池满了

    OSEvent.EventNum++;
    MicroOS_Event_Sub_t *node = OSEvent.free_event; // 保存当前要用的节点
    OSEvent.free_event = OSEvent.free_event->next;  // 将要用的节点从空闲节点中剔除

    node->id = id;
    node->EventFunction = EventFunction;
    node->Userdata = (void *)Userdata;
    node->IsRunning = true;
    node->Triggered = false;
    node->IsUsed = true;

    node->next = OSEvent.active_event;
    OSEvent.active_event = node;

    return MICROOS_OK;
}

void MicroOS_DeleteEvent(uint8_t id)
{
    MicroOS_Event_Sub_t **pp = (MicroOS_Event_Sub_t **)&OSEvent.active_event;

    while (*pp)
    {
        if ((*pp)->id == id)
        {
            OSEvent.EventNum--;
            MicroOS_Event_Sub_t *tmp = *pp;
            *pp = tmp->next;

            memset(tmp, 0, sizeof(MicroOS_Event_Sub_t));

            tmp->next = OSEvent.free_event;
            OSEvent.free_event = tmp;

            return;
        }
        else
        {
            pp = &(*pp)->next;
        }
    }
}

MicroOS_Status_t MicroOS_TriggerEvent(uint8_t id)
{
    MicroOS_Event_Sub_t *p = OSEvent.active_event;
    while (p)
    {
        if (p->id == id && p->IsUsed && p->IsRunning)
        {
            p->Triggered = true;
            return MICROOS_OK;
        }
        p = p->next;
    }
    return MICROOS_ERROR;
}

MicroOS_Status_t MicroOS_SuspendEvent(uint8_t id)
{
    MicroOS_Event_Sub_t *p = OSEvent.active_event;
    while (p)
    {
        if (p->id == id)
        {
            p->IsRunning = false;
            return MICROOS_OK;
        }
        p = p->next;
    }
    return MICROOS_ERROR;
}

MicroOS_Status_t MicroOS_ResumeEvent(uint8_t id)
{
    MicroOS_Event_Sub_t *p = OSEvent.active_event;
    while (p)
    {
        if (p->id == id)
        {
            p->IsRunning = true;
            return MICROOS_OK;
        }
        p = p->next;
    }
    return MICROOS_ERROR;
}

static void MicroOS_DispatchAllEvents(void)
{
    MicroOS_Event_Sub_t *p = OSEvent.active_event;

    while (p)
    {
        if (p->IsUsed && p->IsRunning && p->Triggered == true)
        {
            OSEvent.CurrentEventId = p->id;
            p->EventFunction(p->Userdata);
            p->Triggered = false;
        }
        p = p->next;
    }
}

#if MICROOS_MESSAGEEVENT_ENABLE
static void MicroOS_MessageEvent_Init()
{
    OSMessageEvent.CurrentMessageEventId = 0;
    OSMessageEvent.MaxMessage = MICROOS_MESSAGEEVENT_SIZE;
    OSMessageEvent.MessageNum = 0;
}

MicroOS_Status_t MicroOS_RegisterMessageEvent(uint8_t id, const char *name, MicroOSQueue_EventFunction_t function)
{
    if (id >= MICROOS_MESSAGEEVENT_SIZE)
    {
        return MICROOS_ERROR;
    }

    if (OSMessageEvent.MessageNum >= MICROOS_MESSAGEEVENT_SIZE)
    {
        return MICROOS_ERROR;
    }

    MICROOS_CHECK_PTR(function);

    if (OSMessageEvent.Event[id].IsUsed)
    {
        return MICROOS_BUSY;
    }

    OSMessageEvent.MessageNum++;
    OSMessageEvent.Event[id].MessageEventFunction = function;
    OSMessageEvent.Event[id].name = (char *)name;
    OSMessageEvent.Event[id].IsUsed = true;
    OSMessageEvent.Event[id].IsRunning = true;
    // OSMessageEvent.Event[id].TriggerCount = 0;
    memset(&OSMessageEvent.Event[id].Userdata, 0, sizeof(MicroOSQueue_Message_t));
    MicroOSQueue_Init(&OSMessageEvent.Event[id].queue);

    return MICROOS_OK;
}

MicroOS_Status_t MicroOS_DeleteMessageEvent(uint8_t id)
{
    if (id >= MICROOS_MESSAGEEVENT_SIZE)
    {
        return MICROOS_ERROR;
    }

    OSMessageEvent.Event[id].IsUsed = false;
    OSMessageEvent.Event[id].IsRunning = false;
    OSMessageEvent.Event[id].name = NULL;
    memset((void *)&OSMessageEvent.Event[id].Userdata, 0, sizeof(MicroOSQueue_Message_t));
    MicroOSQueue_Reset(&OSMessageEvent.Event[id].queue);

    return MICROOS_OK;
}

MicroOS_Status_t MicroOS_TriggerMessageEvent(uint8_t id, const void *data, size_t data_len)
{
    if (id >= MICROOS_MESSAGEEVENT_SIZE)
    {
        return MICROOS_ERROR;
    }

    if (OSMessageEvent.Event[id].IsUsed && OSMessageEvent.Event[id].IsRunning)
    {

        MicroOS_Status_t ret = MicroOSQueue_Push(&OSMessageEvent.Event[id].queue, data, data_len);

        if (ret != MICROOS_OK)
        {
            return ret;
        }
    }

    return MICROOS_OK;
}

MicroOS_Status_t MicroOS_SuspendMessageEvent(uint8_t id)
{
    if (id >= MICROOS_MESSAGEEVENT_SIZE)
    {
        return MICROOS_ERROR;
    }

    OSMessageEvent.Event[id].IsRunning = false;

    return MICROOS_OK;
}

MicroOS_Status_t MicroOS_ResumeMessageEvent(uint8_t id)
{
    if (id >= MICROOS_MESSAGEEVENT_SIZE)
    {
        return MICROOS_ERROR;
    }

    OSMessageEvent.Event[id].IsRunning = true;

    return MICROOS_OK;
}

static void MicroOS_MessageEventDispatch(void)
{
    for (uint8_t i = 0; i < MICROOS_MESSAGEEVENT_SIZE; i++)
    {
        MicroOS_MessageEvent_Sub_t *evt = &OSMessageEvent.Event[i];

        if (!(evt->IsUsed && evt->IsRunning))
        {
            continue;
        }

        if (!evt->MessageEventFunction)
        {
            continue;
        }

        if (MicroOSQueue_IsEmpty(&evt->queue))
        {
            continue;
        }

        if (MicroOSQueue_Pop(&evt->queue, evt->Userdata.data, &evt->Userdata.len) == MICROOS_OK)
        {
            OSMessageEvent.CurrentMessageEventId = i;
            evt->MessageEventFunction(&evt->Userdata);
        }
    }
}
#endif

#if MICROOS_SUBSCRIPTION_ENABLE

static void MicroOS_PubSub_Init(void)
{
    memset(&OSPubSub, 0, sizeof(MicroOS_PubSub_t));
}
 
MicroOS_Status_t MicroOS_CreateTopic(uint8_t id, const char *topic)
{
    if (id >= MICROOS_TOPIC_SIZE)
    {
        return MICROOS_INVALID_PARAM;
    }
 
    if (OSPubSub.topics[id].IsUsed)
    {
        return MICROOS_BUSY;
    }
 
    if (OSPubSub.TopicCount >= MICROOS_TOPIC_SIZE)
    {
        return MICROOS_BUSY;
    }
 
    OSPubSub.TopicCount++;
    OSPubSub.topics[id].IsUsed = true;
    OSPubSub.topics[id].name = (char *)topic;
    memset(OSPubSub.topics[id].subscribers, 0, sizeof(OSPubSub.topics[id].subscribers));
    OSPubSub.topics[id].Userdata = NULL;
    OSPubSub.topics[id].IsRunning = true;
    OSPubSub.topics[id].IsPending = false;
 
    return MICROOS_OK;
}
 
MicroOS_Status_t MicroOS_DeleteTopic(uint8_t id)
{
    if (id >= MICROOS_TOPIC_SIZE)
    {
        return MICROOS_INVALID_PARAM;
    }
 
    if (OSPubSub.topics[id].IsUsed)
    {
        OSPubSub.TopicCount--;
        OSPubSub.topics[id].IsUsed = false;
        OSPubSub.topics[id].name = NULL;
        memset(OSPubSub.topics[id].subscribers, 0, sizeof(OSPubSub.topics[id].subscribers));
        OSPubSub.topics[id].Userdata = NULL;
        OSPubSub.topics[id].IsRunning = false;
        OSPubSub.topics[id].IsPending = false;
    }
 
    return MICROOS_OK;
}
 
// 订阅一个主题
MicroOS_Status_t MicroOS_Subscribe(uint8_t topic_id, uint8_t sub_id, const char *name, MicroOS_SubscriberFunction_t func)
{
    if (topic_id >= MICROOS_TOPIC_SIZE || sub_id >= MICROOS_SUBSCRIBER_NUM)
    {
        return MICROOS_INVALID_PARAM;
    }
 
    if (!OSPubSub.topics[topic_id].IsUsed)
    {
        return MICROOS_ERROR;
    }
 
    if (OSPubSub.topics[topic_id].subscribers[sub_id].IsUsed)
    {
        return MICROOS_BUSY;
    }
 
    OSPubSub.topics[topic_id].subscribers[sub_id].IsUsed = true;
    OSPubSub.topics[topic_id].subscribers[sub_id].IsRunning = true;
    OSPubSub.topics[topic_id].subscribers[sub_id].name = (char *)name;
    OSPubSub.topics[topic_id].subscribers[sub_id].callback = func;
 
    return MICROOS_OK;
}
 
MicroOS_Status_t MicroOS_Unsubscribe(uint8_t topic_id, uint8_t sub_id)
{
    if (topic_id >= MICROOS_TOPIC_SIZE || sub_id >= MICROOS_SUBSCRIBER_NUM)
    {
        return MICROOS_INVALID_PARAM;
    }
 
    if (!OSPubSub.topics[topic_id].IsUsed)
    {
        return MICROOS_ERROR;
    }
 
    if (!OSPubSub.topics[topic_id].subscribers[sub_id].IsUsed)
    {
        return MICROOS_ERROR;
    }
 
    memset(&OSPubSub.topics[topic_id].subscribers[sub_id], 0, sizeof(MicroOS_Subscriber_t));
 
    return MICROOS_OK;
}
 
MicroOS_Status_t MicroOS_Publish(uint8_t topic_id, const void *Userdata)
{
    if (topic_id >= MICROOS_TOPIC_SIZE)
    {
        return MICROOS_INVALID_PARAM;
    }
 
    if (!OSPubSub.topics[topic_id].IsUsed)
    {
        return MICROOS_ERROR;
    }
 
    if (!OSPubSub.topics[topic_id].IsRunning)
    {
        return MICROOS_BUSY;
    }
 
    OSPubSub.topics[topic_id].IsPending = true;
    OSPubSub.topics[topic_id].Userdata = (void *)Userdata;
 
    return MICROOS_OK;
}
 
MicroOS_Status_t MicroOS_SuspendSubscription(uint8_t topic_id, uint8_t sub_id)
{
    if (topic_id >= MICROOS_TOPIC_SIZE || sub_id >= MICROOS_SUBSCRIBER_NUM)
    {
        return MICROOS_INVALID_PARAM;
    }
 
    if (!OSPubSub.topics[topic_id].IsUsed)
    {
        return MICROOS_ERROR;
    }
 
    OSPubSub.topics[topic_id].subscribers[sub_id].IsRunning = false;
 
    return MICROOS_OK;
}
 
MicroOS_Status_t MicroOS_ResumeSubscription(uint8_t topic_id, uint8_t sub_id)
{
    if (topic_id >= MICROOS_TOPIC_SIZE || sub_id >= MICROOS_SUBSCRIBER_NUM)
    {
        return MICROOS_INVALID_PARAM;
    }
 
    if (!OSPubSub.topics[topic_id].IsUsed)
    {
        return MICROOS_ERROR;
    }
 
    OSPubSub.topics[topic_id].subscribers[sub_id].IsRunning = true;
 
    return MICROOS_OK;
}
 
MicroOS_Status_t MicroOS_ClearSubscriptions(uint8_t topic_id)
{
    if (topic_id >= MICROOS_TOPIC_SIZE)
    {
        return MICROOS_INVALID_PARAM;
    }
 
    if (!OSPubSub.topics[topic_id].IsUsed)
    {
        return MICROOS_ERROR;
    }
 
    memset(OSPubSub.topics[topic_id].subscribers, 0, sizeof(OSPubSub.topics[topic_id].subscribers));
 
    return MICROOS_OK;
}
 
uint8_t MicroOS_SubscriberCount(uint8_t topic_id) 
{
    if (topic_id >= MICROOS_TOPIC_SIZE)
    {
        return 0;
    }
 
    if (!OSPubSub.topics[topic_id].IsUsed)
    {
        return 0;
    }
 
    uint8_t count = 0;
    for (uint8_t i = 0; i < MICROOS_SUBSCRIBER_NUM; i++)
    {
        if (OSPubSub.topics[topic_id].subscribers[i].IsUsed)
        {
            count++;
        }
    }
 
    return count;
}

bool MicroOS_IsTopicSuspended(uint8_t topic_id)
{
    if (topic_id >= MICROOS_TOPIC_SIZE)
    {
        return false;
    }
 
    return OSPubSub.topics[topic_id].IsRunning == false;
}
 
bool MicroOS_IsSubscriptionSuspended(uint8_t topic_id, uint8_t sub_id)
{
    if (topic_id >= MICROOS_TOPIC_SIZE || sub_id >= MICROOS_SUBSCRIBER_NUM)
    {
        return false;
    }
 
    return OSPubSub.topics[topic_id].subscribers[sub_id].IsRunning == false;
}

static void MicroOS_TopicDispatch(void)
{
    for(unsigned int i = 0; i < MICROOS_TOPIC_SIZE; i++)
    {
        if(!OSPubSub.topics[i].IsUsed || !OSPubSub.topics[i].IsRunning)
        {
            continue;
        }

        if(!OSPubSub.topics[i].IsPending)
        {
            continue;
        }

        for(unsigned int j = 0; j < MICROOS_SUBSCRIBER_NUM; j++)
        {
            if(!OSPubSub.topics[i].subscribers[j].IsRunning || !OSPubSub.topics[i].subscribers[j].IsUsed)
            {
                continue;
            }

            if(OSPubSub.topics[i].subscribers[j].callback)
            {
                OSPubSub.topics[i].subscribers[j].callback((void*)OSPubSub.topics[i].Userdata);
            }
        }

        OSPubSub.topics[i].IsPending = false;
    }
}
#endif
