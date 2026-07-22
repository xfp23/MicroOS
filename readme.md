
[中文](README.zh.md)

# **MicroOS Scheduler Driver Documentation**

## **1. Overview**

`MicroOS` is a lightweight cooperative task scheduler designed for bare-metal embedded systems with limited resources. It provides a minimal yet flexible API to manage periodic tasks, task delays, events, and task control without the overhead of a full RTOS.

Key features:

* Single-instance cooperative scheduler.
* Static task table with user-defined IDs (ID also acts as priority — lower ID runs first).
* Event management system for asynchronous callbacks, with per-trigger user data.
* Tick-based timing system driven by a hardware timer interrupt.
* Callback-driven OSdelay manager using a static pool to avoid heap fragmentation — no manual polling required.
* Optional task sleeping mechanism.
* Suitable for MCUs with small RAM/Flash.

**Version:** `1.2.0`

---

## **2. Design Principles**

* **No dynamic stacks:** All tasks share the same call stack (cooperative multitasking).
* **Fixed-size task table:** Number of tasks defined at compile time with `MICROOS_TASK_SIZE`.
* **Static event pool:** Events are managed via a pre-allocated pool `OS_EVENT_POOLSIZE`.
* **Tick-based scheduling:** Driven by a global tick counter incremented in a hardware ISR.
* **Callback-based delay system:** Implemented using a static pool (`OS_DELAY_POOLSIZE`). Unlike a polling-style delay, `MicroOS_OSdelay` registers a callback that the scheduler invokes automatically once the delay expires — no manual "is it done yet" check or manual cleanup is needed.
* **User-defined frequency:** `MICROOS_FREQ_HZ` must match the hardware tick source.

---

## **3. Configuration**

All configuration macros live in `MicroOS_conf.h`.

```c
/* -------------------- MicroOS Version -------------------- */
#define MICROOS_VERSION_MAJOR "1.2.0"   // MicroOS version

/* -------------------- MicroOS System Frequency -------------------- */
#define MICROOS_FREQ_HZ       1000      // System clock frequency (Hz)

/* -------------------- Task Module Configuration -------------------- */
#define MICROOS_TASK_SIZE     10        // Maximum number of tasks
#define OS_DELAY_POOLSIZE     10        // Delay task pool size

/* -------------------- Event Module Configuration -------------------- */
#define OS_EVENT_POOLSIZE     10        // Event pool size
```

*The user must configure `MICROOS_FREQ_HZ` to match the timer interrupt frequency (e.g., 1000 Hz for a 1 ms tick).*

### **3.1 Time Conversion Macros**

Tick/millisecond conversion helpers (defined in `MicroOS_com.h`) are used throughout the API wherever a `Ticks` parameter is documented:

```c
// Ticks → Milliseconds
#define OS_TICKS_MS(tick)   ((tick) * (1000 / MICROOS_FREQ_HZ))

// Milliseconds → Ticks
#define OS_MS_TICKS(ms)     ((ms) * (MICROOS_FREQ_HZ / 1000))
```

---

## **4. Public API**

### **4.1 Initialization**

```c
MicroOS_Status_t MicroOS_Init(void);
```

Initializes the scheduler and clears all task and event entries.

---

### **4.2 Adding Tasks**

```c
MicroOS_Status_t MicroOS_AddTask(uint8_t id,
                                  char *Taskname,
                                  MicroOS_TaskFunction_t TaskFunction,
                                  void *Userdata,
                                  uint32_t Ticks);
```

* **id:** Task ID (0 – `MICROOS_TASK_SIZE`-1). Also acts as priority; lower ID runs first.
* **Taskname:** Task name (string identifier).
* **TaskFunction:** Pointer to the task function.
* **Userdata:** Pointer to user data passed to the task function.
* **Ticks:** Execution period, in ticks (`OS_MS_TICKS(ms)`).

---

### **4.3 Starting the Scheduler**

```c
void MicroOS_StartScheduler(void);
```

Starts the cooperative scheduler. Runs in an infinite loop, dispatching events, servicing OSdelay callbacks, and running due tasks each iteration.

---

### **4.4 Tick Handler**

```c
void MicroOS_TickHandler(void);
```

Must be called inside the hardware timer ISR every `1/MICROOS_FREQ_HZ` seconds to increment `TickCount`.

---

### **4.5 Get System Tick Count**

```c
uint32_t MicroOS_GetTick(void);
```

Returns the current tick count.

---

### **4.6 Task Control**

```c
MicroOS_Status_t MicroOS_SuspendTask(uint8_t id);
MicroOS_Status_t MicroOS_ResumeTask(uint8_t id);
MicroOS_Status_t MicroOS_ResetTask(uint8_t id);
MicroOS_Status_t MicroOS_SleepTask(uint8_t id, uint32_t Ticks);
MicroOS_Status_t MicroOS_WakeupTask(uint8_t id);
MicroOS_Status_t MicroOS_DeleteTask(uint8_t id);
```

