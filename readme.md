
---

# **MicroOS Scheduler Driver Documentation**

## **1. Overview**

`MicroOS` is a lightweight cooperative task scheduler designed for resource-constrained embedded systems. It provides a simple way to manage periodic tasks, basic delays, and task control (suspend, resume, delete) without the complexity of a full RTOS.

Key features:

* Single-instance scheduler (all tasks share the same MicroOS instance).
* Static task table with user-defined IDs (acts as both task identifier and priority).
* Tick-based timing system using a 1ms SysTick or timer interrupt.
* Lightweight OS-style delay mechanism using a static task pool to avoid heap fragmentation.
* Suitable for bare-metal projects and small devices.

---

## **2. Design Principles**

* **No dynamic stacks:** All tasks run on the same stack (cooperative multitasking).
* **Fixed task table:** Number of tasks is defined by `MICROOS_TASK_NUM`.
* **Tick-based scheduling:** Periodic tasks are triggered by comparing `TickCount` and `LastRunTime`.
* **Lightweight delay manager:** `OSdelay` uses a static linked list pool (`OS_DELAY_TASKSIZE`) to implement non-blocking delays.

---

## **3. Data Structures**

### **3.1 MicroOS Task Structure**

```c
typedef struct {
    bool IsUsed;                  // Task slot is allocated
    bool IsRunning;               // Task is currently active
    uint32_t Period;              // Task period in ms
    uint32_t LastRunTime;         // Last execution tick
    void (*TaskFunction)(void*);  // Task callback function
    void* Userdata;               // User-defined data pointer
} MicroOS_Task_t;
```

### **3.2 MicroOS Instance**

```c
typedef struct {
    MicroOS_Task_t Tasks[MICROOS_TASK_NUM];  // Task table
    uint32_t TickCount;                      // Global tick counter
    uint32_t MaxTasks;                       // Max number of supported tasks
    uint8_t CurrentTaskId;                   // Currently executing task ID
    uint8_t TaskNum;                         // Number of active tasks
} MicroOS_t;
```

### **3.3 OS Delay Task**

```c
typedef struct MicroOS_OSdelay_Task_t {
    uint8_t id;                          // Delay task ID
    uint32_t ms;                         // Remaining delay time
    bool IsTimeout;                      // Timeout flag
    struct MicroOS_OSdelay_Task_t *next; // Linked list pointer
} MicroOS_OSdelay_Task_t;
```

---

## **4. Public API**

### **4.1 Initialization**

```c
MicroOS_Status_t MicroOS_Init(void);
```

Initializes the MicroOS instance and clears the task table.

---

### **4.2 Adding a Task**

```c
MicroOS_Status_t MicroOS_AddTask(uint8_t id,
                                 MicroOS_TaskFunction_t TaskFunction,
                                 void *Userdata,
                                 uint32_t Period);
```

* **id:** Unique task ID (0–MICROOS\_TASK\_NUM-1). Lower IDs can be treated as higher priority.
* **TaskFunction:** Callback function pointer.
* **Userdata:** Pointer to user-defined data.
* **Period:** Task execution period in ms.

---

### **4.3 Starting the Scheduler**

```c
void MicroOS_StartScheduler(void);
```

Runs the task scheduler inside an infinite loop.
*Note:* This function does not return unless `MicroOS_handle` becomes `NULL`.

---

### **4.4 Tick Handler**

```c
MicroOS_Status_t MicroOS_TickHandler(void);
```

Increments `TickCount`.
Must be called in a **1ms SysTick or hardware timer ISR** for accurate scheduling.

---

### **4.5 Task Control**

```c
MicroOS_Status_t MicroOS_SuspendTask(uint8_t id);
MicroOS_Status_t MicroOS_ResumeTask(uint8_t id);
MicroOS_Status_t MicroOS_DeleteTask(uint8_t id);
```

* `SuspendTask`: Temporarily disables task execution.
* `ResumeTask`: Reactivates a suspended task.
* `DeleteTask`: Removes task and clears its slot.

---

### **4.6 OS Delay API**

```c
MicroOS_Status_t MicroOS_OSdelay(uint8_t id, uint32_t ms);
bool MicroOS_GetDelayStatus(uint8_t id);
void MicroOS_OSdelay_Remove(uint8_t id);
```

* `MicroOS_OSdelay(id, ms)`: Start a delay timer for a task.
* `MicroOS_GetDelayStatus(id)`: Returns `true` when delay is finished.
* `MicroOS_OSdelay_Remove(id)`: Removes the delay entry (must be called to free resources).

---

## **5. Usage Example**

### **5.1 Initialization and Task Setup**

```c
void Task_LED(void *param) {
    // Toggle LED
}

void Task_UART(void *param) {
    // Handle UART communication
}

int main(void) {
    MicroOS_Init();
    MicroOS_AddTask(0, Task_LED, NULL, 100);   // LED blink every 100ms
    MicroOS_AddTask(1, Task_UART, NULL, 10);   // UART polling every 10ms

    MicroOS_StartScheduler();  // Start cooperative scheduler
}
```

### **5.2 Tick ISR**

```c
void SysTick_Handler(void) {
    MicroOS_TickHandler();     // Must be called every 1ms
}
```

### **5.3 Using OSdelay**

```c
void Task_Sensor(void *param) {
    static bool waiting = false;

    if(!waiting) {
        MicroOS_OSdelay(1, 500);  // 500ms delay
        waiting = true;
    }

    if(MicroOS_GetDelayStatus(1)) {
        // Delay finished, perform sensor read
        MicroOS_OSdelay_Remove(1);
        waiting = false;
    }
}
```

---

## **6. Configuration**

* `MICROOS_TASK_NUM`: Maximum number of concurrent tasks (default 64).
* `OS_DELAY_TASKSIZE`: Size of the delay task pool (default 32).

---

## **7. Limitations**

* Cooperative multitasking (no preemption).
* Single stack shared by all tasks.
* No priority-based preemption (priority is implied by task period and ID).
* `OSdelay` is cooperative and must be polled.

---
