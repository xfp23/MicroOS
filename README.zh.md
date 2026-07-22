
[English](README.md)

# **MicroOS 调度器驱动文档**

## **1. 概述**

`MicroOS` 是一个面向资源受限裸机嵌入式系统设计的轻量级协作式任务调度器。它提供了一套最小化但灵活的 API，用于管理周期任务、任务延时、事件以及任务控制，同时避免完整 RTOS 带来的额外开销。

主要特性：

* 单实例协作式调度器。
* 静态任务表，任务数量由用户定义 ID（ID 同时作为优先级使用——ID 越低，任务运行优先级越高）。
* 事件管理系统，用于异步回调，并支持每次触发传递用户数据。
* 基于 Tick 的时间系统，由硬件定时器中断驱动。
* 基于回调的 OSdelay 管理器，使用静态内存池避免堆碎片问题——无需手动轮询。
* 可选的任务休眠机制。
* 适用于小型 RAM/Flash 资源的 MCU。

**版本：** `1.2.0`

---

## **2. 设计原则**

* **无动态任务栈：** 所有任务共享同一个调用栈（协作式多任务）。
* **固定大小任务表：** 任务数量通过 `MICROOS_TASK_SIZE` 在编译时定义。
* **静态事件池：** 事件通过预分配内存池 `OS_EVENT_POOLSIZE` 进行管理。
* **基于 Tick 的调度：** 由硬件 ISR 中递增的全局 Tick 计数器驱动。
* **基于回调的延时系统：** 使用静态内存池 (`OS_DELAY_POOLSIZE`) 实现。不同于轮询式延时，`MicroOS_OSdelay` 会注册一个回调函数，当延时结束后，调度器会自动调用该回调——无需手动检查是否完成，也无需手动释放资源。
* **用户自定义频率：** `MICROOS_FREQ_HZ` 必须与硬件 Tick 来源保持一致。

---

## **3. 配置**

所有配置宏位于 `MicroOS_conf.h` 中。

```c
/* -------------------- MicroOS Version -------------------- */
#define MICROOS_VERSION_MAJOR "1.2.0"   // MicroOS版本

/* -------------------- MicroOS System Frequency -------------------- */
#define MICROOS_FREQ_HZ       1000      // 系统时钟频率（Hz）

/* -------------------- Task Module Configuration -------------------- */
#define MICROOS_TASK_SIZE     10        // 最大任务数量
#define OS_DELAY_POOLSIZE     10        // 延时任务池大小

/* -------------------- Event Module Configuration -------------------- */
#define OS_EVENT_POOLSIZE     10        // 事件池大小
````

*用户必须配置 `MICROOS_FREQ_HZ` 以匹配定时器中断频率（例如：1000Hz 表示每 1ms 产生一次 Tick）。*

### **3.1 时间转换宏**

Tick/毫秒转换辅助宏（定义于 `MicroOS_com.h`），用于所有 API 中标注 `Ticks` 参数的位置：

```c
// Tick 转换为毫秒
#define OS_TICKS_MS(tick)   ((tick) * (1000 / MICROOS_FREQ_HZ))

// 毫秒转换为 Tick
#define OS_MS_TICKS(ms)     ((ms) * (MICROOS_FREQ_HZ / 1000))
```

---

## **4. 公共 API**

### **4.1 初始化**

```c
MicroOS_Status_t MicroOS_Init(void);
```

初始化调度器，并清空所有任务和事件条目。

---

### **4.2 添加任务**

```c
MicroOS_Status_t MicroOS_AddTask(uint8_t id,
                                  char *Taskname,
                                  MicroOS_TaskFunction_t TaskFunction,
                                  void *Userdata,
                                  uint32_t Ticks);
```

* **id：** 任务 ID（0 – `MICROOS_TASK_SIZE`-1）。同时作为优先级使用；ID 越低，运行优先级越高。
* **Taskname：** 任务名称（字符串标识）。
* **TaskFunction：** 任务函数指针。
* **Userdata：** 传递给任务函数的用户数据指针。
* **Ticks：** 任务执行周期，单位为 Tick（使用 `OS_MS_TICKS(ms)` 转换）。

---

### **4.3 启动调度器**

```c
void MicroOS_StartScheduler(void);
```

启动协作式调度器。该函数运行在无限循环中，每次循环分发事件、处理 OSdelay 回调，并执行到期任务。

---

### **4.4 Tick处理函数**

```c
void MicroOS_TickHandler(void);
```

必须在硬件定时器 ISR 中以 `1/MICROOS_FREQ_HZ` 秒的周期调用，用于递增 `TickCount`。

---

### **4.5 获取系统 Tick 计数**

```c
uint32_t MicroOS_GetTick(void);
```

返回当前 Tick 计数值。

---

### **4.6 任务控制**

```c
MicroOS_Status_t MicroOS_SuspendTask(uint8_t id);
MicroOS_Status_t MicroOS_ResumeTask(uint8_t id);
MicroOS_Status_t MicroOS_ResetTask(uint8_t id);
MicroOS_Status_t MicroOS_SleepTask(uint8_t id, uint32_t Ticks);
MicroOS_Status_t MicroOS_WakeupTask(uint8_t id);
MicroOS_Status_t MicroOS_DeleteTask(uint8_t id);
```

* `SuspendTask` —— 无限期暂停任务。
* `ResumeTask` —— 恢复被暂停的任务。
* `ResetTask` —— 重置任务记录的 `LastRunTime`，并清除休眠/运行状态。
* `SleepTask` —— 让任务休眠指定数量的 **Ticks**。
* `WakeupTask` —— 提前唤醒休眠中的任务。
* `DeleteTask` —— 删除任务条目。

---

## **4.7 延时管理**

```c
MicroOS_Status_t MicroOS_delay(uint32_t Ticks);