* `SuspendTask` – Pause a task indefinitely.
* `ResumeTask` – Resume a suspended task.
* `ResetTask` – Reset a task's recorded `LastRunTime` and clear its sleep/running state.
* `SleepTask` – Puts a task to sleep for a given number of **Ticks**.
* `WakeupTask` – Wake up a sleeping task early.
* `DeleteTask` – Remove a task entry.

---

### **4.7 Delay Management**

```c
MicroOS_Status_t MicroOS_delay(uint32_t Ticks);

MicroOS_Status_t MicroOS_OSdelay(uint8_t id,
                                  MicroOS_OSdelayFunction_t OSdelayFunction,
                                  const void *Userdata,
                                  uint32_t Ticks);
```

* `MicroOS_delay()` – **Blocking** delay; busy-waits until the given number of ticks has elapsed. Blocks the entire scheduler, so use sparingly.
* `MicroOS_OSdelay()` – **Non-blocking**, callback-based delay. Registers (or re-arms, if `id` already exists) a delay of `Ticks`. When the delay expires, the scheduler automatically calls `OSdelayFunction(Userdata)` from within `MicroOS_StartScheduler()`'s main loop, and the pool entry is freed automatically afterward — no manual "done" check or manual removal is required.

---

### **4.8 Event Management**

```c
MicroOS_Status_t MicroOS_RegisterEvent(uint8_t id,
                                        char *name,
                                        MicroOS_EventFunction_t EventFunction);

void MicroOS_DeleteEvent(uint8_t id);

MicroOS_Status_t MicroOS_TriggerEvent(uint8_t id, const void *Userdata);

MicroOS_Status_t MicroOS_SuspendEvent(uint8_t id);

MicroOS_Status_t MicroOS_ResumeEvent(uint8_t id);
```

* `RegisterEvent` – Add or update an event callback with a name.
* `DeleteEvent` – Remove an event from the active list.
* `TriggerEvent` – Marks an event as triggered and attaches `Userdata` for this trigger; it executes in the scheduler loop with that data passed to the callback.
* `SuspendEvent` – Temporarily disable an event from executing.
* `ResumeEvent` – Reactivate a suspended event.

#### **Simple Example**

```c
void MyEventHandler(void *data) {
    // Handle the event
    printf("Event triggered!\n");
}

void MyTask(void *param) {
    MicroOS_TriggerEvent(0, NULL);  // Trigger event ID 0
}

int main(void) {
    MicroOS_Init();
    MicroOS_RegisterEvent(0, "MyEvent", MyEventHandler);
    MicroOS_AddTask(0, "MyTask", MyTask, NULL, OS_MS_TICKS(100));
    MicroOS_StartScheduler();
}
```

---

## **5. Usage Examples**

### **5.1 Initialization**

```c
void LED_Task(void *param) {
    // Toggle LED
}

void UART_Task(void *param) {
    // Handle UART
}

int main(void) {
    MicroOS_Init();

    MicroOS_AddTask(0, "LED_Task", LED_Task, NULL, OS_MS_TICKS(100));
    MicroOS_AddTask(1, "UART_Task", UART_Task, NULL, OS_MS_TICKS(10));

    MicroOS_StartScheduler();
}
```

### **5.2 Tick ISR**

```c
void SysTick_Handler(void) {
    MicroOS_TickHandler();  // Called every 1ms if MICROOS_FREQ_HZ = 1000
}
```

### **5.3 Sleep Example**

```c
void Sensor_Task(void *param) {
    static bool firstRun = true;
    if (firstRun) {
        MicroOS_SleepTask(0, OS_MS_TICKS(500));  // Sleep for 500ms
        firstRun = false;
        return;
    }

    // Sensor processing after sleep
}
```

### **5.4 Delay Example**

```c
void Comm_DelayHandler(void *userdata) {
    // Runs automatically once the delay expires — no polling needed
    // Do work after delay here
}

void Comm_Task(void *param) {
    static bool started = false;
    if (!started) {
        MicroOS_OSdelay(1, Comm_DelayHandler, NULL, OS_MS_TICKS(200)); // 200ms delay
        started = true;
    }
}
```

---

## **6. Limitations**

* Cooperative scheduling only (no preemption).
* Single stack shared by all tasks.
* Task priority is implicit via ID and period.
* OSdelay callbacks run from within `MicroOS_StartScheduler()`'s main loop; a long-running task or event handler will delay other callbacks in the same iteration.
* Event pool size is fixed at compile-time (`OS_EVENT_POOLSIZE`).
* Task table and delay pool sizes are fixed at compile-time (`MICROOS_TASK_SIZE`, `OS_DELAY_POOLSIZE`).

---
