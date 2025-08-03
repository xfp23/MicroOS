
---

[中文](README.zh.md)

# **MicroOS Scheduler Driver Documentation**

## **1. Overview**

`MicroOS` is a lightweight cooperative task scheduler designed for bare-metal embedded systems with limited resources. It provides a minimal yet flexible API to manage periodic tasks, task delays, events, and task control without the overhead of a full RTOS.

Key features:

* Single-instance cooperative scheduler.
* Static task table with user-defined IDs.
* Event management system for asynchronous callbacks.
* Tick-based timing system driven by a hardware timer interrupt.
* Lightweight delay manager using a static task pool to avoid heap fragmentation.
* Optional task sleeping mechanism.
* Suitable for MCUs with small RAM/Flash.

---

## **2. Design Principles**

* **No dynamic stacks:** All tasks share the same call stack (cooperative multitasking).
* **Fixed-size task table:** Number of tasks defined at compile time with `MICROOS_TASK_SIZE`.
* **Static event pool:** Events are managed via a pre-allocated pool `OS_EVENT_POOLSIZE`.
* **Tick-based scheduling:** Driven by a global tick counter incremented in a hardware ISR.
* **Delay system:** Implemented using static linked list pool (`OS_DELAY_POOLSIZE`).
* **User-defined frequency:** `MICROOS_FREQ_HZ` must match the hardware tick source.

---

## **3. Configuration**

### **3.1 Core Macros**

```c
#define MICROOS_TASK_SIZE    10       // Maximum number of tasks
#define OS_DELAY_POOLSIZE    10       // Max OSdelay entries
#define OS_EVENT_POOLSIZE    10       // Max event entries
#define MICROOS_FREQ_HZ      1000     // Scheduler tick frequency in Hz (must match hardware timer)
````

### **3.2 Time Conversion Macros**

```c
// Ticks → Milliseconds
#define OS_TICKS_MS(tick)   ((tick) * (1000 / MICROOS_FREQ_HZ))

// Milliseconds → Ticks
#define OS_MS_TICKS(ms)     ((ms) * (MICROOS_FREQ_HZ / 1000))
```

*The user must configure `MICROOS_FREQ_HZ` to match the timer interrupt frequency (e.g., 1000Hz for 1ms tick).*

---

## **4. Data Structures**

### **4.1 Task Structure**

```c
typedef struct {
    bool IsUsed;
    bool IsRunning;
    bool IsSleeping;
    uint32_t SleepTicks;
    uint32_t Period;
    uint32_t LastRunTime;
    void (*TaskFunction)(void*);
    void* Userdata;
} MicroOS_Task_Sub_t;
```

### **4.2 Event Structure**

```c
typedef struct MicroOS_Event_Sub_t {
    uint8_t id;
    bool IsRunning;
    bool IsUsed;
    bool IsTriggered;
    void (*EventFunction)(void *data);
    void *Userdata;
    struct MicroOS_Event_Sub_t *next;
} MicroOS_Event_Sub_t;
```

### **4.3 OS Instance**

```c
typedef struct {
    MicroOS_Task_Sub_t Tasks[MICROOS_TASK_SIZE];
    uint32_t TickCount;
    uint8_t CurrentTaskId;
} MicroOS_Task_t;
```

---

## **5. Public API**

### **5.1 Initialization**

```c
MicroOS_Status_t MicroOS_Init(void);
```

Initializes the scheduler and clears all task and event entries.

---

### **5.2 Adding Tasks**

```c
MicroOS_Status_t MicroOS_AddTask(uint8_t id,
                                 MicroOS_TaskFunction_t TaskFunction,
                                 void *Userdata,
                                 uint32_t PeriodTicks);
