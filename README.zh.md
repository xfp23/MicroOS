
[English](./readme.md)

# **MicroOS 调度器驱动文档**

## **1. 概述**

`MicroOS` 是一个专为资源受限的裸机嵌入式系统设计的轻量级协作式任务调度器。它提供了一套精简但灵活的 API，用于管理周期性任务、任务延时、事件、消息事件以及任务控制，而无需引入完整 RTOS 的开销。

主要特性：

* 单实例协作式调度器。
* 静态任务表，任务 ID 由用户定义（ID 同时也代表优先级——ID 越小越先运行）。
* **Event（事件）**系统：适合简单、无状态的异步回调（单一固定负载，在注册时绑定）。
* **Message Event（消息事件）**系统：适合需要每次触发携带不同负载的异步回调，底层由静态分配、拷贝式的 FIFO 队列支撑——即使连续触发（例如在中断里）也不会丢数据。
* 基于 Tick 的定时系统，由硬件定时器中断驱动。
* 回调驱动的 OSdelay 延时管理器，使用静态内存池，避免堆碎片——无需手动轮询。
* 可选的任务睡眠机制。
* 整个库不使用任何动态内存（不调用 `malloc`），适合 RAM/Flash 较小的 MCU，也适合车规等安全关键场景的代码库。

**版本：** `1.2.3`

---

## **2. 设计原则**

* **无动态栈：** 所有任务共用同一个调用栈（协作式多任务）。
* **无动态内存：** 任务、事件、消息事件、延时、队列全部由固定大小、编译期确定的静态内存池支撑，全程没有 `malloc`/`free`。
* **固定大小的任务表：** 任务数量在编译期由 `MICROOS_TASK_SIZE` 确定。
* **静态事件池：** 事件通过预分配的内存池 `OS_EVENT_POOLSIZE` 管理。
* **Event 和 Message Event 该怎么选，取决于你的数据：**
  * **Event** 只存一个 `Userdata` 指针，在调用 `MicroOS_RegisterEvent()` 时绑定一次。之后每次 `MicroOS_TriggerEvent()` 都会用这同一个绑定的指针触发回调——没有每次触发独立的数据，也不排队。适合"发生了某件事"这类简单、无状态的通知，回调不需要区分是哪一次触发的数据。
  * **Message Event** 携带每次触发独立的负载，在 `MicroOS_TriggerMessageEvent()` 调用时被拷贝进一个静态 FIFO 队列。如果在调度器来得及分发之前被多次触发，每条负载都会按到达顺序保留下来，而不是被后一次覆盖。适合同一个事件可能连续多次触发、且每次数据都不同的场景——典型的例子是中断里连续压入传感器/CAN/UART 数据。
* **基于 Tick 的调度：** 由硬件中断里递增的全局 tick 计数器驱动。
* **回调式延时系统：** 由静态内存池（`OS_DELAY_POOLSIZE`）实现。和轮询式延时不同，`MicroOS_OSdelay` 注册的是一个回调函数，延时到期后由调度器自动调用——不需要手动检查"是否完成"，也不需要手动清理。
* **用户自定义频率：** `MICROOS_FREQ_HZ` 必须和硬件 tick 源保持一致。

---

## **3. 配置**

所有配置宏都在 `MicroOS_conf.h` 中。

```c
/*======================================================================
 * MicroOS 版本
 *====================================================================*/
#define MICROOS_VERSION_MAJOR          "1.2.3"   // MicroOS 版本号

/*======================================================================
 * 系统配置
 *====================================================================*/
#define MICROOS_FREQ_HZ                1000U     // 系统 tick 频率 (Hz)

/*======================================================================
 * 任务模块配置
 *====================================================================*/
#define MICROOS_TASK_SIZE              10U       // 调度器最大任务数
#define OS_DELAY_POOLSIZE              10U       // OSdelay 对象池大小

/*======================================================================
 * 事件模块配置
 *====================================================================*/
#define OS_EVENT_POOLSIZE              10U       // 最大注册事件数

/*======================================================================
 * 消息事件模块配置
 *====================================================================*/
#define MICROOS_MESSAGEEVENT_ENABLE    1U        // 是否启用消息事件模块 (0: 禁用, 1: 启用)
#define MICROOS_MESSAGEEVENT_SIZE      10U       // 最大消息事件数

/*======================================================================
 * 队列模块配置
 *====================================================================*/
#define MICROOS_QUEUE_DEPTH            10U       // 队列容量（可容纳的消息条数）
#define MICROOS_QUEUE_MSG_SIZE         12U       // 单条消息最大负载字节数
```

