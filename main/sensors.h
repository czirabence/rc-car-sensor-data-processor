/** @file
 * @brief Macros and functions to configure and use peripherals  
 * 
 * @details This header contains useful macros and functions for setting up peripherals,
 * reading sensors and controlling the speed controller of the rc car via a pwm signal 
 * 
 * @defgroup Macros Macros
 * @brief Macros
 * 
 * @defgroup GPIOPins Macros
 * @brief Define GPIO pins
 * @c THROTTLE_OUT_GPIO: pwm throttle command output 
 * 
*/

/** @def THROTTLE_OUT_GPIO
 * pwm throttle command output*/
#define THROTTLE_OUT_GPIO 33
/** @def THROTTLE_IN_GPIO
 * pwm throttle command input*/
#define THROTTLE_IN_GPIO 34
/** @def TACHOMETER_GPIO
 * tachometer signal 
*/
#define TACHOMETER_GPIO 35
/** @def HC_SR04_TRIG_GPIO
 * ultrasonic sensor trigger signal
*/
#define HC_SR04_TRIG_GPIO 16
/** @def HC_SR04_ECHO_GPIO
 * ultrasonic sensor input
*/
#define HC_SR04_ECHO_GPIO 17
/**@}*/

/** @defgroup MeasurementPeriods
 * @brief Define sample rate of tachometer and HC­SR04 distance sensor
 * @{
*/

/** @def VELO_MEAS_PERIOD_MS
 * period of rotational velocity calculation from tachometer data
*/
#define VELO_MEAS_PERIOD_MS 200
/** @def DISTANCE_MEAS_PERIOD_MS
 * sample rate of distance measurements
*/
#define DISTANCE_MEAS_PERIOD_MS 100
/**@}*/

/** @defgroup HardwareSpecs
 * @brief Define hardware specifications
 * @{
*/
#define ROT_VEL_MAX 100.0              //[rot/s] max should never be reached
#define TACHO_COUNTS_PER_ROTATION 8
#define PWM_FREQ 74                    //Hz
#define THROTTLE_STANDSTILL_DUTY 11.4  //percent
/**@}*/

/**
 * @brief Configure peripherals for sensors and actuators 
 * 
 * @details 
 */
void sensors_init();

/**
 * @brief Get last tachometer measurement taken on #TACHOMETER_GPIO
 * 
 * @return rotational velocity [rot/s] 
 */
float get_velocity(void);

/**
 * @brief Get duty cycle measured on #THROTTLE_IN_GPIO
 * 
 * @return duty cycle in percentage [%]
 */
float get_throttle_in_duty(void);

/**
 * @brief Get last distance measured on #HC_SR04_ECHO_GPIO
 * 
 * @return distance from nearest object [m] 
 */
float get_distance(void);

/**
 * @brief Set duty cycle for #THROTTLE_OUT_GPIO
 * 
 * @param duty - pwm duty cycle in percentage [%] 
 */
void set_throttle_duty(float duty);

