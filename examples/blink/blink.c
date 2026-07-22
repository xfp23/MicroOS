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

// OSdelay 到期后自动被调度器调用的回调函数，不再需要手动轮询
void Delay_Callback(void *userdata) {
    printf("Delay finished, doing work\n");
}

// 模拟使用 OSdelay 的任务：只需在需要时启动一次延时，
// 到期后 Delay_Callback 会被调度器自动调用，节点也会自动释放
void Task_DelayExample(void *param) {
    static bool started = false;
    if (!started) {
        // 设置延时 500ms
        MicroOS_OSdelay(0, Delay_Callback, NULL, OS_MS_TICKS(500));
        started = true;
        printf("Delay started\n");
    }
}

int main(void) {
    // 初始化 MicroOS
    if (MicroOS_Init() != MICROOS_OK) {
        printf("MicroOS initialization failed!\n");
        return -1;
    }

    // 添加任务：ID 必须唯一且小于 MICROOS_TASK_SIZE，周期用 OS_MS_TICKS(ms) 换算成 Ticks
    MicroOS_AddTask(0, "LED_Task", Task_LED, NULL, OS_MS_TICKS(1000));          // 1000 ms 周期
    MicroOS_AddTask(1, "UART_Task", Task_UART, NULL, OS_MS_TICKS(2000));        // 2000 ms 周期
    MicroOS_AddTask(2, "Delay_Task", Task_DelayExample, NULL, OS_MS_TICKS(100)); // 100 ms 周期

    // 启动调度器（一般不会返回）
    MicroOS_StartScheduler();

    return 0;
}

// 假设这是1ms硬件定时器中断调用
void SysTick_Handler(void) {
    MicroOS_TickHandler();
}