*用户必须配置 `MICROOS_FREQ_HZ` 使其与定时器中断频率一致（例如 1ms tick 对应 1000Hz）。*

每个 Message Event 都拥有自己独立的队列实例，大小由 `MICROOS_QUEUE_DEPTH`（能容纳多少条待处理消息）和 `MICROOS_QUEUE_MSG_SIZE`（单条消息最大字节数）决定。这两个宏是**所有 Message Event 共用**的，目前还不支持按事件单独配置。如果不同事件的负载大小差异较大，`MICROOS_QUEUE_MSG_SIZE` 需要按最大的那个负载来设置。

将 `MICROOS_MESSAGEEVENT_ENABLE` 设为 `0` 可以整体禁用消息事件模块。

### **3.1 时间换算宏**

Tick/毫秒换算辅助宏（定义在 `MicroOS_com.h` 中），在 API 文档中凡是涉及 `Ticks` 参数的地方都会用到：

```c
// Ticks → 毫秒
#define OS_TICKS_MS(tick)   ((tick) * (1000 / MICROOS_FREQ_HZ))

// 毫秒 → Ticks
#define OS_MS_TICKS(ms)     ((ms) * (MICROOS_FREQ_HZ / 1000))
```

---

## **4. 公开 API**

### **4.1 初始化**

```c
MicroOS_Status_t MicroOS_Init(void);
```

初始化调度器，清空所有任务、事件和消息事件条目。

---

### **4.2 添加任务**

```c
MicroOS_Status_t MicroOS_AddTask(uint8_t id,
                                  char *Taskname,
                                  MicroOS_TaskFunction_t TaskFunction,
                                  void *Userdata,
                                  uint32_t Ticks);
```

* **id：** 任务 ID（0 – `MICROOS_TASK_SIZE`-1）。同时也代表优先级，ID 越小越先运行。
* **Taskname：** 任务名（字符串标识）。
* **TaskFunction：** 任务函数指针。
* **Userdata：** 传给任务函数的用户数据指针。
* **Ticks：** 执行周期，单位为 ticks（`OS_MS_TICKS(ms)`）。

---

### **4.3 启动调度器**

```c
void MicroOS_StartScheduler(void);
```

启动协作式调度器。运行在一个无限循环中，每一轮都会分发事件、分发消息事件、处理 OSdelay 回调、并运行到期的任务。

---

### **4.4 Tick 处理函数**

```c
void MicroOS_TickHandler(void);
```

必须在硬件定时器中断里每 `1/MICROOS_FREQ_HZ` 秒调用一次，用于递增 `TickCount`。

---

### **4.5 获取系统 Tick 计数**

```c
uint32_t MicroOS_GetTick(void);
```

返回当前的 tick 计数。

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

* `SuspendTask` – 无限期暂停一个任务。
* `ResumeTask` – 恢复一个被暂停的任务。
* `ResetTask` – 重置任务记录的 `LastRunTime`，并清除其睡眠/运行状态。
* `SleepTask` – 让任务睡眠指定的 **Ticks** 数。
* `WakeupTask` – 提前唤醒一个正在睡眠的任务。
* `DeleteTask` – 删除一个任务条目。

---

### **4.7 延时管理**

```c
MicroOS_Status_t MicroOS_delay(uint32_t Ticks);

MicroOS_Status_t MicroOS_OSdelay(uint8_t id,
                                  MicroOS_OSdelayFunction_t OSdelayFunction,
                                  const void *Userdata,
                                  uint32_t Ticks);
```

