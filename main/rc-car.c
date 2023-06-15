#include "wifi_station.h"
#include "tcp_server.h"
#include "sensors.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"

static const char *TAG = "rc-car-main";

const char *csv_header = "rot_velocity, throt_in_duty, throt_out_duty, distance\n";
char tx_buff[51];

void control_loop_task(void *pvParameters)
{
    while(true)
    {
        measurement_data data = get_measurements();
        measurements_to_csv(tx_buff, data);
        set_throttle_duty(data.throttle_in_duty);
        vTaskDelay(50/portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    sensors_init();
    nvs_init();
    wifi_init_sta();
    xTaskCreate(control_loop_task, "control loop", 2048, NULL, 6, NULL);
    xTaskCreate(tcp_server_task, "tcp_server", 4096, (void*)tx_buff, 5, NULL);
}