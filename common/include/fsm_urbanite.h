/**
 * @file fsm_urbanite.h
 * @brief Header for fsm_urbanite.c file.
 * @author Felipe Fernández-Arche
 * @author alumno2
 * @date 10/04/2025
 */

#ifndef FSM_URBANITE_SYSTEM_H_
#define FSM_URBANITE_SYSTEM_H_

/* Includes ------------------------------------------------------------------*/
#include "fsm_button.h"
#include "fsm_display.h"
#include "fsm_ultrasound.h"

/* Standard C includes */
#include <stdint.h>

/* Defines and enums ----------------------------------------------------------*/
/**
 * @brief Enumerator for the Urbanite finite state machine.
 * 
 * This enumerator defines the different states that the Urbanite finite state machine can be in. Each state represents a specific condition or step in the process of showing the distance to the nearest obstacle.
 * 
 */
enum FSM_URBANITE { // DUDA No se que es eso de que se definan publicamente
   OFF = 0, /*!< Starting state. Also comes here when the button has been pressed for the required time to turn OFF the Urbanite */
   MEASURE, /*!< State to measure the distance to the obstacles */
   SLEEP_WHILE_OFF, /*!< State to start the low power mode while the Urbanite is OFF */
   SLEEP_WHILE_ON /*!< State to start the low power mode while the Urbanite is ON */
 };


/* Typedefs --------------------------------------------------------------------*/
typedef struct fsm_urbanite_t fsm_urbanite_t;

/* Function prototypes and explanation -------------------------------------------------*/
/**
 * @brief Create a new Urbanite FSM.
 * 
 * This function creates a new Urbanite FSM with the given button, ultrasound, display FSMs and the required times for configuration.
 * 
 * @param p_fsm_button Pointer to the button FSM to interact with the Urbanite.
 * @param on_off_press_time_ms Time in ms to consider ON/OFF of the Urbanite parking aid system.
 * @param pause_display_time_ms Time in ms to pause the display system.
 * @param p_fsm_ultrasound_rear Pointer to the rear ultrasound FSM.
 * @param p_fsm_display_rear Pointer to the rear display FSM.
 * @return fsm_urbanite_t* Pointer to the Urbanite FSM.
 */
fsm_urbanite_t * fsm_urbanite_new (fsm_button_t *p_fsm_button, uint32_t on_off_press_time_ms, uint32_t pause_display_time_ms, fsm_ultrasound_t *p_fsm_ultrasound_rear, fsm_display_t *p_fsm_display_rear);

/**
 * @brief Fire the Urbanite FSM.
 * 
 * This function is used to check the transitions and execute the actions of the Urbanite FSM.
 * 
 * @param p_fsm Pointer to the `fsm_urbanite_t` struct.
 */
void fsm_urbanite_fire (fsm_urbanite_t *p_fsm);

/**
 * @brief Destroy an Urbanite FSM.
 * 
 * This function destroys an Urbanite FSM and frees the memory.
 * 
 * @param p_fsm Pointer to an `fsm_urbanite_t` struct.
 */
void fsm_urbanite_destroy (fsm_urbanite_t *p_fsm);

#endif /* FSM_URBANITE_SYSTEM_H_ */