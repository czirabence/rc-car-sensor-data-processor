#include "sensors.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/pulse_cnt.h"
#include "driver/ledc.h"
#include "driver/mcpwm_cap.h"
#include "driver/mcpwm_sync.h"
#include "esp_timer.h"
#include "esp_private/esp_clk.h"
#include "driver/i2c.h"
#include "mpu6050.h"

volatile int tachometer_count = 0;
volatile uint32_t throttle_in_duty_ticks = 0;
volatile uint32_t echo_tof_ticks = 0;
//static mpu6050_handle_t mpu6050_dev = NULL;

static portMUX_TYPE tachometer_spinlock = portMUX_INITIALIZER_UNLOCKED;
static portMUX_TYPE throttle_in_spinlock = portMUX_INITIALIZER_UNLOCKED;
static portMUX_TYPE echo_spinlock = portMUX_INITIALIZER_UNLOCKED; 

void tachometer_callback(void *arg)
{
    pcnt_unit_handle_t pcnt_handle = (pcnt_unit_handle_t)(arg);
    taskENTER_CRITICAL_ISR(&tachometer_spinlock);
    ESP_ERROR_CHECK(pcnt_unit_get_count(pcnt_handle, &tachometer_count));
    taskEXIT_CRITICAL_ISR(&tachometer_spinlock);
    ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_handle));
}

void tachometer_setup(void)
{
    pcnt_unit_config_t unit_config = {
        .high_limit = ROT_VEL_MAX * TACHO_COUNTS_PER_ROTATION * VELO_MEAS_PERIOD_MS/1E3,
        .low_limit = -1,
        .flags.accum_count = false,
    };
    pcnt_glitch_filter_config_t filter_config = {
            .max_glitch_ns = 10000,
    };
    pcnt_chan_config_t channel_config = {
        .edge_gpio_num = TACHOMETER_GPIO,
        .level_gpio_num = -1,
        .flags.invert_edge_input = false,
        .flags.io_loop_back = false,
    };
    pcnt_unit_handle_t unit_handle = NULL;
    pcnt_channel_handle_t channel_handle = NULL;
    ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &unit_handle));
    ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(unit_handle, &filter_config));
    ESP_ERROR_CHECK(pcnt_new_channel(unit_handle, &channel_config, &channel_handle));
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(channel_handle, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE));
    ESP_ERROR_CHECK(pcnt_unit_enable(unit_handle));
    ESP_ERROR_CHECK(pcnt_unit_start(unit_handle));
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = tachometer_callback,
        .arg = unit_handle,
        .name = "tacho_callback"
    };
    esp_timer_handle_t tacho_periodic_handle = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &tacho_periodic_handle));
    ESP_ERROR_CHECK(esp_timer_start_periodic(tacho_periodic_handle, VELO_MEAS_PERIOD_MS * 1E3));
}

mcpwm_cap_timer_handle_t capture_timer_setup(int group_id)
{
    mcpwm_capture_timer_config_t timer_config = {
        .group_id = group_id,
        .clk_src = MCPWM_CAPTURE_CLK_SRC_DEFAULT,
    };
    mcpwm_cap_timer_handle_t timer_handle = NULL;
    ESP_ERROR_CHECK(mcpwm_new_capture_timer(&timer_config, &timer_handle));
    return timer_handle;
}

    bool throttle_in_callback(mcpwm_cap_channel_handle_t cap_chan, const mcpwm_capture_event_data_t *edata, void *user_ctx)
{
    static uint32_t pwm_pos_edge_ticks = 0;
    if(edata->cap_edge == MCPWM_CAP_EDGE_POS)
    {
        pwm_pos_edge_ticks = edata->cap_value;
    }
    else // MCPWM_CAP_EDGE_NEG
    {
        taskENTER_CRITICAL_ISR(&throttle_in_spinlock);
        throttle_in_duty_ticks = edata->cap_value - pwm_pos_edge_ticks;
        taskEXIT_CRITICAL_ISR(&throttle_in_spinlock);
    }
    return true;
}

void throttle_in_setup(mcpwm_cap_timer_handle_t timer_handle)
{
    mcpwm_capture_channel_config_t channel_config = {
        .gpio_num = THROTTLE_IN_GPIO,
        .prescale = 1,
        .flags.pos_edge = true,
        .flags.neg_edge = true,
        .flags.pull_up = false,
        .flags.pull_down = false,
        .flags.invert_cap_signal = false,
        .flags.io_loop_back = false,
    };
    mcpwm_capture_event_callbacks_t event_callbacks = {
        .on_cap = throttle_in_callback,
    };
    mcpwm_cap_channel_handle_t channel_handle = NULL;
    ESP_ERROR_CHECK(mcpwm_new_capture_channel(timer_handle, &channel_config, &channel_handle));
    ESP_ERROR_CHECK(mcpwm_capture_channel_register_event_callbacks(channel_handle, &event_callbacks, NULL));
    ESP_ERROR_CHECK(mcpwm_capture_channel_enable(channel_handle));
}

