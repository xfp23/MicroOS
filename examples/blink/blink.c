#include "MicroOS.h"
#include <stdio.h> // Only for demonstration output

// Simulated LED toggle event callback
void UserEvent_BlinkLed(void *param) {
    static int ledState = 0;
    ledState = !ledState;
    printf("LED is now %s\n", ledState ? "ON" : "OFF");
}

// Task that triggers the blink event every 500ms
void Task_TriggerBlinkEvent(void *param) {
    MicroOS_TriggerEvent(0);
}

int main(void) {
    if (MicroOS_Init() != MICROOS_OK) {
        printf("MicroOS initialization failed!\n");
        return -1;
    }

    // Register event ID 0 with the LED blink callback
    MicroOS_RegisterEvent(0, UserEvent_BlinkLed, NULL);

    // Add a task that triggers the blink event every 500ms
    MicroOS_AddTask(0, Task_TriggerBlinkEvent, NULL, OS_MS_TICKS(500));

    // Start scheduler (blocking call)
    MicroOS_StartScheduler();

    return 0;
}

// Simulated 1ms tick interrupt handler
void SysTick_Handler(void) {
    MicroOS_TickHandler();
}
