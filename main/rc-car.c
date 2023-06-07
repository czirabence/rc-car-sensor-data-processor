#include "tcp_server.h"
#include "sensors.h"
#include "tcp_server.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h" 
#include "esp_system.h"
#include "esp_log.h"
#include "sys/time.h"

static const char *TAG = "control_loop";

typedef struct data_structure{
    uint64_t time_us;
    float rot_velocity;
    float throttle_in_duty;
    float throttle_out_duty;
    float distance_measurement;
} data_structure;

data_structure data;

const char *csv_header = "rot_velocity, throt_in_duty, throt_out_duty, distance\n";
char csv_buff[100];

static void get_measurements()
{
    //get time in microseconds
    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    data.time_us = (int64_t)tv_now.tv_sec * 1000000L + (int64_t)tv_now.tv_usec;
    //get measurement values
    data.rot_velocity = get_velocity();
    data.throttle_in_duty = get_throttle_in_duty();
    data.distance_measurement = get_distance();
    //convert measurements into csv format
    sprintf(csv_buff, "%llu, %f, %f, %f, %f\n", data.time_us, data.rot_velocity, data.throttle_in_duty, data.throttle_out_duty, data.distance_measurement);
}

static void task_control_loop(void *vParams)
{
    for(;;)
    {
        get_measurements();
        set_throttle_duty(data.throttle_in_duty);
        ESP_LOGI(TAG, "%s", csv_buff);
        vTaskDelay(20/portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    esp_log_level_set(TAG, ESP_LOG_INFO);
    sensors_init();
    xTaskCreate(task_control_loop, "control loop", 2048, NULL, 5, NULL);
}

