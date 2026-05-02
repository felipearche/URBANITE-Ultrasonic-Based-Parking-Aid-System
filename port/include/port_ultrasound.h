/**
 * @file port_ultrasound.h
 * @brief Header for the portable functions to interact with the HW of the ultrasound sensors. The functions must be implemented in the platform-specific code.
 * @author Felipe Fernández-Arche
 * @author David Pedraza
 * @date 26/03/2025
 */
#ifndef PORT_ULTRASOUND_H_
#define PORT_ULTRASOUND_H_

/* Includes ------------------------------------------------------------------*/
/* Standard C includes */
#include <stdint.h>
#include <stdbool.h>

/* Defines and enums ----------------------------------------------------------*/
#define PORT_REAR_PARKING_SENSOR_ID 0 /*!< Rear parking sensor identifier */
#define PORT_PARKING_SENSOR_TRIGGER_UP_US 10 /*!< Duration in microseconds of the trigger signal */
#define PORT_PARKING_SENSOR_TIMEOUT_MS 100 /*!< Time in ms to wait for the next measurement */
#define SPEED_OF_SOUND_MS 343 /*!< Speed of sound in air in m/s */

/* Function prototypes and explanation -------------------------------------------------*/
/**
 * @brief Configure the HW specifications of a given ultrasound sensor.
 * 
 * @param ultrasound_id Ultrasound ID. This index is used to select the element of the ultrasound_arr[] array
 */
void port_ultrasound_init (uint32_t ultrasound_id);

/**
 * @brief Start a new measurement of the ultrasound sensor.
 * 
 * This function prepares the timer of the trigger and the timer of the echo signal to start a new measurement. It also enables the timer that controls the new measurement.
 * 
 * @param ultrasound_id Ultrasound ID. This index is used to select the element of the ultrasound_arr[] array
 */
void port_ultrasound_start_measurement (uint32_t ultrasound_id);

 /**
  * @brief Stop the timer that controls the trigger signal.
  * 
  * This function stops the timer that controls the trigger signal because the time to trigger the ultrasound sensor has finished. It also sets the trigger signal to low.
  * 
  * @param ultrasound_id Ultrasound ID. This index is used to select the element of the ultrasound_arr[] array
  */
void port_ultrasound_stop_trigger_timer (uint32_t ultrasound_id);

/**
 * @brief Stop the timer that controls the echo signal.
 * 
 * This function stops the timer that controls the echo signal because the echo signal has been received.
 * 
 * @param ultrasound_id Ultrasound ID. This index is used to select the element of the ultrasound_arr[] array
 */
void port_ultrasound_stop_echo_timer (uint32_t ultrasound_id);

/**
 * @brief Start the timer that controls the new measurement.
 * 
 * This function starts the timer that controls the new measurement.
 * 
 */
void port_ultrasound_start_new_measurement_timer (void);

/**
 * @brief Stop the timer that controls the new measurement.
 * 
 * This function stops the timer that controls the new measurement.
 * 
 */
void port_ultrasound_stop_new_measurement_timer (void);

/**
 * @brief Reset the time ticks of the echo signal
 * 
 * This function resets the time ticks of the echo signal once the distance has been calculated.
 * 
 * @param ultrasound_id Ultrasound ID. This index is used to select the element of the ultrasound_arr[] array
 */
void port_ultrasound_reset_echo_ticks (uint32_t ultrasound_id);

/**
 * @brief Stop all the timers of the ultrasound sensor and reset the echo ticks.
 * 
 * This function calls the functions to stop the trigger and echo timers and to reset the echo ticks.
 * 
 * @param ultrasound_id Ultrasound ID. This index is used to select the element of the ultrasound_arr[] array
 */
void port_ultrasound_stop_ultrasound (uint32_t ultrasound_id);

/**
 * @brief Get the readiness of the trigger signal.
 * 
 * This function returns the status of readiness the trigger signal. If it is true, the ultrasound sensor is ready to start a new measurement.
 * 
 * @param ultrasound_id Ultrasound ID. This index is used to select the element of the ultrasound_arr[] array
 * @return true 
 * @return false 
 */
bool port_ultrasound_get_trigger_ready (uint32_t ultrasound_id);

/**
 * @brief Set the readiness of the trigger signal.
 * 
 * This function sets the status of readiness of the trigger signal. If it is true, the ultrasound sensor will ready to start a new measurement.
 * 
 * @param ultrasound_id Ultrasound ID. This index is used to select the element of the ultrasound_arr[] array
 * @param trigger_ready Status of the trigger signal.
 */