* `MicroOS_delay()` – **阻塞式**延时；忙等直到指定的 tick 数过去。会阻塞整个调度器，请谨慎使用。
* `MicroOS_OSdelay()` – **非阻塞**、回调式延时。注册（若 `id` 已存在则重新装载）一个 `Ticks` 长度的延时。延时到期后，调度器会在 `MicroOS_StartScheduler()` 主循环中自动调用 `OSdelayFunction(Userdata)`，之后该内存池条目会被自动释放——不需要手动检查"是否完成"，也不需要手动移除。

---

### **4.8 事件管理**

```c
MicroOS_Status_t MicroOS_RegisterEvent(uint8_t id,
                                        char *name,
                                        MicroOS_EventFunction_t EventFunction,
                                        const void *Userdata);

void MicroOS_DeleteEvent(uint8_t id);

MicroOS_Status_t MicroOS_TriggerEvent(uint8_t id);

MicroOS_Status_t MicroOS_SuspendEvent(uint8_t id);

MicroOS_Status_t MicroOS_ResumeEvent(uint8_t id);
```

* `RegisterEvent` – 添加或更新一个事件回调，带名称，并在注册时绑定一个**固定**的负载指针（`Userdata`）。
* `DeleteEvent` – 从活动事件列表中移除一个事件。
* `TriggerEvent` – 将某个事件标记为已触发；调度器循环中执行时使用的是注册时绑定的 `Userdata`。触发调用**不带**每次独立的负载——每次触发看到的都是同一个绑定指针，触发之间也不排队。如果需要每次触发携带不同数据，请改用 **Message Event**（见 4.9 节）。
* `SuspendEvent` – 暂时禁止一个事件被执行。
* `ResumeEvent` – 重新激活一个被暂停的事件。

#### **简单示例**

```c
static int ledState = 0;

void MyEventHandler(void *data) {
    int *state = (int *)data;
    printf("Event triggered! state=%d\n", *state);
}

void MyTask(void *param) {
    MicroOS_TriggerEvent(0);  // 触发事件 ID 0（使用注册时绑定的数据）
}

int main(void) {
    MicroOS_Init();
    MicroOS_RegisterEvent(0, "MyEvent", MyEventHandler, &ledState);
    MicroOS_AddTask(0, "MyTask", MyTask, NULL, OS_MS_TICKS(100));
    MicroOS_StartScheduler();
}
```

---

### **4.9 消息事件管理**

```c
MicroOS_Status_t MicroOS_RegisterMessageEvent(uint8_t id,
                                               const char *name,
                                               MicroOSQueue_EventFunction_t function);

MicroOS_Status_t MicroOS_DeleteMessageEvent(uint8_t id);

MicroOS_Status_t MicroOS_TriggerMessageEvent(uint8_t id, const void *data, size_t data_len);

MicroOS_Status_t MicroOS_SuspendMessageEvent(uint8_t id);

MicroOS_Status_t MicroOS_ResumeMessageEvent(uint8_t id);
```

* `RegisterMessageEvent` – 添加或更新一个消息事件回调，带名称。和普通 Event 不同，这里不绑定负载——负载是每次触发时单独传入的。
* `DeleteMessageEvent` – 删除一个消息事件及其队列中的所有内容。
* `TriggerMessageEvent` – 将 `data` 指向的 `data_len` 字节拷贝进该事件的静态队列（`data_len` 不能超过 `MICROOS_QUEUE_MSG_SIZE`）。可以安全地连续调用——例如在中断里——即使调度器还没来得及分发之前的触发，每条负载也会按到达顺序被保留，而不会被覆盖。如果队列已满，会返回错误。
* `SuspendMessageEvent` – 暂时禁止某个消息事件被执行（已排队的消息会保留，但不会被分发）。
* `ResumeMessageEvent` – 重新激活一个被暂停的消息事件。

回调函数收到的是一个指向出队消息的指针（`data` + `len`），在回调调用期间由 MicroOS 持有：

```c
typedef struct {
    uint16_t len;
    uint8_t  data[MICROOS_QUEUE_MSG_SIZE];
} MicroOSQueue_Message_t;
```

#### **消息事件示例**