```

* **id:** Task ID (0–MICROOS\_TASK\_SIZE-1).
* **PeriodTicks:** Execution period in **Ticks** (use `OS_MS_TICKS()` if you want to specify ms).

---

### **5.3 Starting the Scheduler**

```c
void MicroOS_StartScheduler(void);
```

Starts the cooperative scheduler. Runs in an infinite loop.

---

### **5.4 Tick Handler**

```c
MicroOS_Status_t MicroOS_TickHandler(void);
```

Must be called inside the hardware timer ISR every `1/MICROOS_FREQ_HZ` seconds to increment `TickCount`.

---

### **5.5 Task Control**

```c
MicroOS_Status_t MicroOS_SuspendTask(uint8_t id);
MicroOS_Status_t MicroOS_ResumeTask(uint8_t id);
MicroOS_Status_t MicroOS_DeleteTask(uint8_t id);
MicroOS_Status_t MicroOS_SleepTask(uint8_t id, uint32_t Ticks);
MicroOS_Status_t MicroOS_WakeupTask(uint8_t id);
```

* `SuspendTask` – Pause task indefinitely.
* `ResumeTask` – Resume a suspended task.
* `DeleteTask` – Remove task entry.
* `SleepTask` – Puts a task into sleep for a given number of **Ticks**.
* `WakeupTask` – Wake up a sleeping task early.

---

### **5.6 Delay Management**

```c
MicroOS_Status_t MicroOS_delay(uint32_t Ticks);
MicroOS_Status_t MicroOS_OSdelay(uint8_t id, uint32_t Ticks);
bool MicroOS_OSdelayDone(uint8_t id);
void MicroOS_OSdelay_Remove(uint8_t id);
```

* `MicroOS_delay()` - blocking delay.
* `MicroOS_OSdelay()` – Start a delay timer.
* `MicroOS_OSdelayDone()` – Check if delay expired.
* `MicroOS_OSdelay_Remove()` – Free delay entry.

---

### **5.7 Event Management**

#### **API**

```c
MicroOS_Status_t MicroOS_RegisterEvent(uint8_t id,
                                       MicroOS_EventFunction_t EventFunction,
                                       void *Userdata);

void MicroOS_DeleteEvent(uint8_t id);

MicroOS_Status_t MicroOS_TriggerEvent(uint8_t id);

MicroOS_Status_t MicroOS_SuspendEvent(uint8_t id);

MicroOS_Status_t MicroOS_ResumeEvent(uint8_t id);
```

* `RegisterEvent` – Add or update an event callback.
* `DeleteEvent` – Remove an event from the active list.
* `TriggerEvent` – Mark an event as triggered; it will execute in the scheduler loop.
* `SuspendEvent` – Temporarily disable an event from executing.
* `ResumeEvent` – Reactivate a suspended event.

#### **Simple Example**

```c
void MyEventHandler(void *data) {
    // Handle the event
    printf("Event triggered!\n");
}

void MyTask(void *param) {
    MicroOS_TriggerEvent(0);  // Trigger event ID 0
}

int main(void) {
    MicroOS_Init();
    MicroOS_RegisterEvent(0, MyEventHandler, NULL);
    MicroOS_AddTask(0, MyTask, NULL, OS_MS_TICKS(100));
    MicroOS_StartScheduler();
}
```

---

## **6. Usage Examples**

### **6.1 Initialization**

```c
void LED_Task(void *param) {
    // Toggle LED
}

void UART_Task(void *param) {
    // Handle UART
}

int main(void) {
    MicroOS_Init();

    MicroOS_AddTask(0, LED_Task, NULL, OS_MS_TICKS(100));
    MicroOS_AddTask(1, UART_Task, NULL, OS_MS_TICKS(10));

    MicroOS_StartScheduler();
}
```

### **6.2 Tick ISR**

```c
void SysTick_Handler(void) {
    MicroOS_TickHandler();  // Called every 1ms if MICROOS_FREQ_HZ = 1000
}
```

### **6.3 Sleep Example**

```c
void Sensor_Task(void *param) {
    static bool firstRun = true;
    if(firstRun) {
        MicroOS_SleepTask(0, OS_MS_TICKS(500));  // Sleep for 500ms
        firstRun = false;
        return;
    }

    // Sensor processing after sleep
}
```

### **6.4 Delay Example**

```c
void Comm_Task(void *param) {
    static bool waiting = false;

    if(!waiting) {
        MicroOS_OSdelay(1, OS_MS_TICKS(200));  // 200ms delay
        waiting = true;
    }

    if(MicroOS_OSdelayDone(1)) {
        // Do work after delay
        MicroOS_OSdelay_Remove(1);
        waiting = false;
    }
}
```

---

## **7. Limitations**

* Cooperative scheduling only (no preemption).
* Single stack shared by all tasks.
* Task priority is implicit via ID and period.
* OSdelay requires polling.
* Event pool size is fixed at compile-time (`OS_EVENT_POOLSIZE`).

---
