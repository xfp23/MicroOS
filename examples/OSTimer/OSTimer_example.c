/**
 * @file    OSTimer_example.c
 * @brief   MicroOS OSTimer usage example.
 *
 * Demonstrates:
 *   1. Periodic timer      - blink an LED every 500 ms (with a user argument).
 *   2. One-shot timer      - communication timeout, "fed" with Reload() on every RX.
 *   3. Debounce pattern    - Start() on a running timer restarts it from zero.
 *   4. ISR -> main loop    - the timer callback only triggers an Event; the heavy
 *                            work is done in the scheduler loop.
 *   5. Stop / Delete       - life-cycle control from a normal task.
 *
 * IMPORTANT: OSTimer callbacks run in tick-interrupt context.
 *            Keep them short and non-blocking (no printf, no MicroOS_delay()).
 *
 * NOTE: LED_Toggle(), Log() and the IRQ handlers are placeholders for your own
 *       BSP code. Adjust the header name to match your project.
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "MicroOS.h"

/*==============================================================================
 * Placeholders for board-specific code
 *============================================================================*/

extern void LED_Toggle(uint8_t led);
extern void Log(const char *msg);

/*==============================================================================
 * IDs
 *============================================================================*/

/* OSTimer IDs (must be < MICROOS_OSTIMER_SIZE, so set it to >= 3 in MicroOS_conf.h) */
enum
{
    TIMER_LED      = 0,
    TIMER_COMM_TMO = 1,
    TIMER_DEBOUNCE = 2,
};

/* Event IDs */
enum
{
    EVENT_COMM_TIMEOUT = 0,
    EVENT_KEY_PRESSED  = 1,
};

/* Task IDs */
enum
{
    TASK_CONTROL = 0,
};

/*==============================================================================
 * Small helper: check return values
 *============================================================================*/

static void check(MicroOS_Status_t st)
{
    if (st != MICROOS_OK)
    {
        /* Handle configuration errors here (assert, log, error LED, ...) */
        for (;;) { }
    }
}

/*==============================================================================
 * 1. Periodic timer: LED blink
 *============================================================================*/

static uint8_t g_led_id = 1;

/* Runs in ISR context: just toggle a pin, nothing else. */
static void LED_TimerCb(void *args)
{
    uint8_t led = *(uint8_t *)args;
    LED_Toggle(led);
}

/*==============================================================================
 * 2. One-shot timer: communication timeout
 *    The ISR callback only notifies the main loop through an Event.
 *============================================================================*/

static void CommTimeout_TimerCb(void *args)
{
    (void)args;
    MicroOS_TriggerEvent(EVENT_COMM_TIMEOUT);   /* defer the real work */
}

/* Runs in the scheduler loop (not ISR): heavier work is allowed here. */
static void CommTimeout_EventHandler(void *userdata)
{
    (void)userdata;
    Log("Communication timeout!");

    /* Re-arm the one-shot timer to keep monitoring. This is safe because the
     * timer was already stopped when it expired. */
    MicroOS_OSTimer_Start(TIMER_COMM_TMO);
}

/* Called from the UART RX interrupt each time a byte/frame arrives. */
void UART_RX_IRQHandler(void)
{
    /* ... read data ... */

    /* "Feed" the timeout timer: restart the current period without changing
     * its running state. */
    MicroOS_OSTimer_Reload(TIMER_COMM_TMO);
}

/*==============================================================================
 * 3. Debounce: Start() on a running timer restarts it from zero
 *============================================================================*/

static void Debounce_TimerCb(void *args)
{
    (void)args;
    /* The key signal was stable for 20 ms -> notify the main loop. */
    MicroOS_TriggerEvent(EVENT_KEY_PRESSED);
}

static void KeyPressed_EventHandler(void *userdata)
{
    (void)userdata;
    Log("Key pressed (debounced)");
}

/* Called on every GPIO edge, including contact bounce. */
void KEY_EXTI_IRQHandler(void)
{
    /* Each edge restarts the 20 ms window; the callback only fires once the
     * signal has been quiet for 20 ms. */
    MicroOS_OSTimer_Start(TIMER_DEBOUNCE);
}

/*==============================================================================
 * 5. Control task: stop and delete the LED timer after 10 s
 *============================================================================*/

static void Control_Task(void *param)
{
    static uint32_t run_count = 0;
    (void)param;

    /* Task period is 1 s (see AddTask below) */
    run_count++;

    if (run_count == 10U)
    {
        MicroOS_OSTimer_Stop(TIMER_LED);      /* stopped, still created      */
    }
    else if (run_count == 12U)
    {
        MicroOS_OSTimer_Start(TIMER_LED);     /* can be started again        */
    }
    else if (run_count == 20U)
    {
        MicroOS_OSTimer_Delete(TIMER_LED);    /* config cleared, ID reusable */
    }
}

/*==============================================================================
 * Tick ISR
 *============================================================================*/

void SysTick_Handler(void)
{
    MicroOS_TickHandler();   /* every 1/MICROOS_FREQ_HZ seconds */
}

/*==============================================================================
 * main
 *============================================================================*/

int main(void)
{
    /* BSP init (clock, GPIO, UART, SysTick at MICROOS_FREQ_HZ) ... */

    MicroOS_Init();

    /* Events handled in the scheduler loop */
    check(MicroOS_RegisterEvent(EVENT_COMM_TIMEOUT, "CommTmo",CommTimeout_EventHandler, NULL));
    check(MicroOS_RegisterEvent(EVENT_KEY_PRESSED, "Key",KeyPressed_EventHandler, NULL));

    /* 1. Periodic: toggle LED every 500 ms */
    check(MicroOS_OSTimer_Create(TIMER_LED, LED_TimerCb,OS_MS_TICKS(500), true, &g_led_id));
    check(MicroOS_OSTimer_Start(TIMER_LED));

    /* 2. One-shot: 1 s communication timeout */
    check(MicroOS_OSTimer_Create(TIMER_COMM_TMO, CommTimeout_TimerCb,
                                 OS_MS_TICKS(1000), false, NULL));
    check(MicroOS_OSTimer_Start(TIMER_COMM_TMO));

    /* 3. One-shot: 20 ms key debounce (started from the key interrupt) */
    check(MicroOS_OSTimer_Create(TIMER_DEBOUNCE, Debounce_TimerCb,
                                 OS_MS_TICKS(20), false, NULL));

    /* 5. Control task, runs once per second */
    check(MicroOS_AddTask(TASK_CONTROL, "Control", Control_Task,NULL, OS_MS_TICKS(1000)));

    MicroOS_StartScheduler();   /* never returns */

    return 0;
}