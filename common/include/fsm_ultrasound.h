/**
 * @file fsm_ultrasound.h
 * @brief Header for fsm_ultrasound.c file.
 * @author Felipe Fernández-Arche
 * @author David Pedraza
 * @date 25/03/2025
 */

 #ifndef FSM_ULTRASOUND_H_
 #define FSM_ULTRASOUND_H_
 
 /* Includes ------------------------------------------------------------------*/
 #include "fsm.h"
 
 /* Standard C includes */
 #include <stdint.h>
 #include <stdbool.h>
 
 /* Defines and enums ----------------------------------------------------------*/
 #define FSM_ULTRASOUND_NUM_MEASUREMENTS 5 /**<! Number of measurements to store in the array */
 
 /**
  * @brief Enumerator for the ultrasound finite state machine.
  * 
  * This enumerator defines the different states that the ultrasound finite state machine can be in. Each state represents a specific condition or step in the ultrasound distance measurement process.
  * 
  */
 enum FSM_ULTRASOUND {
     WAIT_START = 0, /*!< Starting state. Also comes here when the distance measurement has been completed or a timeout has occurred */
     TRIGGER_START, /*!< State to send the trigger pulse to the ultrasound sensor */
     WAIT_ECHO_START, /*!< State to wait for the echo signal */
     WAIT_ECHO_END, /*!< State to wait for the echo signal */
     SET_DISTANCE /*!< State to compute the distance from the echo signal */
 };
 
 /* Typedefs --------------------------------------------------------------------*/
 typedef struct fsm_ultrasound_t fsm_ultrasound_t;
 
 /* Function prototypes and explanation -------------------------------------------------*/
 /**
  * @brief Create a new ultrasound FSM.
  * 
  * This function creates a new ultrasound transceiver FSM with the given ultrasound ID.
  * 
  * @param ultrasound_id Ultrasound ID. Must be unique.
  * @return fsm_ultrasound_t* Pointer to the ultrasound FSM.
  */
 fsm_ultrasound_t * fsm_ultrasound_new (uint32_t ultrasound_id);
 
 /**
  * @brief Destroy an ultrasound FSM.
  * 
  * This function destroys an ultrasound transceiver FSM and frees the memory.
  * 
  * @param p_fsm Pointer to an `fsm_ultrasound_t` struct.
  */
 void fsm_ultrasound_destroy (fsm_ultrasound_t *p_fsm);
 
 /**
  * @brief Return the distance of the last object detected by the ultrasound sensor.a64l.
  * 
  * The function also resets the field new_measurement to indicate that the distance has been read.
  * 
  * @param p_fsm Pointer to an `fsm_ultrasound_t` struct.
  * @return uint32_t Distance measured by the ultrasound sensor in centimeters.
  */
 uint32_t fsm_ultrasound_get_distance (fsm_ultrasound_t *p_fsm);
 
 /**
  * @brief Fire the ultrasound FSM.
  * 
  * This function is used to fire the ultrasound FSM. It is used to check the transitions and execute the actions of the ultrasound FSM.
  * 
  * @param p_fsm Pointer to an `fsm_ultrasound_t` struct.
  */
 void fsm_ultrasound_fire (fsm_ultrasound_t *p_fsm);
 
 /**
  * @brief Get the status of the ultrasound transceiver FSM.
  * 
  * This function returns the status of the ultrasound. This function might be used for testing and debugging purposes.
  * 
  * @param p_fsm Pointer to an `fsm_ultrasound_t` struct.
  * @return true If the ultrasound system has been indicated to be active.
  * @return false If the ultrasound system has been indicated to be paused.
  */
 bool fsm_ultrasound_get_status (fsm_ultrasound_t *p_fsm);
 
 /**
  * @brief Set the status of the ultrasound sensor. true means that the ultrasound sensor is active and a distance measurement must be performed. false means that the ultrasound sensor is inactive.
  * 
  * @param p_fsm Pointer to an `fsm_ultrasound_t` struct.
  * @param status Status of the ultrasound sensor.
  */
 void fsm_ultrasound_set_status (fsm_ultrasound_t *p_fsm, bool status);
 
 /**
  * @brief Get the ready flag of the trigger signal in the ultrasound HW.
  * 
  * This function returns the ready flag of trigger signal in the ultrasound HW. This function might be used for testing and debugging purposes.
  * 
  * @param p_fsm Pointer to an `fsm_ultrasound_t` struct.
  * @return true If the port indicates that the trigger signal is ready to start a new measurement.
  * @return false If the port indicates that the trigger signal is not ready to start a new measurement.
  */
 bool fsm_ultrasound_get_ready (fsm_ultrasound_t *p_fsm);
 
 /**
  * @brief Return the flag that indicates if a new measurement is ready.
  * 
  * @param p_fsm Pointer to the ultrasound FSM.
  * @return true 
  * @return false 
  */
 bool fsm_ultrasound_get_new_measurement_ready (fsm_ultrasound_t *p_fsm);
 
 /**
  * @brief Stop the ultrasound sensor.
  * 
  * This function stops the ultrasound sensor by indicating to the port to stop the ultrasound sensor (to reset all timer ticks) and to set the status of the ultrasound sensor to inactive.
  * 
  * @param p_fsm Pointer to an `fsm_ultrasound_t` struct.
  */
 void fsm_ultrasound_stop (fsm_ultrasound_t *p_fsm);
 
 /**
  * @brief Start the ultrasound sensor.
  * 
  * This function starts the ultrasound sensor by indicating to the port to start the ultrasound sensor (to reset all timer ticks) and to set the status of the ultrasound sensor to active.
  * 
  * @param p_fsm Pointer to an `fsm_ultrasound_t` struct.
  */
 void fsm_ultrasound_start (fsm_ultrasound_t *p_fsm);
 
 /**
  * @brief Get the inner FSM of the ultrasound.
  * 
  * This function returns the inner FSM of the ultrasound.
  * 
  * @param p_fsm Pointer to an `fsm_ultrasound_t` struct.
  * @return fsm_t* Pointer to the inner FSM.
  */
 fsm_t* fsm_ultrasound_get_inner_fsm (fsm_ultrasound_t *p_fsm);
 
 /**
  * @brief Get the state of the ultrasound FSM.
  * 
  * This function returns the current state of the ultrasound FSM.
  * 
  * @param p_fsm Pointer to an `fsm_ultrasound_t` struct.
  * @return uint32_t Current state of the ultrasound FSM.
  */
 uint32_t fsm_ultrasound_get_state (fsm_ultrasound_t *p_fsm);
 
 /**
  * @brief Set the state of the ultrasound FSM.
  * 
  * This function sets the current state of the ultrasound FSM.
  * 
  * @param p_fsm Pointer to an `fsm_ultrasound_t` struct.
  * @param state New state of the ultrasound FSM.
  */
 void fsm_ultrasound_set_state (fsm_ultrasound_t *p_fsm, int8_t state);
 
 /**
  * @brief Check if the ultrasound sensor is doing a distance measurement.
  * 
  * The ultrasound sensor is always inactive because all the transitions are due to HW interrupts.
  * 
  * @param p_fsm Pointer to an `fsm_ultrasound_t` struct.
  * @return true 
  * @return false 
  */
 bool fsm_ultrasound_check_activity (fsm_ultrasound_t *p_fsm);
 
 #endif /* FSM_ULTRASOUND_H_ */