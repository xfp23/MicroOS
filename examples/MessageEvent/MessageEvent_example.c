#include "MicroOS.h"
#include <stdio.h> // 仅用于示例打印

/* ------------------------------------------------------------------ */
/* 消息事件回调：调度器会在主循环里自动把队列中每一条消息依次弹出来调用  */
/* ------------------------------------------------------------------ */
void UART_MessageHandler(const MicroOSQueue_Message_t *msg)
{
    printf("[UART_MessageHandler] got %u bytes: ", msg->len);
    for (uint16_t i = 0; i < msg->len; i++)
    {
        printf("%02X ", msg->data[i]);
    }
    printf("\n");
}

/* ------------------------------------------------------------------ */
/* 模拟 UART 接收中断：短时间内可能连续触发好几次                      */
/* 用 Message Event 而不是普通 Event，就是为了保证每一帧都不会         */
/* 因为调度器还没来得及处理上一帧，就被下一帧覆盖掉                    */
/* ------------------------------------------------------------------ */
void UART_RX_IRQHandler(void)
{
    static uint8_t counter = 0;
    uint8_t frame[4];

    // 模拟每次中断收到一帧不同的数据
    frame[0] = counter++;
    frame[1] = 0xAA;
    frame[2] = 0xBB;
    frame[3] = 0xCC;

    MicroOS_Status_t ret = MicroOS_TriggerMessageEvent(0, frame, sizeof(frame));

    if (ret == MICROOS_QUEUE_FULL)
    {
        // 队列满了：说明消费速度跟不上突发的触发速度，
        // 需要考虑加大 MICROOS_QUEUE_DEPTH，或者在这里加诊断计数
        printf("[UART_RX_IRQHandler] queue full, frame dropped\n");
    }
}

/* ------------------------------------------------------------------ */
/* 一个普通周期任务，用来对比：Message Event 的分发和普通任务是同一个  */
/* 调度循环里完成的，不需要用户额外做什么                              */
/* ------------------------------------------------------------------ */
void Heartbeat_Task(void *param)
{
    printf("[Heartbeat_Task] alive\n");
}

int main(void)
{
    if (MicroOS_Init() != MICROOS_OK)
    {
        printf("MicroOS initialization failed!\n");
        return -1;
    }

    // 注册消息事件：ID 0，绑定回调，不用像普通 Event 那样在这里传数据，
    // 数据是每次 TriggerMessageEvent 时才传入的
    MicroOS_RegisterMessageEvent(0, "UART_RX", UART_MessageHandler);

    // 一个普通周期任务，1000ms 打印一次心跳，方便和消息事件的触发做对比
    MicroOS_AddTask(0, "Heartbeat_Task", Heartbeat_Task, NULL, OS_MS_TICKS(1000));

    // 启动调度器：Message Event 的分发会在主循环里自动进行，
    // 不需要用户在 Heartbeat_Task 或其他任务里手动检查队列
    MicroOS_StartScheduler();

    return 0;
}

// 假设这是 1ms 硬件定时器中断调用
void SysTick_Handler(void)
{
    MicroOS_TickHandler();
}