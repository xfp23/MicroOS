# MicroOS Lightweight Task Scheduler

MicroOS is a lightweight task scheduler designed for embedded platforms and microcontrollers, supporting periodic task management and suitable for resource-constrained projects. It can be used on any system that provides a time base (such as a timer interrupt).

## Features

- Static task allocation, no dynamic memory usage
- Support for task suspend and resume
- Task priority determined by ID (lower ID means higher priority)
- Compatible with any platform that can provide a time base (e.g., timer interrupt, system tick)

## Basic API Usage

### 1. Initialize MicroOS

```c
MicroOS_Status_t status = MicroOS_Init();
if (status != MICROOS_OK) {
    // Handle initialization failure
}
```

### 2. Add a Task

```c
void MyTask(void *userdata) {
    // Task code
}

status = MicroOS_AddTask(0, MyTask, NULL, 100); // ID=0, period=100ms
if (status != MICROOS_OK) {
    // Handle add failure
}
```

### 3. Start the Scheduler

```c
MicroOS_StartScheduler();
```

### 4. Call TickHandler in Timer Interrupt

Call `MicroOS_TickHandler()` in your system's timer interrupt (typically every 1ms):

```c
void TimerInterruptHandler(void) {
    MicroOS_TickHandler();
}
```

### 5. Suspend/Resume Task

```c
MicroOS_SuspendTask(0); // Suspend task with ID 0
MicroOS_ResumeTask(0);  // Resume task with ID 0
```

## Task Function Prototype

```c
typedef void (*MicroOS_TaskFunction_t)(void *Userdata);
```

## Notes

- The number of tasks is defined by the macro `MICROOS_TASK_NUM`, default is 10
- Each task ID must be unique and less than `MICROOS_TASK_NUM`
- All tasks run in the same MicroOS instance
- MicroOS is platform-independent and only requires a reliable time base for scheduling

---

For more details, please refer to the comments in the `MicroOS.h` header file.