#include "MicroOS.h"

/**
 * @brief Simulate ADC hardware sampling.
 *
 * @return ADC raw value.
 */
uint16_t GetAdc_Value(void)
{
    static uint16_t value = 0;

    value += 0x0100;

    return value;
}

/**
 * @brief ADC calculation subscriber.
 *
 * Receive ADC conversion complete notification and calculate sensor value.
 *
 * @param param Pointer to ADC data.
 */
void CalSensor(void *param)
{
    uint16_t *adc = (uint16_t *)param;

    float sensor = *adc / 11.2f;

    printf("Sensor Value: %.2f\r\n", sensor);
}

/**
 * @brief Fault detection subscriber.
 *
 * Check ADC value and simulate fault detection.
 *
 * @param param Pointer to ADC data.
 */
void CheckFault(void *param)
{
    uint16_t *adc = (uint16_t *)param;

    if (*adc > 0x8000)
    {
        uint8_t fault = 1;

        printf("Fault detected! ADC=0x%04X\r\n", *adc);
    }
}

/**
 * @brief ADC data buffer.
 *
 * The Subscription module only transfers the pointer.
 * User is responsible for maintaining the data lifetime.
 */
static uint16_t adc = 0;

/**
 * @brief ADC acquisition task.
 *
 * Periodically collects ADC data.
 * When conversion is completed, publish the ADC topic.
 *
 * @param param User parameter.
 */
void AdcCollect_Task(void *param)
{
    adc = GetAdc_Value();

    printf("ADC Sample: 0x%04X\r\n", adc);

    /*
     * Publish ADC completion event.
     *
     * All subscribers registered to this topic
     * will be notified.
     *
     * Note:
     * Subscription does not copy or manage data lifetime.
     * The user must ensure that the data remains valid
     * during callback execution.
     *
     * If data buffering is required,
     * Subscription can be combined with Message Event.
     */
    MicroOS_Publish(0, &adc);
}

int main(void)
{
    /*
     * Initialize MicroOS.
     */
    MicroOS_Init();

    /*
     * Create ADC completion topic.
     */
    MicroOS_CreateTopic(0, "ADC_Complete");

    /*
     * Register subscribers.
     *
     * When ADC_Complete is published:
     *
     * ADC_Complete
     *       |
     *       +---- CalSensor()
     *       |
     *       +---- CheckFault()
     */
    MicroOS_Subscribe(0,0,"CalSensor",CalSensor);

    MicroOS_Subscribe(0,1,"CheckFault",CheckFault);

    /*
     * Create ADC acquisition task.
     *
     * The task runs every 10ms.
     */
    MicroOS_AddTask(0,"ADC_Collect",AdcCollect_Task,NULL,OS_MS_TICKS(10));
    /*
     * Start scheduler.
     */
    MicroOS_StartScheduler();
}