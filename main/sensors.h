#include <inttypes.h>

/** @file sensors.h
 * @author Czira Bence (czirabence@gmail.com)
 * 
 * @brief Macros and functions for sensors and actuators. 
 * 
 * @version 0.1
 * @date 2023-06-12
 * 
 * @details This header contains the macros and functions for setting up peripherals,
 * reading sensors and controlling the speed controller of the rc car. Peripherals were 
 * configured using the esp-idf driver modules: periodic tasks were scheduled with ESP Timer,
 * PWM duty cycles are read with the capture timer functionality of the MCPWM module, input
 * pulses are counted using the pulse counter (PCNT) module and PWM signals are generated with
 * the LEDC module. Spinlocks are used to ensure interrupt protection of variables holding measurement values.
 * 
 * - <b> Tachometer </b> pulses on #TACHOMETER_GPIO are counted using the PCNT module. Periodic reading of the counter is triggered
 * in every #DISTANCE_MEAS_PERIOD_MS by an esp_timer. Both the rising and falling edges
 * of the tachometer signal are counted, which sum up to #TACHO_COUNTS_PER_REVOLUTION.
 * @note Rotational velocity can be obtained with #get_velocity().
 * 
 * - <b> Throttle command output </b> on #THROTTLE_OUT_GPIO is implemented using the
 * LEDC module. The PWM frequency is defined by #PWM_FREQ. The rc car goes
 * in the forward and reverse direction for a duty cycle greater and smaller than
 * #THROTTLE_STATIONARY_DUTY respectively.
 * @note Throttle output duty cycle can be set via #set_throttle_duty().
 * 
 * - The<b> throttle command input</b> is normally sent by the rc receiver to the speed 
 * controller. In the scope of this project, the esp32 microcontroller is placed between
 * these two components in the control loop in order to execute Advanced Driver-Assistance
 * tasks. The rc receiver output is connected to #THROTTLE_IN_GPIO, and its duty cycle is
 * measured with a capture timer from the MCPWM module.
 * @note Get throttle input duty cycle with #get_throttle_in_duty().
 * 
 * - <b> Ultrasonic distance measurements </b> are taken with an HC-SR04 sensor placed on
 * the front of the car. In every #DISTANCE_MEAS_PERIOD_MS, a trigger signal is sent to the sensor on #HC_SR04_TRIG_GPIO,
 * then the time of flight (tof) of echoed ultrasound is measured on #HC_SR04_ECHO_GPIO. The trigger signal is scheduled by
 * an esp_timer, while tof is measured with a capture timer. 
 * @note Retreive distance measurements with #get_distance().
 * @note HC-SR04 distance measurement was implemented according to the <b> mcpwm_capture_hc_sr04 </b> project
 * from esp-idf builtin examples. 
 */
/** @name GPIO pins
 * @{
*/
/** @def THROTTLE_OUT_GPIO
 * @brief pwm throttle command output
 */
#define THROTTLE_OUT_GPIO 33
/** @def THROTTLE_IN_GPIO
 * @brief pwm throttle command input
 */
#define THROTTLE_IN_GPIO 34
/** @def TACHOMETER_GPIO
 * @brief tachometer signal 
*/
#define TACHOMETER_GPIO 35
/** @def HC_SR04_TRIG_GPIO
 * @brief ultrasonic sensor trigger pin
*/
#define HC_SR04_TRIG_GPIO 16
/** @def HC_SR04_ECHO_GPIO
 * @brief ultrasonic sensor echo pin
*/
#define HC_SR04_ECHO_GPIO 17
/**@}*/
/**
 * @name Sample rates.
 * 
 * @{
*/
/** @def VELO_MEAS_PERIOD_MS
 * @brief sample rate for reading cumulated counts on tachometer [ms]
*/
#define VELO_MEAS_PERIOD_MS 200
/** @def DISTANCE_MEAS_PERIOD_MS
 * @brief sample rate of distance measurements [ms]
*/
#define DISTANCE_MEAS_PERIOD_MS 100
/**@}*/

/**
 * @name Hardware specifications
 * 
 * @{
*/
/** @def ROT_VEL_MAX
 * @brief rotational velocity limit for tachometer [rot/min]
*/
#define ROT_VEL_MAX 100.0
/** @def TACHO_COUNTS_PER_REVOLUTION
 * @brief tachometer resolution [counts/revolution]
*/
#define TACHO_COUNTS_PER_REVOLUTION 8
/** @def PWM_FREQ
 * @brief pwm frequency of speed controller [Hz]
*/
#define PWM_FREQ 74
/** @def THROTTLE_STATIONARY_DUTY
 * @brief speed controller duty cycle when motor is stopped [%]
*/
#define THROTTLE_STATIONARY_DUTY 11.258452
/**@}*/
/**
 * @brief Hold measurement values and a timestamp.
 * 
 */
typedef struct measurement_data{
    uint64_t time_us;
    float rot_velocity;
    float throttle_in_duty;
    float distance;
} measurement_data;

/**
 * @brief Configure peripherals for sensors and actuators
 */
void sensors_init(void);

/**
 * @brief Get last tachometer measurement taken on #TACHOMETER_GPIO.
 * 
 * @details Compute rotational velocity from tachometer data with the following formula:
 * \f{equation}{rotational velocity = \frac{tachometer\_counts * 60000}{TACHO\_COUNTS\_PER\_REVOLUTION * VELO\_MEAS\_PERIOD\_MS}\f}
 * 
 * @return rotational velocity [rot/min] 
 */
float get_velocity(void);

/**
 * @brief Get duty cycle measured on #THROTTLE_IN_GPIO.
 * 
 * @return duty cycle in percentage [%]
 */
float get_throttle_in_duty(void);

/**
 * @brief Compute distance from HC-SR04 measurements. Time of flight of the reflected ultrasound is halfed 
 * and multiplied by speed of sound (343 m/s) in order to get distance in meters. 
 *  \f{equation}{distance = \frac{time\_of\_flight\_in\_ticks * 343}{2 * timer\_clock\_frequency\_Hz}\f}
 * 
 * @return distance from nearest object [m] 
 */
float get_distance(void);

/**
 * @brief Set duty cycle for #THROTTLE_OUT_GPIO.
 * 
 * @param duty - pwm duty cycle in percentage [%] 
 */
void set_throttle_duty(float duty);
/**
 * @brief get a #measurement_data instance with current measurements and a timestamp in microseconds.
 * Time is retrieved via esp_timer::esp_timer_get_time(), measured since boot time.
 * 
 * @return measurement data 
 */
measurement_data get_measurements(void);
/**
 * @brief Convert measurement data into csv format, write result into <b>buffer</b>.
 * 
 * @param buffer - char array destination
 * @param data - a measurement_data struct
 */
void measurements_to_csv(char *buffer, measurement_data data);
