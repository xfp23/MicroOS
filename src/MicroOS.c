#include "MicroOS.h"
#include "stdlib.h"
#include "string.h"

static volatile MicroOS_t MicroOS = {0};

static MicroOS_Event_t OSEvent = {0};

static MicroOS_OSdelay_Task_t delay_pool[OS_DELAY_POOLSIZE] = {0};
static MicroOS_OSdelay_Task_t *free_list = NULL;   // 空闲节点链表
static MicroOS_OSdelay_Task_t *active_list = NULL; // 活动节点链表

static void MicroOS_OSdelay_Init(void);

static void MicroOS_OSdelay_Tick(void);

static void MicroOS_OSEvent_Init(void);

static void MicroOS_DispatchAllEvents(void);

MicroOS_Handle_t const MicroOS_handle = &MicroOS;

MicroOS_Status_t MicroOS_Init()
{
    MICROOS_CHECK_PTR(MicroOS_handle);

    MicroOS_handle->MaxTasks = MICROOS_TASK_SIZE;
    MicroOS_handle->TaskNum = 0;
    MicroOS_handle->TickCount = 0;
    MicroOS_handle->CurrentTaskId = 0;
    MicroOS_OSdelay_Init();
    MicroOS_OSEvent_Init();
    return MICROOS_OK;
}

MicroOS_Status_t MicroOS_AddTask(uint8_t id, MicroOS_TaskFunction_t TaskFunction, void *Userdata, uint32_t Period)
{
    MICROOS_CHECK_PTR(MicroOS_handle);
    MICROOS_CHECK_ID(id);
    MICROOS_CHECK_PTR(TaskFunction);

    if (MicroOS_handle->TaskNum > MICROOS_TASK_SIZE)
        return MICROOS_ERROR;

    MicroOS_handle->TaskNum++;
    MicroOS_handle->Tasks[id].TaskFunction = TaskFunction;
    MicroOS_handle->Tasks[id].Userdata = Userdata;
    MicroOS_handle->Tasks[id].Period = Period;
    MicroOS_handle->Tasks[id].LastRunTime = 0;
    MicroOS_handle->Tasks[id].IsRunning = true;
    MicroOS_handle->Tasks[id].IsUsed = true;
    MicroOS_handle->Tasks[id].IsSleeping = false; // 不休眠

    return MICROOS_OK;
}

void MicroOS_StartScheduler(void)
{

    while (1)
    {
        if (MicroOS_handle == NULL)
        {
            return;
        }

        MicroOS_DispatchAllEvents(); // 遍历事件槽
        // 遍历所有任务
        for (uint8_t i = 0; i < MICROOS_TASK_SIZE; i++)
        {
            if (!MicroOS_handle->Tasks[i].IsUsed)
                continue;
            if (!MicroOS_handle->Tasks[i].IsRunning)
                continue;
            uint32_t currentTime = MicroOS_handle->TickCount;

            if (MicroOS_handle->Tasks[i].IsSleeping && currentTime - MicroOS_handle->Tasks[i].LastRunTime >= MicroOS_handle->Tasks[i].SleepTicks)
            {
                MicroOS_handle->Tasks[i].IsSleeping = false;
                MicroOS_handle->Tasks[i].SleepTicks = 0;
            }
            if (MicroOS_handle->Tasks[i].IsSleeping)
                continue;
            if ((uint32_t)(currentTime - MicroOS_handle->Tasks[i].LastRunTime) >= MicroOS_handle->Tasks[i].Period)
            {
                MicroOS_handle->CurrentTaskId = i; // 当前任务ID
                MicroOS_handle->Tasks[i].TaskFunction(MicroOS_handle->Tasks[i].Userdata);
                MicroOS_handle->Tasks[i].LastRunTime = currentTime;
            }
        }
    }
}

MicroOS_Status_t MicroOS_TickHandler(void)
{
    MICROOS_CHECK_PTR(MicroOS_handle);

    MicroOS_handle->TickCount++;
    MicroOS_OSdelay_Tick();

    return MICROOS_OK;
}

MicroOS_Status_t MicroOS_SuspendTask(uint8_t id)
{
    MICROOS_CHECK_PTR(MicroOS_handle);
    MICROOS_CHECK_ID(id);
    if (!MicroOS_handle->Tasks[id].IsUsed)
    {
        return MICROOS_NOT_INITIALIZED;
    }

    MicroOS_handle->Tasks[id].IsRunning = false;
    return MICROOS_OK;
}

MicroOS_Status_t MicroOS_ResumeTask(uint8_t id)
{
    MICROOS_CHECK_PTR(MicroOS_handle);
    MICROOS_CHECK_ID(id);

    if (!MicroOS_handle->Tasks[id].IsUsed)
    {
        return MICROOS_NOT_INITIALIZED;
    }
    MicroOS_handle->Tasks[id].IsRunning = true;
    return MICROOS_OK;
}

MicroOS_Status_t MicroOS_DeleteTask(uint8_t id)
{
    MICROOS_CHECK_PTR(MicroOS_handle);
    MICROOS_CHECK_ID(id);

    if (MicroOS_handle->Tasks[id].IsRunning)
    {
        MicroOS_handle->Tasks[id].IsRunning = false;
        MicroOS_handle->TaskNum--;
    }

    memset((void *)&MicroOS_handle->Tasks[id], 0, sizeof(MicroOS_Task_t));

    return MICROOS_OK;
}

