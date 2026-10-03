# **MicroOS 调度器驱动文档**

[English](./readme.md)

---

## **1. 概述**

`MicroOS` 是一个专为资源受限的裸机嵌入式系统设计的轻量级协作式任务调度器。它提供了一套精简但灵活的 API，用于管理周期性任务、任务延时、事件、消息事件以及任务控制，而无需引入完整 RTOS 的开销。

主要特性：

* 单实例协作式调度器。
* 静态任务表，任务 ID 由用户定义（ID 同时也代表优先级——ID 越小越先运行）。
* **Event(事件)** 系统：适合简单、无状态的异步回调（单一固定负载，在注册时绑定）。
* **Message Event(消息事件)** 系统：适合需要每次触发携带不同负载的异步回调，底层由静态分配、拷贝式的 FIFO 队列支撑——即使连续触发（例如在中断里）也不会丢数据。
* **OSTimer（硬件定时器级软件定时器）**：由 Tick 中断直接驱动，回调在**中断上下文**中执行，支持单次/周期模式，适合对实时性要求较高的短小回调。
* 基于 Tick 的定时系统，由硬件定时器中断驱动。
* 回调驱动的 OSdelay 延时管理器，使用静态内存池，避免堆碎片——无需手动轮询。
* 可选的任务睡眠机制。
* 整个库不使用任何动态内存（不调用 `malloc`），适合 RAM/Flash 较小的 MCU，也适合车规等安全关键场景的代码库。

**版本：** `2.2.0`

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
* **独立的队列模块：** `MicroOS`提供独立的队列库,它既服务于`Message Event`模块，也可以由用户独立使用,不过它受到`MICROOS_QUEUE_DEPTH`和`MICROOS_QUEUE_SINGLE_MSG_SIZE`的管理，请考量你的实际需求与**RAM**大小做出合理的分配
* **OSDelay 与 OSTimer 该怎么选：**
  * **OSdelay** 的回调在 `MicroOS_StartScheduler()` 主循环中执行（非中断上下文），可以做较耗时的处理，但触发时刻会受其他任务影响。仅支持一次性延时。
  * **OSTimer** 的回调直接在 Tick 中断中执行，触发时刻精确，支持周期模式，但回调**必须短小、非阻塞**。如果需要做耗时处理，建议在回调里只置标志位或调用 `MicroOS_TriggerEvent()` / `MicroOS_TriggerMessageEvent()`，把实际工作交给主循环。

---

## **3. 配置**

所有配置宏都在 `MicroOS_conf.h` 中。

```c
/*==============================================================================
 * Version
 *============================================================================*/

/** MicroOS 版本 */
#define MICROOS_VERSION_MAJOR                 "2.2.0"


/*==============================================================================
 * 系统配置
 *============================================================================*/

/** 系统滴答频率 (Hz) */
#define MICROOS_FREQ_HZ                       1000U

/*==============================================================================
 * OSTimer Module
 *============================================================================*/

 /** OSTimer 池大小 */
#define MICROOS_OSTIMER_SIZE                      1U


/*==============================================================================
 * 任务模块
 *============================================================================*/

/** 调度器最大的任务数量 */
#define MICROOS_TASK_SIZE                     10U

/** OSDelay 任务池最大数量 */
#define MICROOS_OSDELAY_POOL_SIZE               10U


/*==============================================================================
 * 普通事件模块
 *============================================================================*/

/** 事件池大小  */
#define MICROOS_EVENT_POOL_SIZE               10U


/*==============================================================================
 * 消息事件模块
 *============================================================================*/

/** 使能消息事件模块 (0: Disable, 1: Enable) */
#define MICROOS_MESSAGEEVENT_ENABLE           1U

/** 消失事件模块数量 */
#define MICROOS_MESSAGEEVENT_SIZE             5U


/*==============================================================================
 * 队列模块
 *============================================================================*/

/** 队列深度 */
#define MICROOS_QUEUE_DEPTH                   5U

/** 单条队列消息大小 (单位: byte) */
#define MICROOS_QUEUE_SINGLE_MSG_SIZE         8U


/*==============================================================================
 * 订阅模块
 *============================================================================*/

/** 使能订阅模块 (0: Disable, 1: Enable) */
#define MICROOS_SUBSCRIPTION_ENABLE           1U

/** 支持的主题数量 */
#define MICROOS_TOPIC_SIZE                    5U

/** 单个主题支持的订阅者数量 */
#define MICROOS_SUBSCRIBER_NUM                3U
```