void port_ultrasound_set_trigger_ready (uint32_t ultrasound_id, bool trigger_ready);

/**
 * @brief Get the status of the trigger signal.
 * 
 * This function returns the status of the trigger signal. It will be true if the time to trigger the ultrasound sensor has finished.
 * 
 * @param ultrasound_id Ultrasound ID. This index is used to select the element of the ultrasound_arr[] array
 * @return true 
 * @return false 
 */
bool port_ultrasound_get_trigger_end (uint32_t ultrasound_id);

/**
 * @brief Set the status of the trigger signal.
 * 
 * This function sets the status of the trigger signal. It will be true to indicate that the time to trigger the ultrasound sensor has finished and the trigger signal is low.
 * 
 * @param ultrasound_id Ultrasound ID. This index is used to select the element of the ultrasound_arr[] array
 * @param trigger_end Status of the trigger signal.
 */
void port_ultrasound_set_trigger_end (uint32_t ultrasound_id, bool trigger_end);

/**
 * @brief Get the time tick when the init of echo signal was received.
 * 
 * @param ultrasound_id 	Ultrasound ID. This index is used to select the element of the ultrasound_arr[] array
 * @return uint32_t 
 */
uint32_t port_ultrasound_get_echo_init_tick (uint32_t ultrasound_id);

/**
 * @brief Set the time tick when the init of echo signal was received.
 * 
 * This function sets the time tick when the init of echo signal was received. It is called by the ISR of the input capture of the echo signal.
 * 
 * @param ultrasound_id Ultrasound ID. This index is used to select the element of the ultrasound_arr[] array
 * @param echo_init_tick Time tick when the init of echo signal was received.
 */
void port_ultrasound_set_echo_init_tick (uint32_t ultrasound_id, uint32_t echo_init_tick);

/**
 * @brief Get the time tick when the end of echo signal was received.
 * 
 * @param ultrasound_id Ultrasound ID. This index is used to select the element of the ultrasound_arr[] array
 * @return uint32_t 
 */
uint32_t port_ultrasound_get_echo_end_tick (uint32_t ultrasound_id);

/**
 * @brief Set the time tick when the end of echo signal was received.
 * 
 * This function sets the time tick when the end of echo signal was received. It is called by the ISR of the input capture of the echo signal.
 * 
 * @param ultrasound_id Ultrasound ID. This index is used to select the element of the ultrasound_arr[] array
 * @param echo_end_tick Time tick when the end of echo signal was received.
 */
void port_ultrasound_set_echo_end_tick (uint32_t ultrasound_id, uint32_t echo_end_tick);

/**
 * @brief Get the status of the echo signal.
 * 
 * This function returns the status of the echo signal. It will be true if the echo signal has been received (both the init and end ticks).
 * 
 * @param ultrasound_id Ultrasound ID. This index is used to select the element of the ultrasound_arr[] array
 * @return true 
 * @return false 
 */
bool port_ultrasound_get_echo_received (uint32_t ultrasound_id);

/**
 * @brief Set the status of the echo signal.
 * 
 * This function sets the status of the echo signal. The ISR of the input capture of the echo signal calls this function to set the status of the echo signal when both the init and end ticks have been received.
 * 
 * @param ultrasound_id Ultrasound ID. This index is used to select the element of the ultrasound_arr[] array
 * @param echo_received Status of the echo signal.
 */
void port_ultrasound_set_echo_received (uint32_t ultrasound_id, bool echo_received);

/**
 * @brief Get the number of overflows of the echo signal timer.
 * 
 * This function returns the number of overflows of the echo signal timer. It is used to calculate the real time elapsed in the echo signal.
 * 
 * @param ultrasound_id Ultrasound ID. This index is used to select the element of the ultrasound_arr[] array
 * @return uint32_t 
 */
uint32_t port_ultrasound_get_echo_overflows (uint32_t ultrasound_id);

/**
 * @brief Set the number of overflows of the echo signal timer.
 * 
 * This function sets the number of overflows of the echo signal timer. It is called by the ISR of the input capture of the echo signal when an overflow occurs to increment the number of overflows.
 * 
 * @param ultrasound_id Ultrasound ID. This index is used to select the element of the ultrasound_arr[] array
 * @param echo_overflows Number of overflows of the echo signal timer.
 */
void port_ultrasound_set_echo_overflows (uint32_t ultrasound_id, uint32_t echo_overflows);

#endif /* PORT_ULTRASOUND_H_ */