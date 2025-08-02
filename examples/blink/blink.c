#include "MicroOS.h"
#include <stdio.h> // 仅用于示例打印

// 模拟LED闪烁任务
void Task_LED(void *param) {
    static int state = 0;
    state = !state;
    printf("LED is now %s\n", state ? "ON" : "OFF");
}

// 模拟UART处理任务
void Task_UART(void *param) {
    printf("UART handling...\n");
}

// 模拟使用 OSdelay 的任务
void Task_DelayExample(void *param) {
    static bool waiting = false;

    if (!waiting) {
        // 设置延时 500MS
        MicroOS_OSdelay(0, OS_MS_TICKS(500));
        waiting = true;
        printf("Delay started\n");
    }

    if (MicroOS_OSdelayDone(0)) {
        MicroOS_OSdelay_Remove(0);
        waiting = false;
        printf("Delay finished, doing work\n");
    }
}

int main(void) {
    // 初始化 MicroOS
    if (MicroOS_Init() != MICROOS_OK) {
        printf("MicroOS initialization failed!\n");
        return -1;
    }

    // 添加任务：ID 必须唯一且小于 MICROOS_TASK_SIZE
    MicroOS_AddTask(0, Task_LED, NULL, OS_MS_TICKS(1000));        // 1000 MS 周期
    MicroOS_AddTask(1, Task_UART, NULL, OS_MS_TICKS(2000));       // 2000 MS 周期
    MicroOS_AddTask(2, Task_DelayExample, NULL, OS_MS_TICKS(100)); // 100 MS 周期

    // 启动调度器（一般不会返回）
    MicroOS_StartScheduler();

    return 0;
}

// 假设这是1ms硬件定时器中断调用
void SysTick_Handler(void) {
    MicroOS_TickHandler();
}