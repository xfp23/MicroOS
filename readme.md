
[中文](./README.zh.md)

# **MicroOS Scheduler Driver Documentation**

## **1. Overview**

`MicroOS` is a lightweight cooperative task scheduler designed for bare-metal embedded systems with limited resources. It provides a minimal yet flexible API to manage periodic tasks, task delays, events, message events, and task control without the overhead of a full RTOS.

Key features:

* Single-instance cooperative scheduler.
* Static task table with user-defined IDs (ID also acts as priority — lower ID runs first).
* **Event** system for simple, stateless asynchronous callbacks (single fixed payload, bound at registration).
* **Message Event** system for asynchronous callbacks that need a per-trigger payload, backed by a statically-allocated, copy-based FIFO queue — safe to trigger repeatedly (e.g. from an ISR) without losing data.
* Tick-based timing system driven by a hardware timer interrupt.
* Callback-driven OSdelay manager using a static pool to avoid heap fragmentation — no manual polling required.
* Optional task sleeping mechanism.
* No dynamic memory anywhere in the library (no `malloc`), suitable for MCUs with small RAM/Flash and for safety-critical (e.g. automotive) codebases.

**Version:** `2.0.1`

---

## **2. Design Principles**

* **No dynamic stacks:** All tasks share the same call stack (cooperative multitasking).
* **No dynamic memory:** Tasks, events, message events, delays, and queues are all backed by fixed-size, compile-time-sized static pools. No `malloc`/`free` anywhere.
* **Fixed-size task table:** Number of tasks defined at compile time with `MICROOS_TASK_SIZE`.
* **Static event pool:** Events are managed via a pre-allocated pool `OS_EVENT_POOLSIZE`.
* **Event vs. Message Event — pick based on your data:**
  * **Event** stores a single `Userdata` pointer, bound once at `MicroOS_RegisterEvent()`. Every `MicroOS_TriggerEvent()` call fires the callback with that same bound pointer — there is no per-trigger payload and no queuing. This is the right tool for simple, stateless notifications ("something happened") where the callback doesn't need trigger-specific data.
  * **Message Event** carries a per-trigger payload, copied into a static FIFO queue at `MicroOS_TriggerMessageEvent()` time. If triggered multiple times before the scheduler has a chance to dispatch, each payload is preserved in order rather than being overwritten. This is the right tool when the same event may fire repeatedly with different data before being consumed — typical of an ISR pushing sensor/CAN/UART data.
* **Tick-based scheduling:** Driven by a global tick counter incremented in a hardware ISR.
* **Callback-based delay system:** Implemented using a static pool (`OS_DELAY_POOLSIZE`). Unlike a polling-style delay, `MicroOS_OSdelay` registers a callback that the scheduler invokes automatically once the delay expires — no manual "is it done yet" check or manual cleanup is needed.
* **User-defined frequency:** `MICROOS_FREQ_HZ` must match the hardware tick source.
* **Independent Queue Module:** `MicroOS` provides a standalone queue library. While it serves the `Message Event` module, it can also be used independently by the user. However, it is governed by the `MICROOS_QUEUE_DEPTH` and `MICROOS_QUEUE_SINGLE_MSG_SIZE` settings; please consider your actual requirements and available **RAM** when determining the appropriate allocation.

---

## **3. Configuration**

All configuration macros live in `MicroOS_conf.h`.

