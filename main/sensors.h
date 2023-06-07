/** @file
 * 
 * @brief Macros and functions to configure and use peripherals. 
 * 
 * @details This header contains useful macros and functions for setting up peripherals,
 * reading sensors and controlling the speed controller of the rc car via a pwm signal.
 * 
 * @defgroup GPIOPins
 * 
 * @brief Define GPIO pins
 * 
 * @{
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
 * 
 * @brief Define sample rate of tachometer and HC­SR04 distance sensor.
 * 
 * @{
*/

/** @def VELO_MEAS_PERIOD_MS
 * period of rotational velocity calculation from tachometer data [ms]
*/
#define VELO_MEAS_PERIOD_MS 200
/** @def DISTANCE_MEAS_PERIOD_MS
 * sample rate of distance measurements [ms]
*/
#define DISTANCE_MEAS_PERIOD_MS 100

/**@}*/

/** @defgroup HardwareSpecs
 * 
 * @brief Define hardware specifications
 * 
 * @{
*/

/** @def ROT_VEL_MAX
 * rotational velocity limit for tachometer [rot/min]
*/
#define ROT_VEL_MAX 100.0
/** @def TACHO_COUNTS_PER_ROTATION
 * tachometer resolution [counts/rotation]
*/
#define TACHO_COUNTS_PER_ROTATION 8
/** @def PWM_FREQ
 * pwm frequency of the rc car speed controller [Hz]
*/
#define PWM_FREQ 74
/** @def THROTTLE_STATIONARY_DUTY
 * speed controller duty cycle when motor is stationary [%]
*/
#define THROTTLE_STATIONARY_DUTY 11.4

/**@}*/

/**
 * @brief Configure peripherals
 * 
 * @details 
 */
void sensors_init();

/**
 * @brief Get last tachometer measurement taken on #TACHOMETER_GPIO.
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
 * @brief Get last distance measured on #HC_SR04_ECHO_GPIO.
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