```c
void CAN_MessageHandler(const MicroOSQueue_Message_t *msg) {
    printf("Got %u bytes: ", msg->len);
    for (uint16_t i = 0; i < msg->len; i++) {
        printf("%02X ", msg->data[i]);
    }
    printf("\n");
}

// 在 CAN 接收中断中调用——可能连续触发多次
void CAN_RX_IRQHandler(void) {
    uint8_t frame[8];
    uint8_t len = CAN_ReadFrame(frame);
    MicroOS_TriggerMessageEvent(0, frame, len); // 每一帧都会被排队，不会被覆盖
}

int main(void) {
    MicroOS_Init();
    MicroOS_RegisterMessageEvent(0, "CAN_RX", CAN_MessageHandler);
    MicroOS_StartScheduler();
}
```

---

## **5. 使用示例**

### **5.1 初始化**

```c
void LED_Task(void *param) {
    // 切换 LED 状态
}

void UART_Task(void *param) {
    // 处理 UART
}

int main(void) {
    MicroOS_Init();

    MicroOS_AddTask(0, "LED_Task", LED_Task, NULL, OS_MS_TICKS(100));
    MicroOS_AddTask(1, "UART_Task", UART_Task, NULL, OS_MS_TICKS(10));

    MicroOS_StartScheduler();
}
```

### **5.2 Tick 中断**

```c
void SysTick_Handler(void) {
    MicroOS_TickHandler();  // 若 MICROOS_FREQ_HZ = 1000，则每 1ms 调用一次
}
```

### **5.3 睡眠示例**

```c
void Sensor_Task(void *param) {
    static bool firstRun = true;
    if (firstRun) {
        MicroOS_SleepTask(0, OS_MS_TICKS(500));  // 睡眠 500ms
        firstRun = false;
        return;
    }

    // 睡眠结束后的传感器处理
}
```

### **5.4 延时示例**

```c
void Comm_DelayHandler(void *userdata) {
    // 延时到期后自动运行——无需轮询
    // 在这里做延时结束后要做的事
}

void Comm_Task(void *param) {
    static bool started = false;
    if (!started) {
        MicroOS_OSdelay(1, Comm_DelayHandler, NULL, OS_MS_TICKS(200)); // 200ms 延时
        started = true;
    }
}
```

### **5.5 Event 与 Message Event 该怎么选**

```c
// Event：适合无状态的“发生了某事”通知。
// 每次触发用的都是同一个绑定的 Userdata 指针。
MicroOS_RegisterEvent(0, "ButtonPressed", OnButtonPressed, NULL);
MicroOS_TriggerEvent(0);

// Message Event：当每次触发都携带不同的数据、
// 且这些数据在被处理前不能丢失时使用。
MicroOS_RegisterMessageEvent(1, "SensorSample", OnSensorSample);
MicroOS_TriggerMessageEvent(1, &sample, sizeof(sample));
```

---

## **6. 局限性**

* 仅支持协作式调度（无抢占）。
* 所有任务共用同一个栈。
* 任务优先级通过 ID 和周期隐式体现。
* OSdelay 回调运行在 `MicroOS_StartScheduler()` 的主循环中；如果某个任务、事件或消息事件的处理函数运行时间过长，会延迟同一轮里的其他回调。
* **Event** 没有每次触发独立的负载，也不排队：`MicroOS_RegisterEvent()` 绑定的 `Userdata` 被所有触发共用。频繁触发不会丢失"触发次数"本身（回调仍然会按触发次数执行），但无法为每次触发传递不同的数据——如果需要，请使用 **Message Event**。
* **Message Event** 的负载大小受 `MICROOS_QUEUE_MSG_SIZE` 限制，超过会被拒绝。每个事件的队列深度受 `MICROOS_QUEUE_DEPTH` 限制，触发速度超过调度器分发速度、且超出该深度时，会返回错误，而不是静默覆盖旧数据。
* 事件池大小在编译期由 `OS_EVENT_POOLSIZE` 固定。
* 消息事件池大小在编译期由 `MICROOS_MESSAGEEVENT_SIZE` 固定，也可以通过 `MICROOS_MESSAGEEVENT_ENABLE` 整体裁剪掉。
* 任务表和延时池大小在编译期由 `MICROOS_TASK_SIZE`、`OS_DELAY_POOLSIZE` 固定。
* 全程不使用任何动态内存（任务、事件、消息事件、延时、队列均为静态内存池）。

---
