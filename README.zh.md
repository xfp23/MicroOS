
[Click here for English documentation](readme.md)

# **MicroOS 调度器驱动文档**

## **1. 概述**

`MicroOS` 是一个轻量级的合作式任务调度器，专为资源有限的裸机嵌入式系统设计。它提供了一个简洁灵活的 API，用于管理周期任务、任务延时、事件和任务控制，避免了完整 RTOS 的复杂性和资源消耗。

主要特点：

* 单实例合作式调度器。
* 静态任务表，用户自定义任务 ID。
* 异步回调的事件管理系统。
* 由硬件定时器中断驱动的基于节拍的时间系统。
* 使用静态任务池的轻量级延时管理，避免堆碎片化。
* 可选的任务睡眠机制。
* 适合 RAM/Flash 资源受限的 MCU。

---

## **2. 设计原则**

* **无动态栈分配：** 所有任务共享同一个调用栈（合作式多任务）。
* **固定大小任务表：** 任务数量编译时定义，宏 `MICROOS_TASK_SIZE`。
* **静态事件池：** 事件通过预分配池管理，大小由 `OS_EVENT_POOLSIZE` 决定。
* **基于节拍的调度：** 由硬件 ISR 增加的全局节拍计数器驱动。
* **延时系统：** 使用静态链表池实现（由 `OS_DELAY_POOLSIZE` 控制）。
* **用户自定义频率：** `MICROOS_FREQ_HZ` 需与硬件节拍源一致。

---

## **3. 配置**

### **3.1 核心宏**

```c
#define MICROOS_TASK_SIZE    10       // 最大任务数
#define OS_DELAY_POOLSIZE    10       // 最大 OSdelay 条目数
#define OS_EVENT_POOLSIZE    10       // 最大事件条目数
#define MICROOS_FREQ_HZ      1000     // 调度节拍频率，单位Hz（需与硬件定时器匹配）
```

### **3.2 时间转换宏**

```c
// 节拍 → 毫秒
#define OS_TICKS_MS(tick)   ((tick) * (1000 / MICROOS_FREQ_HZ))

// 毫秒 → 节拍
#define OS_MS_TICKS(ms)     ((ms) * (MICROOS_FREQ_HZ / 1000))
```

*用户必须确保 `MICROOS_FREQ_HZ` 与定时器中断频率匹配（例如1000Hz 对应1ms节拍）。*

---

## **4. 数据结构**

### **4.1 任务结构体**

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
} MicroOS_Task_t;
```

### **4.2 事件结构体**

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

### **4.3 OS 实例结构体**

```c
typedef struct {
    MicroOS_Task_t Tasks[MICROOS_TASK_SIZE];
    uint32_t TickCount;
    uint8_t CurrentTaskId;
} MicroOS_Task_t;
```

---

## **5. 公共 API**

### **5.1 初始化**

```c
MicroOS_Status_t MicroOS_Init(void);
```

初始化调度器，清空所有任务和事件条目。

---

### **5.2 添加任务**

```c
MicroOS_Status_t MicroOS_AddTask(uint8_t id,
                                 MicroOS_TaskFunction_t TaskFunction,
                                 void *Userdata,
                                 uint32_t PeriodTicks);
