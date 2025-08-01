
---

# **MicroOS Scheduler Driver Documentation**

## **1. Overview**

`MicroOS` is a lightweight cooperative task scheduler designed for bare-metal embedded systems with limited resources. It provides a minimal yet flexible API to manage periodic tasks, task delays, and task control without the overhead of a full RTOS.

Key features:

* Single-instance cooperative scheduler.
* Static task table with user-defined IDs.
* Tick-based timing system driven by a hardware timer interrupt.
* Lightweight delay manager using a static task pool to avoid heap fragmentation.
* Optional task sleeping mechanism.
* Suitable for MCUs with small RAM/Flash.

---

## **2. Design Principles**

* **No dynamic stacks:** All tasks share the same call stack (cooperative multitasking).
* **Fixed-size task table:** Number of tasks defined at compile time with `MICROOS_TASK_NUM`.
* **Tick-based scheduling:** Driven by a global tick counter incremented in a hardware ISR.
* **Delay system:** Implemented using static linked list pool (`OS_DELAY_TASKSIZE`).
* **User-defined frequency:** `MICROOS_FREQ_HZ` must match the hardware tick source.

---

## **3. Configuration**

### **3.1 Core Macros**

```c
#define MICROOS_TASK_NUM    64       // Maximum number of tasks
#define OS_DELAY_TASKSIZE   32       // Max OSdelay entries
#define MICROOS_FREQ_HZ        1000     // Scheduler tick frequency in Hz (must match hardware timer)
```

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
    bool IsUsed;                  // Task slot in use
    bool IsRunning;               // Task is active
    bool IsSleeping;              // Task is sleeping
    uint32_t SleepTicks;          // Remaining sleep time
    uint32_t Period;              // Execution period (in Ticks)
    uint32_t LastRunTime;         // Last execution timestamp
    void (*TaskFunction)(void*);  // Task callback
    void* Userdata;               // Pointer to user data
} MicroOS_Task_t;
```

### **4.2 OS Instance**

```c
typedef struct {
    MicroOS_Task_t Tasks[MICROOS_TASK_NUM];
    uint32_t TickCount;
    uint8_t CurrentTaskId;
} MicroOS_t;
```

### **4.3 Delay Task Structure**

```c
typedef struct MicroOS_OSdelay_Task_t {
    uint8_t id;
    uint32_t ticks;
    bool IsTimeout;
    struct MicroOS_OSdelay_Task_t *next;
} MicroOS_OSdelay_Task_t;
```

---

## **5. Public API**

### **5.1 Initialization**

```c
MicroOS_Status_t MicroOS_Init(void);
```

Initializes the scheduler and clears all task entries.

---

### **5.2 Adding Tasks**

```c
MicroOS_Status_t MicroOS_AddTask(uint8_t id,
                                 MicroOS_TaskFunction_t TaskFunction,
                                 void *Userdata,
                                 uint32_t PeriodTicks);
```

* **id:** Task ID (0–MICROOS\_TASK\_NUM-1).
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
```

* `SuspendTask` – Pause task indefinitely.
* `ResumeTask` – Resume a suspended task.
* `DeleteTask` – Remove task entry.
* `SleepTask` – Puts a task into sleep for a given number of **Ticks**.

---

### **5.6 Delay Management**

```c
MicroOS_Status_t MicroOS_OSdelay(uint8_t id, uint32_t Ticks);
bool MicroOS_GetDelayStatus(uint8_t id);
void MicroOS_OSdelay_Remove(uint8_t id);
```

* `MicroOS_OSdelay()` – Start a delay timer.
* `MicroOS_GetDelayStatus()` – Check if delay expired.
* `MicroOS_OSdelay_Remove()` – Free delay entry.

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

    if(MicroOS_GetDelayStatus(1)) {
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

---
