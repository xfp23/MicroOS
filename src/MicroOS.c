#include "MicroOS.h"
#include "stdlib.h"
#include "string.h"

static volatile MicroOS_Task_t MicroOS = {0}; // 任务对象

static MicroOS_Event_t OSEvent = {0}; // 事件对象

static MicroOS_OSdelay_t OSdelay = {0}; // delay对象

static void MicroOS_OSdelay_Init(void);

static void MicroOS_OSdelay_Tick(void);

static void MicroOS_OSEvent_Init(void);

static void MicroOS_DispatchAllEvents(void);

static MicroOS_Task_Handle_t const MicroOS_Task_Handle = &MicroOS;

MicroOS_Status_t MicroOS_Init()
{
    MICROOS_CHECK_PTR(MicroOS_Task_Handle);

    MicroOS_Task_Handle->MaxTasks = MICROOS_TASK_SIZE;
    MicroOS_Task_Handle->TaskNum = 0;
    MicroOS_Task_Handle->TickCount = 0;
    MicroOS_Task_Handle->CurrentTaskId = 0;
    MicroOS_OSdelay_Init();
    MicroOS_OSEvent_Init();
    return MICROOS_OK;
}

MicroOS_Status_t MicroOS_AddTask(uint8_t id, MicroOS_TaskFunction_t TaskFunction, void *Userdata, uint32_t Period)
{
    MICROOS_CHECK_PTR(MicroOS_Task_Handle);
    MICROOS_CHECK_ID(id);
    MICROOS_CHECK_PTR(TaskFunction);

    if (MicroOS_Task_Handle->TaskNum > MICROOS_TASK_SIZE)
        return MICROOS_ERROR;

    MicroOS_Task_Handle->TaskNum++;
    MicroOS_Task_Handle->Tasks[id].TaskFunction = TaskFunction;
    MicroOS_Task_Handle->Tasks[id].Userdata = Userdata;
    MicroOS_Task_Handle->Tasks[id].Period = Period;
    MicroOS_Task_Handle->Tasks[id].LastRunTime = 0;
    MicroOS_Task_Handle->Tasks[id].IsRunning = true;
    MicroOS_Task_Handle->Tasks[id].IsUsed = true;
    MicroOS_Task_Handle->Tasks[id].IsSleeping = false; // 不休眠

    return MICROOS_OK;
}

void MicroOS_StartScheduler(void)
{

    while (1)
    {
        MicroOS_DispatchAllEvents(); // 遍历事件槽
        // 遍历所有任务
        for (uint8_t i = 0; i < MICROOS_TASK_SIZE; i++)
        {
            if (!MicroOS_Task_Handle->Tasks[i].IsUsed)
                continue;
            if (!MicroOS_Task_Handle->Tasks[i].IsRunning)
                continue;
            uint32_t currentTime = MicroOS_Task_Handle->TickCount;

            if (MicroOS_Task_Handle->Tasks[i].IsSleeping && currentTime - MicroOS_Task_Handle->Tasks[i].LastRunTime >= MicroOS_Task_Handle->Tasks[i].SleepTicks)
            {
                MicroOS_Task_Handle->Tasks[i].IsSleeping = false;
                MicroOS_Task_Handle->Tasks[i].SleepTicks = 0;
            }
            if (MicroOS_Task_Handle->Tasks[i].IsSleeping)
                continue;
            if ((uint32_t)(currentTime - MicroOS_Task_Handle->Tasks[i].LastRunTime) >= MicroOS_Task_Handle->Tasks[i].Period)
            {
                MicroOS_Task_Handle->CurrentTaskId = i; // 当前任务ID
                MicroOS_Task_Handle->Tasks[i].TaskFunction(MicroOS_Task_Handle->Tasks[i].Userdata);
                MicroOS_Task_Handle->Tasks[i].LastRunTime = currentTime;
            }
        }
    }
}

MicroOS_Status_t MicroOS_TickHandler(void)
{
    MICROOS_CHECK_PTR(MicroOS_Task_Handle);

    MicroOS_Task_Handle->TickCount++;
    MicroOS_OSdelay_Tick();

    return MICROOS_OK;
}

MicroOS_Status_t MicroOS_SuspendTask(uint8_t id)
{
    MICROOS_CHECK_PTR(MicroOS_Task_Handle);
    MICROOS_CHECK_ID(id);
    if (!MicroOS_Task_Handle->Tasks[id].IsUsed)
    {
        return MICROOS_NOT_INITIALIZED;
    }

    MicroOS_Task_Handle->Tasks[id].IsRunning = false;
    return MICROOS_OK;
}