*用户必须配置 `MICROOS_FREQ_HZ` 使其与定时器中断频率一致（例如 1ms tick 对应 1000Hz）。*

每个 Message Event 都拥有自己独立的队列实例，大小由 `MICROOS_QUEUE_DEPTH`（能容纳多少条待处理消息）和 `MICROOS_QUEUE_SINGLE_MSG_SIZE`（单条消息最大字节数）决定。这两个宏是**所有 Message Event 共用**的，目前还不支持按事件单独配置。如果不同事件的负载大小差异较大，`MICROOS_QUEUE_SINGLE_MSG_SIZE` 需要按最大的那个负载来设置。

将 `MICROOS_MESSAGEEVENT_ENABLE` 设为 `0` 可以整体禁用消息事件模块。

订阅模块是一种一对多的事件触发机制,将由一个主题给所有订阅者发送通知,一次性呼叫回调所有订阅者。

将 `MICROOS_SUBSCRIPTION_ENABLE` 设为 `0` 可以整体禁用订阅模块。
订阅模块不会帮你管理程序**生命周期**,他只负责回调你发布的用户参数，生命周期请自行管理

如果希望订阅模块具备数据存储能力，可以采用 ***订阅模块 + 消息事件模块*** 的组合方式，或者参考 `4.11` 章节，自行创建独立队列实现。

该设计是库作者基于 MCU RAM 资源限制做出的取舍。订阅模块本身仅负责消息通知与模块解耦，不负责数据生命周期管理，从而降低内存占用并提高执行效率。

通过提供独立的队列模块，用户可以根据实际应用需求灵活组合数据缓存能力，而无需让所有订阅场景承担额外的 RAM 开销。该方案在保证资源可控的同时，也保留了嵌入式系统中对内存使用的高度灵活性。

`MICROOS_OSTIMER_SIZE` 决定 OSTimer 池的大小（即可同时创建的定时器数量），定时器 ID 取值范围为 `0 ~ MICROOS_OSTIMER_SIZE-1`。每个定时器占用一个静态条目，不使用动态内存。


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
MicroOS_Status_t MicroOS_AddTask(uint8_t id, char *Taskname, MicroOS_TaskFunction_t TaskFunction, void *Userdata, uint32_t Ticks);
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

MicroOS_Status_t MicroOS_OSdelay(MicroOS_OSdelayFunction_t OSdelayFunction, const void *Userdata, uint32_t Ticks);
```

* `MicroOS_delay()` – **阻塞式**延时；忙等直到指定的 tick 数过去。会阻塞整个调度器，请谨慎使用。
* `MicroOS_OSdelay()` – **非阻塞**、回调式延时。注册一个 `Ticks` 长度的延时。延时到期后，调度器会在 `MicroOS_StartScheduler()` 主循环中自动调用 `OSdelayFunction(Userdata)`，之后该内存池条目会被自动释放——不需要手动检查"是否完成"，也不需要手动移除。

---

### **4.8 事件管理**

```c
MicroOS_Status_t MicroOS_RegisterEvent(uint8_t id, char *name, MicroOS_EventFunction_t EventFunction, const void *Userdata);

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
MicroOS_Status_t MicroOS_RegisterMessageEvent(uint8_t id, const char *name, MicroOSQueue_EventFunction_t function);

MicroOS_Status_t MicroOS_DeleteMessageEvent(uint8_t id);

MicroOS_Status_t MicroOS_TriggerMessageEvent(uint8_t id, const void *data, size_t data_len);