//ultrasonic distance sensor helper functions
//taken from esp-idf/examples/peripherals/mcpwm/mcpwm_capture_hc_sr04
bool hc_sr04_echo_callback(mcpwm_cap_channel_handle_t cap_chan, const mcpwm_capture_event_data_t *edata, void *user_data)
{
    static uint32_t cap_val_pos_edge = 0;
    if (edata->cap_edge == MCPWM_CAP_EDGE_POS) 
    {
        cap_val_pos_edge = edata->cap_value;
    }
    else 
    {
        taskENTER_CRITICAL_ISR(&echo_spinlock);
        echo_tof_ticks = edata->cap_value - cap_val_pos_edge;
        taskEXIT_CRITICAL_ISR(&echo_spinlock);
    }
    return true;
}

void echo_trigger(void *arg)
{
    gpio_set_level(HC_SR04_TRIG_GPIO, 1); // set high
    esp_rom_delay_us(10);
    gpio_set_level(HC_SR04_TRIG_GPIO, 0); // set low
}

void distance_sensor_setup(mcpwm_cap_timer_handle_t timer_handle)
{
    mcpwm_capture_channel_config_t channel_config = {
        .gpio_num = HC_SR04_ECHO_GPIO,
        .prescale = 1,
        // capture on both edge
        .flags.neg_edge = true,
        .flags.pos_edge = true,
        // pull up internally
        .flags.pull_up = true,
    };
    mcpwm_capture_event_callbacks_t event_callbacks = {
        .on_cap = hc_sr04_echo_callback,
    };
    mcpwm_cap_channel_handle_t channel_handle = NULL;
    ESP_ERROR_CHECK(mcpwm_new_capture_channel(timer_handle, &channel_config, &channel_handle));
    ESP_ERROR_CHECK(mcpwm_capture_channel_register_event_callbacks(channel_handle, &event_callbacks, NULL));
    ESP_ERROR_CHECK(mcpwm_capture_channel_enable(channel_handle));
    //config trig pin
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << HC_SR04_TRIG_GPIO,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));
    // drive low by default
    ESP_ERROR_CHECK(gpio_set_level(HC_SR04_TRIG_GPIO, 0));
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = echo_trigger,
        .arg = NULL,
        .name = "ultrasound_callback"
    };
    esp_timer_handle_t periodic_handle = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_handle));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_handle, DISTANCE_MEAS_PERIOD_MS * 1E3));
}

void throttle_out_setup(void)
{
    ledc_timer_config_t timer_config = {
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = PWM_FREQ,
        .clk_cfg = LEDC_USE_REF_TICK,
    };
    ledc_channel_config_t channel_config = {
        .gpio_num = THROTTLE_OUT_GPIO,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = (uint32_t)(THROTTLE_STANDSTILL_DUTY * 1023),
        .hpoint = 0,
        .flags.output_invert = false,
    };
    ledc_timer_config(&timer_config);
    ledc_channel_config(&channel_config);
}

// void accelerometer_setup(void)
// {
//     i2c_config_t i2c_config = {
//         .mode = I2C_MODE_MASTER,
//         .sda_io_num = ,
//         .sda_pullup_en = GPIO_PULLUP_ENABLE,
//         .scl_io_num = ,
//         .scl_pullup_en = GPIO_PULLUP_ENABLE,
//         .master.clk_speed = I2C_CLK_FREQ_HZ,
//         .clk_flags = 0,
//     };
//     i2c_param_config(I2C_NUM_0, i2c_config);
//     ESP_ERROR_CHECK(i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, ));

// }
// void mpu6050_init()
// {
//     mpu6050_dev = mpu6050_create(BSP_I2C_NUM, MPU6050_I2C_ADDRESS);
//     mpu6050_config(mpu6050_dev, ACCE_FS_4G, GYRO_FS_500DPS);
//     mpu6050_wake_up(mpu6050_dev);
// }

void sensors_init(QueueHandle_t)
{
    mcpwm_cap_timer_handle_t timer_handle = capture_timer_setup(1);
    ESP_ERROR_CHECK(mcpwm_capture_timer_enable(timer_handle));
    ESP_ERROR_CHECK(mcpwm_capture_timer_start(timer_handle));
    distance_sensor_setup(timer_handle);
    throttle_in_setup(timer_handle);
    tachometer_setup();
    throttle_out_setup();
    //mpu6050_init();


}

float get_velocity(void)
{
    taskENTER_CRITICAL(&tachometer_spinlock);
    float rot_s = tachometer_count / TACHO_COUNTS_PER_ROTATION * 1E3/VELO_MEAS_PERIOD_MS;
    taskEXIT_CRITICAL(&tachometer_spinlock);
    return rot_s;
}
float get_throttle_in_duty(void)
{
    taskENTER_CRITICAL(&throttle_in_spinlock);
    uint32_t out = throttle_input_duty_ticks;
    taskEXIT_CRITICAL(&throttle_in_spinlock);
    return out * 100.0*PWM_FREQ / esp_clk_apb_freq();
}
float get_distance(void)
{
    taskENTER_CRITICAL(&echo_spinlock);
    uint32_t out = echo_tof_ticks;
    taskEXIT_CRITICAL(&echo_spinlock);
    return out * (343.0/2 / esp_clk_apb_freq());
}
uint32_t set_throttle_duty(float duty)
{
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, (uint32_t)(duty * 1023 / 100));
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
}