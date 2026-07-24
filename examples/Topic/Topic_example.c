#include "MicroOS.h"

uint16_t GetAdc_Value()
{
    return 0xFFF3;
}

void CalSensor(void *param) // 模拟计算传感器事件
{
    uint16_t *adc = (uint16_t *)param;

    float sensor = *adc / 11.2f;
}

void CheckFault(void *param) // 模拟检查故障事件
{
    uint16_t *adc = (uint16_t *)param;

    if (*adc > 0xffffdd)
    {
        uint8_t fault = 1;
    }
}

static uint16_t adc = 0;

void AdcCollect_Task(void *param)
{

    adc += GetAdc_Value();

    if (adc > 0xEFEFDD) // 模拟采集完成
    {
        // 当像主题发布消息时，订阅了该主题的所有订阅者都会被触发
        MicroOS_Publish(0, (void *)&adc); // 如果想传递数据，必须是全局的，订阅不会帮你管理生命周期,如果你有需求，可以subscribe + messageEvent组合的方式实现
                                          // 这样做的目的是由于整个队列是静态的，为了节省ram的开销
    }
}

int main()
{
    MicroOS_Init();

    MicroOS_CreateTopic(0, "ADC_Complete"); // 创建了一个主题
    MicroOS_Subscribe(0, 0, "CalSensor", CalSensor);
    MicroOS_Subscribe(0, 1, "CheckFault", CheckFault);

    MicroOS_AddTask(0, "ADC_Collect", AdcCollect_Task, NULL, OS_MS_TICKS(10));

    MicroOS_StartScheduler();
}