```c
/*==============================================================================
 * Version
 *============================================================================*/

/** MicroOS version */
#define MICROOS_VERSION_MAJOR                 "2.0.1"


/*==============================================================================
 * System Configuration
 *============================================================================*/

/** System tick frequency (Hz) */
#define MICROOS_FREQ_HZ                       1000U


/*==============================================================================
 * Task Module
 *============================================================================*/

/** Maximum number of scheduler tasks */
#define MICROOS_TASK_SIZE                     10U

/** Delay object pool size */
#define MICROOS_OSDELAY_POOL_SIZE               10U


/*==============================================================================
 * Event Module
 *============================================================================*/

/** Maximum number of registered events */
#define MICROOS_EVENT_POOL_SIZE               10U


/*==============================================================================
 * Message Event Module
 *============================================================================*/

/** Enable Message Event module (0: Disable, 1: Enable) */
#define MICROOS_MESSAGEEVENT_ENABLE           1U

/** Maximum number of Message Events */
#define MICROOS_MESSAGEEVENT_SIZE             5U


/*==============================================================================
 * Queue Module
 *============================================================================*/

/** Queue depth (number of messages) */
#define MICROOS_QUEUE_DEPTH                   5U

/** Maximum payload size of a single message (bytes) */
#define MICROOS_QUEUE_SINGLE_MSG_SIZE         8U


/*==============================================================================
 * Subscription Module
 *============================================================================*/

/** Enable Subscription module (0: Disable, 1: Enable) */
#define MICROOS_SUBSCRIPTION_ENABLE           1U

/** Maximum number of topics */
#define MICROOS_TOPIC_SIZE                    5U

/** Maximum number of subscribers per topic */
#define MICROOS_SUBSCRIBER_NUM                3U
```

*The user must configure `MICROOS_FREQ_HZ` to match the timer interrupt frequency (e.g., 1000 Hz for a 1 ms tick).*

Every Message Event owns its own queue instance sized by `MICROOS_QUEUE_DEPTH` (how many pending messages it can hold) and `MICROOS_QUEUE_SINGLE_MSG_SIZE` (max bytes per message). Both are shared across all Message Events — there is currently no per-event override. If your event payloads vary a lot in size, size `MICROOS_QUEUE_SINGLE_MSG_SIZE` for your largest payload.

Setting `MICROOS_MESSAGEEVENT_ENABLE` to `0` disables the Message Event module.

The subscription module implements a one-to-many event-triggering mechanism; a topic sends notifications to all subscribers, invoking their respective callbacks.

You can disable the subscription module entirely by setting `MICROOS_SUBSCRIPTION_ENABLE` to `0`.
The module does not manage the application **lifecycle**; it is solely responsible for executing the callbacks associated with the user-provided parameters. You are responsible for managing the lifecycle yourself.

If data storage capabilities are required for the subscription module, you can combine the ***subscription module*** with the ***message event module***, or implement a custom solution using an independent queue as described in section `4.11`.

This design reflects a trade-off made by the library author to account for the limited RAM resources of MCUs. The subscription module itself handles only message notification and module decoupling—not data lifecycle management—thereby minimizing memory usage and enhancing execution efficiency.

By providing a separate queue module, the library allows users to flexibly integrate data buffering capabilities based on specific application needs, avoiding the overhead of additional RAM consumption in subscription scenarios where such buffering is unnecessary. This approach ensures resource usage remains manageable while maintaining the high degree of flexibility regarding memory allocation essential to embedded systems.

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

Initializes the scheduler and clears all task, event, and message event entries.

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

Starts the cooperative scheduler. Runs in an infinite loop, dispatching events, dispatching message events, servicing OSdelay callbacks, and running due tasks each iteration.

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
                                        MicroOS_EventFunction_t EventFunction,
                                        const void *Userdata);

void MicroOS_DeleteEvent(uint8_t id);

MicroOS_Status_t MicroOS_TriggerEvent(uint8_t id);

MicroOS_Status_t MicroOS_SuspendEvent(uint8_t id);

MicroOS_Status_t MicroOS_ResumeEvent(uint8_t id);
```

* `RegisterEvent` – Add or update an event callback with a name and a **fixed** payload pointer (`Userdata`), bound once at registration time.
* `DeleteEvent` – Remove an event from the active list.
* `TriggerEvent` – Marks an event as triggered; it executes in the scheduler loop with the `Userdata` bound at registration. Triggering does **not** take a per-call payload — every trigger sees the same bound pointer, and there is no queuing between triggers. If you need per-trigger data, use a **Message Event** instead (see 4.9).
* `SuspendEvent` – Temporarily disable an event from executing.
* `ResumeEvent` – Reactivate a suspended event.

#### **Simple Example**

```c
static int ledState = 0;