MicroOS_Status_t MicroOS_ResumeTask(uint8_t id)
{
    MICROOS_CHECK_PTR(MicroOS_Task_Handle);
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
    MICROOS_CHECK_PTR(MicroOS_Task_Handle);
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
    MICROOS_CHECK_PTR(MicroOS_Task_Handle);
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
    MICROOS_CHECK_PTR(MicroOS_Task_Handle);
    MICROOS_CHECK_ID(id);

    if (!MicroOS_Task_Handle->Tasks[id].IsUsed)
    {
        return MICROOS_NOT_INITIALIZED;
    }

    MicroOS_Task_Handle->Tasks[id].IsSleeping = false;
    MicroOS_Task_Handle->Tasks[id].SleepTicks = 0;

    return MICROOS_OK;
}

MicroOS_Status_t MicroOS_delay(uint32_t Ticks)
{
    MICROOS_CHECK_PTR(MicroOS_Task_Handle);

    if (Ticks == 0)
    {
        return MICROOS_INVALID_PARAM;
    }
    uint32_t startTick = MicroOS_Task_Handle->TickCount;
    while ((MicroOS_Task_Handle->TickCount - startTick) < Ticks)
    {
        // 等待直到指定的毫秒数过去
    }
    return MICROOS_OK;
}

// 初始化任务池
static void MicroOS_OSdelay_Init(void)
{
    // 初始化任务池
    for (int i = 0; i < OS_DELAY_POOLSIZE - 1; i++)
    {
        OSdelay.delay_pool[i].next = &OSdelay.delay_pool[i + 1];
    }
    // 最后一个节点指向 NULL
    OSdelay.delay_pool[OS_DELAY_POOLSIZE - 1].next = NULL;
    OSdelay.free_delay = &OSdelay.delay_pool[0]; // 空闲任务池
    OSdelay.active_delay = NULL;                 // 活动任务池
}

// 添加/更新任务
MicroOS_Status_t MicroOS_OSdelay(uint8_t id, uint32_t Ticks)
{
    MicroOS_OSdelay_Sub_t *p = OSdelay.active_delay;

    // 检查是否已有该 ID
    while (p)
    {
        if (p->id == id)
        {
            p->ms = Ticks;
            p->IsTimeout = false;
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
    node->ms = Ticks;
    node->IsTimeout = false;

    node->next = OSdelay.active_delay;
    OSdelay.active_delay = node;

    return MICROOS_OK;
}

// 查询状态
bool MicroOS_OSdelayDone(uint8_t id)
{
    MicroOS_OSdelay_Sub_t *p = OSdelay.active_delay;
    while (p)
    {
        if (p->id == id)
            return p->IsTimeout;
        p = p->next;
    }
    return false;
}

// Tick 处理
static void MicroOS_OSdelay_Tick(void)
{
    MicroOS_OSdelay_Sub_t *p = OSdelay.active_delay;
    while (p)
    {
        if (p->ms > 0)
        {
            p->ms--;
            if (p->ms == 0)
            {
                p->IsTimeout = true;
            }
        }
        p = p->next;
    }
}

void MicroOS_OSdelay_Remove(uint8_t id)
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

static void MicroOS_OSEvent_Init(void)
{
    // 初始化链表
    for (uint8_t i = 0; i < OS_EVENT_POOLSIZE - 1; i++)
    {
        OSEvent.EventPools[i].next = &OSEvent.EventPools[i + 1];
    }

    OSEvent.EventPools[OS_EVENT_POOLSIZE - 1].next = NULL;
    OSEvent.active_event = NULL;
    OSEvent.free_event = &OSEvent.EventPools[0]; // 空闲事件链表
}

MicroOS_Status_t MicroOS_RegisterEvent(uint8_t id, MicroOS_EventFunction_t EventFunction, void *Userdata)
{
    MICROOS_CHECK_PTR(EventFunction);
    MicroOS_Event_Sub_t *p = OSEvent.active_event;
    while (p)
    {
        if (p->id == id)
        {
            p->EventFunction = EventFunction;
            p->IsRunning = true;
            p->Userdata = Userdata;
            p->TriggerCount = 0;
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
    node->Userdata = Userdata;
    node->IsRunning = true;
    node->TriggerCount = 0;
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
            p->TriggerCount++;
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

void MicroOS_DispatchAllEvents(void)
{
    MicroOS_Event_Sub_t *p = OSEvent.active_event;

    while (p)
    {
        if (p->IsUsed && p->IsRunning && p->TriggerCount > 0)
        {
            OSEvent.CurrentEventId = p->id;
            p->EventFunction(p->Userdata);
            p->TriggerCount--;
        }
        p = p->next;
    }
}