```

* **id:** 任务 ID（范围 0 到 MICROOS\_TASK\_SIZE-1）。
* **PeriodTicks:** 执行周期，单位为节拍（Ticks），可用 `OS_MS_TICKS()` 转换毫秒为节拍。

---

### **5.3 启动调度器**

```c
void MicroOS_StartScheduler(void);
```

启动合作式调度器，进入死循环开始任务调度。

---

### **5.4 节拍处理函数**

```c
MicroOS_Status_t MicroOS_TickHandler(void);
```

此函数应在硬件定时器中断内调用，每隔 `1/MICROOS_FREQ_HZ` 秒调用一次，用于增加节拍计数。

---

### **5.5 任务控制**

```c
MicroOS_Status_t MicroOS_SuspendTask(uint8_t id);
MicroOS_Status_t MicroOS_ResumeTask(uint8_t id);
MicroOS_Status_t MicroOS_DeleteTask(uint8_t id);
MicroOS_Status_t MicroOS_SleepTask(uint8_t id, uint32_t Ticks);
MicroOS_Status_t MicroOS_WakeupTask(uint8_t id);
```

* `SuspendTask` – 暂停任务。
* `ResumeTask` – 恢复被暂停任务。
* `DeleteTask` – 删除任务条目。
* `SleepTask` – 让任务进入睡眠状态指定节拍数。
* `WakeupTask` – 提前唤醒睡眠任务。

---

### **5.6 延时管理**

```c
MicroOS_Status_t MicroOS_delay(uint32_t Ticks);
MicroOS_Status_t MicroOS_OSdelay(uint8_t id, uint32_t Ticks);
bool MicroOS_OSdelayDone(uint8_t id);
void MicroOS_OSdelay_Remove(uint8_t id);
```

* `MicroOS_delay()` - 阻塞延时。
* `MicroOS_OSdelay()` – 启动延时计时。
* `MicroOS_OSdelayDone()` – 检测延时是否完成。
* `MicroOS_OSdelay_Remove()` – 释放延时条目。

---

### **5.7 事件管理**

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

* `RegisterEvent` – 添加或更新事件回调。
* `DeleteEvent` – 从活动事件列表中移除事件。
* `TriggerEvent` – 标记事件为触发状态，调度循环中执行。
* `SuspendEvent` – 临时禁止事件执行。
* `ResumeEvent` – 重新激活已暂停事件。

#### **简单示例**

```c
void MyEventHandler(void *data) {
    // 事件处理函数
    printf("事件被触发！\n");
}

void MyTask(void *param) {
    MicroOS_TriggerEvent(0);  // 触发事件ID为0的事件
}

int main(void) {
    MicroOS_Init();
    MicroOS_RegisterEvent(0, MyEventHandler, NULL);
    MicroOS_AddTask(0, MyTask, NULL, OS_MS_TICKS(100));
    MicroOS_StartScheduler();
}
```

---

## **6. 使用示例**

### **6.1 初始化**

```c
void LED_Task(void *param) {
    // 切换LED状态
}

void UART_Task(void *param) {
    // 处理UART
}

int main(void) {
    MicroOS_Init();

    MicroOS_AddTask(0, LED_Task, NULL, OS_MS_TICKS(100));
    MicroOS_AddTask(1, UART_Task, NULL, OS_MS_TICKS(10));

    MicroOS_StartScheduler();
}
```

### **6.2 节拍中断服务程序**

```c
void SysTick_Handler(void) {
    MicroOS_TickHandler();  // 假设MICROOS_FREQ_HZ=1000，1ms调用一次
}
```

### **6.3 任务睡眠示例**

```c
void Sensor_Task(void *param) {
    static bool firstRun = true;
    if(firstRun) {
        MicroOS_SleepTask(0, OS_MS_TICKS(500));  // 睡眠500ms
        firstRun = false;
        return;
    }

    // 睡眠结束后的处理
}
```

### **6.4 延时示例**

```c
void Comm_Task(void *param) {
    static bool waiting = false;

    if(!waiting) {
        MicroOS_OSdelay(1, OS_MS_TICKS(200));  // 200ms延时
        waiting = true;
    }

    if(MicroOS_OSdelayDone(1)) {
        // 延时结束，执行工作
        MicroOS_OSdelay_Remove(1);
        waiting = false;
    }
}
```

---

## **7. 限制**

* 仅支持合作式调度（无抢占）。
* 所有任务共享单一调用栈。
* 任务优先级通过 ID 和周期隐式体现。
* OSdelay 需轮询检测。
* 事件池大小在编译时固定（由 `OS_EVENT_POOLSIZE` 定义）。

---