void MyEventHandler(void *data) {
    int *state = (int *)data;
    printf("Event triggered! state=%d\n", *state);
}

void MyTask(void *param) {
    MicroOS_TriggerEvent(0);  // Trigger event ID 0 (uses the userdata bound at registration)
}

int main(void) {
    MicroOS_Init();
    MicroOS_RegisterEvent(0, "MyEvent", MyEventHandler, &ledState);
    MicroOS_AddTask(0, "MyTask", MyTask, NULL, OS_MS_TICKS(100));
    MicroOS_StartScheduler();
}
```

---

### **4.9 Message Event Management**

```c
MicroOS_Status_t MicroOS_RegisterMessageEvent(uint8_t id,
                                               const char *name,
                                               MicroOSQueue_EventFunction_t function);

MicroOS_Status_t MicroOS_DeleteMessageEvent(uint8_t id);

MicroOS_Status_t MicroOS_TriggerMessageEvent(uint8_t id, const void *data, size_t data_len);

MicroOS_Status_t MicroOS_SuspendMessageEvent(uint8_t id);

MicroOS_Status_t MicroOS_ResumeMessageEvent(uint8_t id);
```

* `RegisterMessageEvent` – Add or update a message event callback with a name. Unlike a plain Event, no payload is bound here — payloads are supplied per-trigger.
* `DeleteMessageEvent` – Remove a message event and its queue contents.
* `TriggerMessageEvent` – Copies `data_len` bytes from `data` into the event's static queue (`data_len` must not exceed `MICROOS_QUEUE_SINGLE_MSG_SIZE`). Safe to call repeatedly — e.g. from an ISR — before the scheduler has dispatched previous triggers; each payload is preserved in arrival order rather than overwritten. Returns an error if the queue is full.
* `SuspendMessageEvent` – Temporarily disable a message event from executing (queued messages are retained but not dispatched).
* `ResumeMessageEvent` – Reactivate a suspended message event.

The callback receives a pointer to the dequeued message (`data` + `len`), owned by MicroOS for the duration of the call:

```c
typedef struct {
    uint16_t len;
    uint8_t  data[MICROOS_QUEUE_SINGLE_MSG_SIZE];
} MicroOSQueue_Message_t;
```

#### **Message Event Example**

```c
void CAN_MessageHandler(const MicroOSQueue_Message_t *msg) {
    printf("Got %u bytes: ", msg->len);
    for (uint16_t i = 0; i < msg->len; i++) {
        printf("%02X ", msg->data[i]);
    }
    printf("\n");
}

// Called from a CAN RX interrupt — may fire several times back-to-back
void CAN_RX_IRQHandler(void) {
    uint8_t frame[8];
    uint8_t len = CAN_ReadFrame(frame);
    MicroOS_TriggerMessageEvent(0, frame, len); // each frame is queued, not overwritten
}

int main(void) {
    MicroOS_Init();
    MicroOS_RegisterMessageEvent(0, "CAN_RX", CAN_MessageHandler);
    MicroOS_StartScheduler();
}
```

### **4.10 Subscription Module Management**

```c
MicroOS_Status_t MicroOS_CreateTopic(uint8_t id,
                                     const char *topic);

MicroOS_Status_t MicroOS_DeleteTopic(uint8_t id);

MicroOS_Status_t MicroOS_Subscribe(uint8_t topic_id,
                                   uint8_t sub_id,
                                   const char *name,
                                   MicroOS_SubscriberFunction_t func);

MicroOS_Status_t MicroOS_Unsubscribe(uint8_t topic_id,
                                     uint8_t sub_id);

MicroOS_Status_t MicroOS_Publish(uint8_t topic_id,
                                 const void *Userdata);

MicroOS_Status_t MicroOS_SuspendSubscription(uint8_t topic_id,
                                             uint8_t sub_id);

MicroOS_Status_t MicroOS_ResumeSubscription(uint8_t topic_id,
                                            uint8_t sub_id);

MicroOS_Status_t MicroOS_ClearSubscriptions(uint8_t topic_id);

uint8_t MicroOS_SubscriberCount(uint8_t topic_id);