MicroOS_Status_t MicroOS_SuspendMessageEvent(uint8_t id);

MicroOS_Status_t MicroOS_ResumeMessageEvent(uint8_t id);
```

* `RegisterMessageEvent` – 添加或更新一个消息事件回调，带名称。和普通 Event 不同，这里不绑定负载——负载是每次触发时单独传入的。
* `DeleteMessageEvent` – 删除一个消息事件及其队列中的所有内容。
* `TriggerMessageEvent` – 将 `data` 指向的 `data_len` 字节拷贝进该事件的静态队列（`data_len` 不能超过 `MICROOS_QUEUE_SINGLE_MSG_SIZE`）。可以安全地连续调用——例如在中断里——即使调度器还没来得及分发之前的触发，每条负载也会按到达顺序被保留，而不会被覆盖。如果队列已满，会返回错误。
* `SuspendMessageEvent` – 暂时禁止某个消息事件被执行（已排队的消息会保留，但不会被分发）。
* `ResumeMessageEvent` – 重新激活一个被暂停的消息事件。

回调函数收到的是一个指向出队消息的指针（`data` + `len`），在回调调用期间由 MicroOS 持有：

```c
typedef struct {
    uint16_t len;
    uint8_t  data[MICROOS_QUEUE_SINGLE_MSG_SIZE];
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

### **4.10 订阅模块管理**

```c
MicroOS_Status_t MicroOS_CreateTopic(uint8_t id, const char *topic);

MicroOS_Status_t MicroOS_DeleteTopic(uint8_t id);

MicroOS_Status_t MicroOS_Subscribe(uint8_t topic_id, uint8_t sub_id, const char *name, MicroOS_SubscriberFunction_t func);

MicroOS_Status_t MicroOS_Unsubscribe(uint8_t topic_id, uint8_t sub_id);

MicroOS_Status_t MicroOS_Publish(uint8_t topic_id, const void *Userdata);

MicroOS_Status_t MicroOS_SuspendSubscription(uint8_t topic_id, uint8_t sub_id);

MicroOS_Status_t MicroOS_ResumeSubscription(uint8_t topic_id, uint8_t sub_id);

MicroOS_Status_t MicroOS_ClearSubscriptions(uint8_t topic_id);

uint8_t MicroOS_SubscriberCount(uint8_t topic_id);

bool MicroOS_IsTopicSuspended(uint8_t topic_id);

bool MicroOS_IsSubscriptionSuspended(uint8_t topic_id, uint8_t sub_id);
```

* `CreateTopic` – 创建一个发布主题，并绑定唯一的主题 ID。主题作为发布订阅机制中的数据分发入口，每个主题可以包含多个订阅者。
* `DeleteTopic` – 删除指定主题，同时移除该主题下所有订阅关系。
* `Subscribe` – 向指定主题添加订阅者，并注册对应回调函数。当主题发布数据时，所有有效订阅者的回调函数都会被调用。
* `Unsubscribe` – 从指定主题中移除一个订阅者，使其不再接收该主题发布的数据。
* `Publish` – 向指定主题发布数据，并依次调用该主题下所有未暂停订阅者的回调函数。数据由用户通过 `Userdata` 指针传入，不进行内部复制。
* `SuspendSubscription` – 暂停指定订阅者。暂停期间该订阅者不会收到主题发布的数据，但订阅关系仍然保留。
* `ResumeSubscription` – 恢复指定订阅者，使其重新接收主题发布的数据。
* `ClearSubscriptions` – 清除指定主题下的所有订阅者，但不会删除主题本身。
* `SubscriberCount` – 获取指定主题当前注册的订阅者数量。
* `IsTopicSuspended` – 判断指定主题是否处于暂停状态。
* `IsSubscriptionSuspended` – 判断指定订阅者是否处于暂停状态。

订阅回调函数原型：

```c
typedef void (*MicroOS_SubscriberFunction_t)(void *userdata);
```

回调函数参数为发布时传入的用户数据指针：

```c
void CAN_UpdateHandler(void *userdata)
{
    uint8_t *data = (uint8_t *)userdata;

    // 用户处理数据
}

int main(void)
{
    MicroOS_Init();

    MicroOS_CreateTopic(0, "CAN_RX");

    MicroOS_Subscribe(0, 0, "CAN_Handler", CAN_UpdateHandler);

    MicroOS_Publish(0, data);

    MicroOS_StartScheduler();
}
```

订阅模块采用发布/订阅（Publish-Subscribe）模型，实现发布者与订阅者之间的解耦：

```txt
Topic
 |
 +-- Subscriber 0
 |
 +-- Subscriber 1
 |
 +-- Subscriber 2


Publish()
    |
    +--> Subscriber Callback
    +--> Subscriber Callback
    +--> Subscriber Callback
```

***每个主题通过唯一 ID 管理，发布者无需关注订阅者数量以及具体实现，订阅者仅需注册回调即可接收对应主题的数据。该机制适用于事件通知、状态同步、模块间通信等场景。***

## **4.11 队列模块**

```c
MicroOS_Status_t MicroOSQueue_Init(MicroOSQueue_Obj_t *obj);

MicroOS_Status_t MicroOSQueue_Push(MicroOSQueue_Obj_t *obj, const void *data, size_t size);

MicroOS_Status_t MicroOSQueue_Pop(MicroOSQueue_Obj_t *obj, void *data, size_t *size);

bool MicroOSQueue_IsEmpty(MicroOSQueue_Obj_t *obj);

bool MicroOSQueue_IsFull(MicroOSQueue_Obj_t *obj);

MicroOS_Status_t MicroOSQueue_Reset(MicroOSQueue_Obj_t *obj);
```

* `MicroOSQueue_Init` – 初始化一个队列对象。该函数用于初始化队列内部状态，包括读写索引以及队列缓冲区管理信息。队列采用静态内存管理方式，不会动态申请内存。

* `MicroOSQueue_Push` – 向队列中写入一条数据。函数会将用户提供的 `data` 指向的数据复制到队列内部缓冲区中，并根据 `size` 指定的数据长度保存消息。当队列已满或者数据长度超过单条消息最大限制时，会返回错误。

* `MicroOSQueue_Pop` – 从队列中读取一条数据。函数会将队列中的消息复制到用户提供的 `data` 缓冲区，并通过 `size` 返回实际读取的数据长度。当队列为空时，会返回错误。

* `MicroOSQueue_IsEmpty` – 判断队列是否为空。如果队列当前没有任何消息，则返回 `true`，否则返回 `false`。

* `MicroOSQueue_IsFull` – 判断队列是否已满。如果队列当前无法继续存储新的消息，则返回 `true`，否则返回 `false`。

* `MicroOSQueue_Reset` – 重置队列。清空队列中的所有消息，并恢复队列初始状态。该操作不会释放任何内存。

* `MicroOSQueue_Size` - 获取队列剩余空间大小。

### **队列数据结构**

队列内部采用静态数组进行存储：

```c
typedef struct
{
    uint16_t len;
    uint8_t  data[MICROOS_QUEUE_SINGLE_MSG_SIZE];

} MicroOSQueue_Message_t;
```

其中：

* `len` – 表示当前消息的数据长度。
* `data` – 用于存储消息内容，最大长度由 `MICROOS_QUEUE_SINGLE_MSG_SIZE` 配置。

队列对象：

```c
typedef struct
{
    MicroOSQueue_Message_t buffer[MICROOS_QUEUE_DEPTH];

    uint16_t head;
    uint16_t tail;

} MicroOSQueue_Obj_t;
```

其中：

* `buffer` – 消息存储缓冲区。
* `head` – 消息读取位置。
* `tail` – 消息写入位置。

### **队列使用示例**

```c
MicroOSQueue_Obj_t queue;


/* 初始化队列 */
MicroOSQueue_Init(&queue);


/* 写入消息 */
uint8_t tx_data[8] = {
    0x11,
    0x22,
    0x33,
    0x44,
    0x55,
    0x66,
    0x77,
    0x88
};

MicroOSQueue_Push(&queue, tx_data, sizeof(tx_data));


/* 读取消息 */
uint8_t rx_data[8];
size_t len = sizeof(rx_data);

if(MicroOSQueue_Pop(&queue, rx_data, &len) == MICROOS_OK)
{
    // 用户处理接收到的数据
}
```

### **队列工作流程**

```txt
              Push()
                |
                |
                v

        +----------------+
        | Queue Buffer   |
        |                |
        |  Message 0     |
        |  Message 1     |
        |  Message 2     |
        |       ...      |
        +----------------+

                |
                |
              Pop()

                |
                v

          User Process
```

### **设计特点**

队列模块采用静态内存管理方式，不使用 `malloc/free`，适用于资源受限的嵌入式系统。

主要特点：

* 支持固定长度消息存储。
* 无动态内存分配，避免内存碎片。
* 数据写入时自动复制，消息生命周期由队列管理。
* 支持生产者与消费者模型。
* 可用于任务间通信、事件缓存、协议数据缓存等场景。

典型应用：

* UART / CAN 接收缓存。
* Message Event 消息存储。
* 任务间异步通信。
* 中断数据缓冲。

## **4.12 OSTimer 模块**

```c
MicroOS_Status_t MicroOS_OSTimer_Create(uint8_t id, MicroOS_TimerFunction_t timerFunction, uint32_t period, bool is_auto_reload, void *args);

MicroOS_Status_t MicroOS_OSTimer_Delete(uint8_t id);

MicroOS_Status_t MicroOS_OSTimer_Start(uint8_t id);

MicroOS_Status_t MicroOS_OSTimer_Stop(uint8_t id);

MicroOS_Status_t MicroOS_OSTimer_Reload(uint8_t id);
```

回调函数原型：

```c
typedef void (*MicroOS_TimerFunction_t)(void *args);
```

* `OSTimer_Create` – 创建（配置）一个定时器，**不会自动启动**，需调用 `OSTimer_Start()`。
  * **id：** 定时器 ID（0 – `MICROOS_OSTIMER_SIZE`-1）。
  * **timerFunction：** 到期回调，不能为 `NULL`。
  * **period：** 周期，单位为 ticks（`OS_MS_TICKS(ms)`），必须大于 0，否则返回 `MICROOS_INVALID_PARAM`。
  * **is_auto_reload：** `true` 为周期模式，到期后自动重新计时；`false` 为单次模式，到期后自动停止（定时器仍保持已创建状态，可再次 `Start`）。
  * **args：** 每次到期时传给回调的用户参数，可为 `NULL`。
* `OSTimer_Delete` – 停止并删除定时器，清除其全部配置，ID 可重新用于 `Create`。重复删除是安全的。
* `OSTimer_Start` – 启动定时器，并将计数清零，从完整的一个周期开始计时。对正在运行的定时器调用会使其重新开始计时。定时器必须已创建。
* `OSTimer_Stop` – 停止定时器并将计数清零。定时器保持已创建状态，可再次 `Start`。注意这里没有"暂停/继续"，已经过去的时间会被丢弃。
* `OSTimer_Reload` – 手动重载：将计数清零，重新开始当前周期，**不改变**运行状态（不会启动已停止的定时器）。常用于"喂狗"式的超时检测。

> **⚠️ 注意：OSTimer 的回调运行在 Tick 中断上下文中**
>
> * 回调必须短小、非阻塞，不要调用 `printf`、`MicroOS_delay()`、阻塞式互斥等函数。
> * 回调耗时过长会拖慢其他定时器的计数和其他中断的响应。
> * 需要做耗时处理时，请在回调中触发 Event / Message Event，把工作交给主循环。
> * 主循环中调用 `Create / Delete / Start / Stop / Reload` 与中断中的分发共享同一份数据，请根据目标平台决定是否需要加临界区保护。

#### **OSdelay 与 OSTimer 对比**

| 特性 | `MicroOS_OSdelay` | `MicroOS_OSTimer` |
|---|---|---|
| 回调执行上下文 | 主循环（`MicroOS_StartScheduler`） | Tick 中断（ISR） |
| 模式 | 仅一次性 | 单次 / 周期 |
| 触发精度 | 受主循环中其他任务影响 | 由 Tick 中断决定，较精确 |
| 回调中允许的操作 | 可做较耗时处理 | 必须短小、非阻塞 |
| 到期后条目 | 自动释放 | 单次模式：停止但保留；周期模式：自动重新计时 |
| 条目数量 | `MICROOS_OSDELAY_POOL_SIZE` | `MICROOS_OSTIMER_SIZE` |

#### **OSTimer 示例**

```c
static void LED_TimerCb(void *args)
{
    // 在中断上下文中执行，保持简短
    LED_Toggle();
}

static void Timeout_TimerCb(void *args)
{
    // 超时后只通知主循环，不在这里做耗时处理
    MicroOS_TriggerEvent(0);
}

int main(void)
{
    MicroOS_Init();

    // 周期定时器：每 500ms 翻转一次 LED
    MicroOS_OSTimer_Create(0, LED_TimerCb, OS_MS_TICKS(500), true, NULL);
    MicroOS_OSTimer_Start(0);

    // 单次定时器：1s 超时检测
    MicroOS_OSTimer_Create(1, Timeout_TimerCb, OS_MS_TICKS(1000), false, NULL);
    MicroOS_OSTimer_Start(1);

    MicroOS_StartScheduler();
}

// 在收到数据时"喂狗"，推迟超时
void UART_RX_Handler(void)
{
    MicroOS_OSTimer_Reload(1);
}

// 不再需要时
// MicroOS_OSTimer_Stop(0);
// MicroOS_OSTimer_Delete(0);
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

`MicroOS_OSdelay()` 用于注册一个一次性延时任务。延时时间到达后，MicroOS 会自动执行对应的回调函数，无需用户轮询或阻塞等待。

```c
void Comm_DelayHandler(void *userdata)
{
    // 200ms 到期后自动执行
    // 在这里处理延时结束后的逻辑
}

void Comm_Task(void *param)
{
    static bool started = false;

    if (!started)
    {
        // 注册一个 200ms 的一次性延时任务
        MicroOS_OSdelay(Comm_DelayHandler, NULL, OS_MS_TICKS(200));

        started = true;
    }

    // 其他任务逻辑……
}
```

`MicroOS_OSdelay()` 采用**非阻塞**方式实现延时，不会占用 CPU 进行等待，也不会阻塞当前任务。延时时间到达后，对应回调函数仅执行一次，如需再次延时，需重新调用 `MicroOS_OSdelay()` 进行注册。

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
* **Message Event** 的负载大小受 `MICROOS_QUEUE_SINGLE_MSG_SIZE` 限制，超过会被拒绝。每个事件的队列深度受 `MICROOS_QUEUE_DEPTH` 限制，触发速度超过调度器分发速度、且超出该深度时，会返回错误，而不是静默覆盖旧数据。
* 事件池大小在编译期由 `OS_EVENT_POOLSIZE` 固定。
* 消息事件池大小在编译期由 `MICROOS_MESSAGEEVENT_SIZE` 固定，也可以通过 `MICROOS_MESSAGEEVENT_ENABLE` 整体裁剪掉。
* 任务表和延时池大小在编译期由 `MICROOS_TASK_SIZE`、`OS_DELAY_POOLSIZE` 固定。
* 全程不使用任何动态内存（任务、事件、消息事件、延时、队列均为静态内存池）。
* **OSTimer** 的回调运行在 Tick 中断上下文中，必须短小、非阻塞；回调过长会影响其他定时器的计数精度和系统中断响应。
* OSTimer 池大小在编译期由 `MICROOS_OSTIMER_SIZE` 固定。定时器不支持暂停/继续，`Stop` 会丢弃已计时间。

---