MicroOS_Status_t MicroOS_OSdelay(uint8_t id,
                                  MicroOS_OSdelayFunction_t OSdelayFunction,
                                  const void *Userdata,
                                  uint32_t Ticks);
```

* `MicroOS_delay()` —— **阻塞式**延时；等待指定 Tick 数过去。该函数会阻塞整个调度器，因此应谨慎使用。
* `MicroOS_OSdelay()` —— **非阻塞式**基于回调的延时。注册（如果 `id` 已存在，则重新启动）一个持续 `Ticks` 的延时。当延时结束后，调度器会在 `MicroOS_StartScheduler()` 主循环中自动调用 `OSdelayFunction(Userdata)`，并自动释放内存池条目——无需手动检查完成状态，也无需手动删除。

---

## **4.8 事件管理**

```c
MicroOS_Status_t MicroOS_RegisterEvent(uint8_t id,
                                        char *name,
                                        MicroOS_EventFunction_t EventFunction);

void MicroOS_DeleteEvent(uint8_t id);

MicroOS_Status_t MicroOS_TriggerEvent(uint8_t id, const void *Userdata);

MicroOS_Status_t MicroOS_SuspendEvent(uint8_t id);

MicroOS_Status_t MicroOS_ResumeEvent(uint8_t id);
```

* `RegisterEvent` —— 添加或更新一个带名称的事件回调。
* `DeleteEvent` —— 从活动列表中删除事件。
* `TriggerEvent` —— 标记事件已触发，并附带本次触发的 `Userdata`；该事件会在调度器循环中执行，并将该数据传递给回调函数。
* `SuspendEvent` —— 临时禁用事件执行。
* `ResumeEvent` —— 重新启用被暂停的事件。

### **简单示例**

```c
void MyEventHandler(void *data) {
    // 处理事件
    printf("Event triggered!\n");
}

void MyTask(void *param) {
    MicroOS_TriggerEvent(0, NULL);  // 触发事件 ID 0
}

int main(void) {
    MicroOS_Init();
    MicroOS_RegisterEvent(0, "MyEvent", MyEventHandler);
    MicroOS_AddTask(0, "MyTask", MyTask, NULL, OS_MS_TICKS(100));
    MicroOS_StartScheduler();
}
```

---

## **5. 使用示例**

### **5.1 初始化**

```c
void LED_Task(void *param) {
    // 翻转LED
}

void UART_Task(void *param) {
    // 处理UART
}

int main(void) {
    MicroOS_Init();

    MicroOS_AddTask(0, "LED_Task", LED_Task, NULL, OS_MS_TICKS(100));
    MicroOS_AddTask(1, "UART_Task", UART_Task, NULL, OS_MS_TICKS(10));

    MicroOS_StartScheduler();
}
```

---

### **5.2 Tick中断**

```c
void SysTick_Handler(void) {
    MicroOS_TickHandler();  // 当 MICROOS_FREQ_HZ = 1000 时，每1ms调用一次
}
```

---

### **5.3 休眠示例**

```c
void Sensor_Task(void *param) {
    static bool firstRun = true;

    if (firstRun) {
        MicroOS_SleepTask(0, OS_MS_TICKS(500));  // 休眠500ms
        firstRun = false;
        return;
    }

    // 休眠结束后的传感器处理
}
```

---

### **5.4 延时示例**

```c
void Comm_DelayHandler(void *userdata) {
    // 延时结束后自动运行，无需轮询
    // 在这里执行延时后的操作
}

void Comm_Task(void *param) {
    static bool started = false;

    if (!started) {
        MicroOS_OSdelay(1, Comm_DelayHandler, NULL, OS_MS_TICKS(200)); // 200ms延时
        started = true;
    }
}
```

---

## **6. 限制**

* 仅支持协作式调度（无抢占）。
* 所有任务共享同一个任务栈。
* 任务优先级通过 ID 和周期隐式决定。
* OSdelay 回调会在 `MicroOS_StartScheduler()` 主循环中执行；如果某个任务或事件处理函数执行时间过长，会导致同一次循环中的其他回调延迟执行。
* 事件池大小固定，在编译时通过 `OS_EVENT_POOLSIZE` 定义。
* 任务表和延时池大小固定，在编译时通过 `MICROOS_TASK_SIZE` 和 `OS_DELAY_POOLSIZE` 定义。

```
