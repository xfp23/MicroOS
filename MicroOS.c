#include "MicroOS.h"
#include "stdlib.h"

static volatile MicroOS_t MicroOS = {0};

static MicroOS_OSdelay_Task_t delay_pool[OS_DELAY_TASKSIZE] = {0};
static MicroOS_OSdelay_Task_t *free_list = NULL;   // 空闲节点链表
static MicroOS_OSdelay_Task_t *active_list = NULL; // 活动节点链表

static void MicroOS_OSdelay_Init(void);

static void MicroOS_OSdelay_Tick(void);

MicroOS_Handle_t MicroOS_handle = &MicroOS;

MicroOS_Status_t MicroOS_Init()
{
    MICROOS_CHECK_PTR(MicroOS_handle);

    MicroOS_handle->MaxTasks = MICROOS_TASK_NUM;
    MicroOS_handle->TaskNum = 0;
    MicroOS_handle->TickCount = 0;
    MicroOS_handle->CurrentTaskId = 0;
    MicroOS_OSdelay_Init();
    return MICROOS_OK;
}

MicroOS_Status_t MicroOS_AddTask(uint8_t id, MicroOS_TaskFunction_t TaskFunction, void *Userdata, uint32_t Period)
{
    MICROOS_CHECK_PTR(MicroOS_handle);
    MICROOS_CHECK_ID(id);
    MICROOS_CHECK_PTR(TaskFunction);

    if (MicroOS_handle->TaskNum > MICROOS_TASK_NUM)
        return MICROOS_ERROR;

    MicroOS_handle->TaskNum++;
    MicroOS_handle->Tasks[id].TaskFunction = TaskFunction;
    MicroOS_handle->Tasks[id].Userdata = Userdata;
    MicroOS_handle->Tasks[id].Period = Period;
    MicroOS_handle->Tasks[id].LastRunTime = 0;
    MicroOS_handle->Tasks[id].IsRunning = true;
    MicroOS_handle->Tasks[id].IsUsed = true;

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
        // 遍历所有任务
        for (uint8_t i = 0; i < MICROOS_TASK_NUM; i++)
        {
            if (!MicroOS_handle->Tasks[i].IsUsed)
                continue;
            if (!MicroOS_handle->Tasks[i].IsRunning)
                continue;
            uint32_t currentTime = MicroOS_handle->TickCount;
            if ((currentTime - MicroOS_handle->Tasks[i].LastRunTime) >= MicroOS_handle->Tasks[i].Period)
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

    MicroOS_handle->Tasks[id].IsRunning = false;
    return MICROOS_OK;
}

MicroOS_Status_t MicroOS_ResumeTask(uint8_t id)
{
    MICROOS_CHECK_PTR(MicroOS_handle);
    MICROOS_CHECK_ID(id);

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

    // 清除任务数据
    MicroOS_handle->Tasks[id].TaskFunction = NULL;
    MicroOS_handle->Tasks[id].Userdata = NULL;
    MicroOS_handle->Tasks[id].Period = 0;
    MicroOS_handle->Tasks[id].LastRunTime = 0;
    MicroOS_handle->Tasks[id].IsUsed = false;

    return MICROOS_OK;
}

MicroOS_Status_t MicroOS_delay(uint32_t ms)
{
    MICROOS_CHECK_PTR(MicroOS_handle);

    if (ms == 0)
    {
        return MICROOS_INVALID_PARAM;
    }
    uint32_t startTick = MicroOS_handle->TickCount;
    while ((MicroOS_handle->TickCount - startTick) < ms)
    {
        // 等待直到指定的毫秒数过去
    }
    return MICROOS_OK;
}

// 初始化任务池
void MicroOS_OSdelay_Init(void)
{
    // 初始化任务池
    for (int i = 0; i < OS_DELAY_TASKSIZE - 1; i++)
    {
        delay_pool[i].next = &delay_pool[i + 1];
    }
    // 最后一个节点指向 NULL
    delay_pool[OS_DELAY_TASKSIZE - 1].next = NULL;
    free_list = &delay_pool[0]; // 空闲任务池
    active_list = NULL; // 活动任务池
}

// 添加/更新任务
MicroOS_Status_t MicroOS_OSdelay(uint8_t id, uint32_t ms)
{
    MicroOS_OSdelay_Task_t *p = active_list;

    // 检查是否已有该 ID
    while (p)
    {
        if (p->id == id)
        {
            p->ms = ms;
            p->IsTimeout = false;
            return MICROOS_OK;;
        }
        p = p->next;
    }

    // 没有则从 free_list 取节点
    if (!free_list)
        return MICROOS_BUSY; // 没空闲节点了

    MicroOS_OSdelay_Task_t *node = free_list;
    free_list = free_list->next;

    node->id = id;
    node->ms = ms;
    node->IsTimeout = false;

    node->next = active_list;
    active_list = node;

    return MICROOS_OK;
}

// 查询状态
bool MicroOS_GetDelayStatus(uint8_t id)
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
            tmp->id = 0;
            tmp->ms = 0;
            tmp->IsTimeout = false;

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