bool MicroOS_IsTopicSuspended(uint8_t topic_id);

bool MicroOS_IsSubscriptionSuspended(uint8_t topic_id,
                                     uint8_t sub_id);
```

* `CreateTopic` – Create a publish topic and assign a unique topic ID. The topic serves as the data distribution entry point in the publish-subscribe mechanism. Each topic can contain multiple subscribers.
* `DeleteTopic` – Delete the specified topic and remove all subscription relationships associated with that topic.
* `Subscribe` – Add a subscriber to the specified topic and register the corresponding callback function. When data is published to the topic, the callback functions of all active subscribers will be invoked.
* `Unsubscribe` – Remove a subscriber from the specified topic so that it no longer receives data published by that topic.
* `Publish` – Publish data to the specified topic and sequentially invoke the callback functions of all subscribers that are not suspended. The data is passed through the `Userdata` pointer provided by the user and is not copied internally.
* `SuspendSubscription` – Suspend the specified subscriber. During suspension, the subscriber will not receive data published by the topic, but the subscription relationship will be retained.
* `ResumeSubscription` – Resume the specified subscriber and allow it to receive topic data again.
* `ClearSubscriptions` – Clear all subscribers from the specified topic without deleting the topic itself.
* `SubscriberCount` – Get the current number of registered subscribers for the specified topic.
* `IsTopicSuspended` – Check whether the specified topic is currently suspended.
* `IsSubscriptionSuspended` – Check whether the specified subscriber is currently suspended.

Subscription callback function prototype:

```c
typedef void (*MicroOS_SubscriberFunction_t)(void *userdata);
```

The callback function parameter is the user data pointer passed during publishing:

```c
void CAN_UpdateHandler(void *userdata)
{
    uint8_t *data = (uint8_t *)userdata;

    // User processing data
}

int main(void)
{
    MicroOS_Init();

    MicroOS_CreateTopic(0, "CAN_RX");

    MicroOS_Subscribe(0,
                      0,
                      "CAN_Handler",
                      CAN_UpdateHandler);

    MicroOS_Publish(0, data);

    MicroOS_StartScheduler();
}
```

The Subscription module adopts the Publish-Subscribe model to achieve decoupling between publishers and subscribers:

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

***Each topic is managed through a unique ID. The publisher does not need to know the number of subscribers or their specific implementations. Subscribers only need to register a callback to receive data from the corresponding topic. This mechanism is suitable for event notification, state synchronization, module communication, and other scenarios.***

## **4.11 Queue Module**

```c
MicroOS_Status_t MicroOSQueue_Init(MicroOSQueue_Obj_t *obj);

MicroOS_Status_t MicroOSQueue_Push(MicroOSQueue_Obj_t *obj,
                                   const void *data,
                                   size_t size);

MicroOS_Status_t MicroOSQueue_Pop(MicroOSQueue_Obj_t *obj,
                                  void *data,
                                  size_t *size);

bool MicroOSQueue_IsEmpty(MicroOSQueue_Obj_t *obj);

bool MicroOSQueue_IsFull(MicroOSQueue_Obj_t *obj);

