#include "wifi_station.h"
#include "tcp_server.h"
#include "sensors.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"

/** @def CYCLE_PERIOD_MS
 * @brief Execution period of control loop. 
 */
#define LOOP_PERIOD_MS 50
/** @def MEASUREMENT_WCET
 * @brief Worst Case Execution Time of the measurement gathering task.
 */
#define MEASUREMENTS_WCET 10
/** @def OUTPUT_CALC_WCET
 * @brief Worst Case Execution Time of output calculation task.
 */
#define OUTPUT_CALC_WCET 40
#define MEASUREMENTS_QUEUE_LEN 1
//queue length for csv measurement rows
#define MEASUREMENTS_CSV_QUEUE_LEN 5

static const char *TAG = "main";

void measurements_task(void *pvParameters)
{
    QueueHandle_t xMeasurementsQueue = ((QueueHandle_t *)pvParameters)[0];
    QueueHandle_t xMeasurementsCSVQueue = ((QueueHandle_t *)pvParameters)[1];
    measurements_data data;
    char tx_buff[CSV_BUFF_SIZE];
    TickType_t xLastWakeTime = xTaskGetTickCount();
    while(true)
    {
        get_measurements(&data);
        //send measurements to output_compute_task
        xQueueSend(xMeasurementsQueue, (void*)(&data), portMAX_DELAY);
        //client connected to TCP server, measurements are sent to server 
        if(server_state == Connected)
        {
            if(uxQueueSpacesAvailable(xMeasurementsCSVQueue) > 1)
            {
                measurements_to_csv(tx_buff, &data);
                xQueueSend(xMeasurementsCSVQueue, (void*)(tx_buff), pdMS_TO_TICKS(0));
            }
            //only one free space is left in the queue, client is warned about data loss
            else if(uxQueueSpacesAvailable(xMeasurementsCSVQueue) == 1)
            {
                xQueueSend(xMeasurementsCSVQueue,
                           "Some data may be untransmitted\n",
                           pdMS_TO_TICKS(0));
                ESP_LOGE(TAG, "measurements task: measurements csv queue is full");
            }
        }
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(LOOP_PERIOD_MS));
    }
}

void output_compute_task(void *pvParameters)
{
    QueueHandle_t xMeasurementsQueue = (QueueHandle_t)pvParameters;
    measurements_data data;
    float out_duty = THROTTLE_STATIONARY_DUTY;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    while(true)
    {
        //ensure fixed period updates by updating control output at the beginning of the period
        set_throttle_duty(out_duty);
        BaseType_t received_ok = xQueueReceive(xMeasurementsQueue, (void*)(&data), pdMS_TO_TICKS(LOOP_PERIOD_MS/2));
        //no measurements received, out_duty set to stationary
        if(!received_ok)
        {
            ESP_LOGE(TAG, "outputcompute task: timeout for measurements data receive");
            out_duty = THROTTLE_STATIONARY_DUTY;
        }
        //output computation
        else
        {
            out_duty = data.throttle_in_duty;
        }
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(LOOP_PERIOD_MS));
    }
}

void app_main(void)
{
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    sensors_init();
    nvs_init();
    wifi_init_sta();
    /* FreeRTOS queue handle used by measurements_task() to 
    pass measurements_data instances to output_compute_task()*/
    QueueHandle_t xMeasurementsQueue = xQueueCreate(MEASUREMENTS_QUEUE_LEN, sizeof(measurements_data));
    if(xMeasurementsQueue == NULL)
    {
        ESP_LOGE(TAG, "xMeasurementsQueue could not be created");
        return;
    }
    /* FreeRTOS queue handle used by measurements_task() to
    pass measurements in csv format to the tcp server*/
    QueueHandle_t xMeasurementsCSVQueue = xQueueCreate(MEASUREMENTS_CSV_QUEUE_LEN, CSV_BUFF_SIZE);
    if(xMeasurementsCSVQueue == NULL)
    {
        ESP_LOGE(TAG, "xMeasurementsCSVQueue could not be created");
        return;
    }
    xTaskCreate(tcp_server_task, "tcp_server", 4096, (void *)xMeasurementsCSVQueue, 1, NULL);
    QueueHandle_t pvParameters[] = {xMeasurementsQueue, xMeasurementsCSVQueue}; 
    xTaskCreatePinnedToCore(measurements_task, "measurements", 2048, (void *)pvParameters, 2, NULL, 0);
    xTaskCreatePinnedToCore(output_compute_task, "output_compute", 2048, (void *)xMeasurementsQueue, 2, NULL, 1);
}