MicroOS_Status_t MicroOS_SleepTask(uint8_t id, uint32_t Ticks)
{
    MICROOS_CHECK_PTR(MicroOS_handle);
    MICROOS_CHECK_ID(id);

    if (Ticks == 0)
    {
        return MICROOS_INVALID_PARAM;
    }

    if (!MicroOS_handle->Tasks[id].IsUsed)
    {
        return MICROOS_NOT_INITIALIZED;
    }

    MicroOS_handle->Tasks[id].IsSleeping = true;
    MicroOS_handle->Tasks[id].SleepTicks = Ticks;
    MicroOS_handle->Tasks[id].LastRunTime = MicroOS_handle->TickCount;

    return MICROOS_OK;
}

MicroOS_Status_t MicroOS_WakeupTask(uint8_t id)
{
    MICROOS_CHECK_PTR(MicroOS_handle);
    MICROOS_CHECK_ID(id);

    if (!MicroOS_handle->Tasks[id].IsUsed)
    {
        return MICROOS_NOT_INITIALIZED;
    }

    MicroOS_handle->Tasks[id].IsSleeping = false;
    MicroOS_handle->Tasks[id].SleepTicks = 0;

    return MICROOS_OK;
}

MicroOS_Status_t MicroOS_delay(uint32_t Ticks)
{
    MICROOS_CHECK_PTR(MicroOS_handle);

    if (Ticks == 0)
    {
        return MICROOS_INVALID_PARAM;
    }
    uint32_t startTick = MicroOS_handle->TickCount;
    while ((MicroOS_handle->TickCount - startTick) < Ticks)
    {
        // 等待直到指定的毫秒数过去
    }
    return MICROOS_OK;
}

// 初始化任务池
void MicroOS_OSdelay_Init(void)
{
    // 初始化任务池
    for (int i = 0; i < OS_DELAY_POOLSIZE - 1; i++)
    {
        delay_pool[i].next = &delay_pool[i + 1];
    }
    // 最后一个节点指向 NULL
    delay_pool[OS_DELAY_POOLSIZE - 1].next = NULL;
    free_list = &delay_pool[0]; // 空闲任务池
    active_list = NULL;         // 活动任务池
}

// 添加/更新任务
MicroOS_Status_t MicroOS_OSdelay(uint8_t id, uint32_t Ticks)
{
    MicroOS_OSdelay_Task_t *p = active_list;

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

    // 没有则从 free_list 取节点
    if (!free_list)
        return MICROOS_BUSY; // 没空闲节点了

    MicroOS_OSdelay_Task_t *node = free_list;
    free_list = free_list->next;

    node->id = id;
    node->ms = Ticks;
    node->IsTimeout = false;

    node->next = active_list;
    active_list = node;

    return MICROOS_OK;
}

// 查询状态
bool MicroOS_OSdelayDone(uint8_t id)
{
    MicroOS_OSdelay_Task_t *p = active_list;
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
    MicroOS_OSdelay_Task_t *p = active_list;
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
    MicroOS_OSdelay_Task_t **pp = &active_list;
    while (*pp)
    {
        if ((*pp)->id == id)
        {
            MicroOS_OSdelay_Task_t *tmp = *pp;
            *pp = (*pp)->next;

            // 清零节点
            memset((void *)tmp, 0, sizeof(MicroOS_OSdelay_Task_t));

            // 放回 free_list
            tmp->next = free_list;
            free_list = tmp;
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


MicroOS_Status_t MicroOS_RegisterEvent(uint8_t id,MicroOS_EventFunction_t EventFunction,void *Userdata)
{
    MICROOS_CHECK_PTR(EventFunction);
    MicroOS_Event_Task_t *p = OSEvent.active_event;
    while (p)
    {
        if (p->id == id)
        {
            p->EventFunction = EventFunction;
            p->IsRunning = true;
            p->Userdata = Userdata;
            p->IsTriggered = false;
            p->IsUsed = true;
            return MICROOS_OK;
        }
        p = p->next;
    }

    if (!OSEvent.free_event)
        return MICROOS_BUSY; // 事件池满了

        OSEvent.EventNum++;
    MicroOS_Event_Task_t *node = OSEvent.free_event; // 保存当前要用的节点
    OSEvent.free_event = OSEvent.free_event->next;   // 将要用的节点从空闲节点中剔除

    node->id = id;
    node->EventFunction = EventFunction;
    node->Userdata = Userdata;
    node->IsRunning = true;
    node->IsTriggered = false;
    node->IsUsed = true;

    node->next = OSEvent.active_event;
    OSEvent.active_event = node;

    return MICROOS_OK;
}

void MicroOS_DeleteEvent(uint8_t id)
{
    MicroOS_Event_Task_t **pp = (MicroOS_Event_Task_t**)&OSEvent.active_event;

    while (*pp)
    {
        if ((*pp)->id == id)
        {
             OSEvent.EventNum--;
            MicroOS_Event_Task_t *tmp = *pp;
            *pp = tmp->next;

            memset(tmp, 0, sizeof(MicroOS_Event_Task_t));

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
    MicroOS_Event_Task_t *p = OSEvent.active_event;
    while (p)
    {
        if (p->id == id)
        {
            p->IsTriggered = true; 
            return MICROOS_OK;
        }
        p = p->next;
    }
    return MICROOS_ERROR;
}

MicroOS_Status_t MicroOS_SuspendEvent(uint8_t id)
{
    MicroOS_Event_Task_t *p = OSEvent.active_event;
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
    MicroOS_Event_Task_t *p = OSEvent.active_event;
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
    MicroOS_Event_Task_t *p = OSEvent.active_event;

    while (p)
    {
        if (p->IsRunning && p->IsTriggered && p->IsUsed)
        {
            OSEvent.CurrentEventId = p->id;
            p->EventFunction(p->Userdata);
            p->IsTriggered = false;
        }
        p = p->next;
    }
}