MicroOS_Status_t MicroOSQueue_Reset(MicroOSQueue_Obj_t *obj);
```

* `MicroOSQueue_Init` – Initialize a queue object. This function initializes the internal state of the queue, including read/write indexes and queue buffer management information. The queue uses static memory management and does not dynamically allocate memory.

* `MicroOSQueue_Push` – Write a data item into the queue. The function copies the data pointed to by the user-provided `data` into the internal queue buffer and stores the message according to the data length specified by `size`. An error will be returned when the queue is full or the data length exceeds the maximum size of a single message.

* `MicroOSQueue_Pop` – Read a data item from the queue. The function copies the message stored in the queue into the user-provided `data` buffer and returns the actual read data length through `size`. An error will be returned when the queue is empty.

* `MicroOSQueue_IsEmpty` – Check whether the queue is empty. Returns `true` if the queue currently contains no messages; otherwise returns `false`.

* `MicroOSQueue_IsFull` – Check whether the queue is full. Returns `true` if the queue cannot store any additional messages; otherwise returns `false`.

* `MicroOSQueue_Reset` – Reset the queue. Clears all messages in the queue and restores the queue to its initial state. This operation does not release any memory.

### **Queue Data Structure**

The queue internally uses a static array for storage:

```c
typedef struct
{
    uint16_t len;
    uint8_t  data[MICROOS_QUEUE_MSG_SIZE];

} MicroOSQueue_Message_t;
```

Where:

* `len` – Indicates the length of the current message data.
* `data` – Used to store message content. The maximum length is configured by `MICROOS_QUEUE_MSG_SIZE`.

Queue object:

```c
typedef struct
{
    MicroOSQueue_Message_t buffer[MICROOS_QUEUE_DEPTH];

    uint16_t head;
    uint16_t tail;

} MicroOSQueue_Obj_t;
```

Where:

* `buffer` – Message storage buffer.
* `head` – Message read position.
* `tail` – Message write position.

### **Queue Usage Example**

```c
MicroOSQueue_Obj_t queue;


/* Initialize queue */
MicroOSQueue_Init(&queue);


/* Write message */
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

MicroOSQueue_Push(&queue,
                  tx_data,
                  sizeof(tx_data));


/* Read message */
uint8_t rx_data[8];
size_t len = sizeof(rx_data);

if(MicroOSQueue_Pop(&queue,
                    rx_data,
                    &len) == MICROOS_OK)
{
    // User processes received data
}
```

### **Queue Workflow**

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

### **Design Features**

The Queue module uses static memory management and does not use `malloc/free`, making it suitable for resource-constrained embedded systems.

Main features:

* Supports fixed-length message storage.
* No dynamic memory allocation, avoiding memory fragmentation.
* Data is automatically copied during writing, and the message lifetime is managed by the queue.
* Supports the producer-consumer model.
* Can be used for inter-task communication, event buffering, protocol data buffering, and other scenarios.

Typical applications:

* UART / CAN receive buffer.
* Message Event message storage.
* Asynchronous communication between tasks.
* Interrupt data buffering.

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

### **5.5 Event vs. Message Event — choosing between them**

```c
// Event: fine for a stateless "something happened" notification.
// The same Userdata pointer is reused on every trigger.
MicroOS_RegisterEvent(0, "ButtonPressed", OnButtonPressed, NULL);
MicroOS_TriggerEvent(0);

// Message Event: needed when each trigger carries different data
// that must not be lost if triggered again before being handled.
MicroOS_RegisterMessageEvent(1, "SensorSample", OnSensorSample);
MicroOS_TriggerMessageEvent(1, &sample, sizeof(sample));
```

---

## **6. Limitations**

* Cooperative scheduling only (no preemption).
* Single stack shared by all tasks.
* Task priority is implicit via ID and period.
* OSdelay callbacks run from within `MicroOS_StartScheduler()`'s main loop; a long-running task, event, or message event handler will delay other callbacks in the same iteration.
* **Event** has no per-trigger payload and no queuing: the `Userdata` bound at `MicroOS_RegisterEvent()` is shared by every trigger. Triggering it rapidly does not lose "events" (the callback still runs once per trigger), but it cannot deliver distinct data per trigger — use a **Message Event** if that's required.
* **Message Event** payload size is capped by `MICROOS_QUEUE_SINGLE_MSG_SIZE`; larger payloads are rejected. Each event's queue depth is capped by `MICROOS_QUEUE_DEPTH`; triggering faster than the scheduler can dispatch, beyond that depth, returns an error rather than silently overwriting data.
* Event pool size is fixed at compile-time (`OS_EVENT_POOLSIZE`).
* Message event pool size is fixed at compile-time (`MICROOS_MESSAGEEVENT_SIZE`), and can be compiled out entirely via `MICROOS_MESSAGEEVENT_ENABLE`.
* Task table and delay pool sizes are fixed at compile-time (`MICROOS_TASK_SIZE`, `OS_DELAY_POOLSIZE`).
* No dynamic memory is used anywhere (tasks, events, message events, delays, queues are all static pools).

---