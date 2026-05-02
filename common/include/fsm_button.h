/**
 * @file fsm_button.h
 * @brief Header for fsm_button.c file.
 * @author Felipe Fernández-Arche
 * @author David Pedraza
 * @date 25/03/25
 */

#ifndef FSM_BUTTON_H_
#define FSM_BUTTON_H_

/* Includes ------------------------------------------------------------------*/

/* Standard C includes */
#include <stdint.h>
#include <stdbool.h>

/* Other includes */
#include "fsm.h"

/* Defines and enums ----------------------------------------------------------*/
/* Enums */
/**
 * @brief This enumerator defines the different states that the button finite state machine can be in. Each state represents a specific condition or step in the button press process.
 * 
 */
enum FSM_BUTTON {
    BUTTON_RELEASED = 0, /*!< Starting state. Also comes here when the button has been released */
    BUTTON_RELEASED_WAIT, /*!< State to perform the anti-debounce mechanism for a falling edge */
    BUTTON_PRESSED, /*!< State while the button is being pressed */
    BUTTON_PRESSED_WAIT /*!< State to perform the anti-debounce mechanism for a rising edge */
  };

/* Typedefs --------------------------------------------------------------------*/
typedef struct fsm_button_t fsm_button_t;

/* Function prototypes and explanation -------------------------------------------------*/
/**
 * @brief Create a new button FSM.
 * 
 * This function creates a new button FSM with the given debounce time and button ID.
 * 
 * @param debounce_time_ms Debounce time in milliseconds.
 * @param button_id Button ID. Must be unique.
 * @return fsm_button_t* Pointer to the button FSM.
 */
fsm_button_t * fsm_button_new (uint32_t debounce_time_ms, uint32_t button_id);

/**
 * @brief Destroy a button FSM.
 * 
 * This function destroys a button FSM and frees the memory.
 * 
 * @param p_fsm Pointer to an `fsm_button_t` struct.
 */
void fsm_button_destroy (fsm_button_t *p_fsm);

/**
 * @brief Fire the button FSM.
 * 
 * This function is used to fire the button FSM. It is used to check the transitions and execute the actions of the button FSM.
 * 
 * @param p_fsm Pointer to an `fsm_button_t` struct.
 */
void fsm_button_fire (fsm_button_t *p_fsm);

/**
 * @brief Get the inner FSM of the button.
 * 
 * This function returns the inner FSM of the button.
 * 
 * @param p_fsm Pointer to an `fsm_button_t` struct.
 * @return fsm_t* Pointer to the inner FSM.
 */
fsm_t * fsm_button_get_inner_fsm (fsm_button_t *p_fsm);

/**
 * @brief Get the state of the button FSM.
 * 
 * This function returns the current state of the button FSM.
 * 
 * @param p_fsm Pointer to an `fsm_button_t` struct.
 * @return uint32_t Current state of the button FSM.
 */
uint32_t fsm_button_get_state (fsm_button_t *p_fsm);

/**
 * @brief Return the duration of the last button press.
 * 
 * @param p_fsm Pointer to an `fsm_button_t` struct.
 * @return uint32_t Duration of the last button press in milliseconds.
 */
uint32_t fsm_button_get_duration (fsm_button_t *p_fsm);

/**
 * @brief Reset the duration of the last button press.
 * 
 * @param p_fsm Pointer to an `fsm_button_t` struct.
 */
void fsm_button_reset_duration (fsm_button_t *p_fsm);

/**
 * @brief Get the debounce time of the button FSM.
 * 
 * This function returns the debounce time of the button FSM.
 * 
 * @param p_fsm 	Pointer to an `fsm_button_t` struct.
 * @return uint32_t Debounce time in milliseconds.
 */
uint32_t fsm_button_get_debounce_time_ms (fsm_button_t *p_fsm);

/**
 * @brief Check if the button FSM is active, or not.
 * 
 * The button is inactive when it is in the status BUTTON_RELEASED.
 * 
 * @param p_fsm Pointer to an `fsm_button_t` struct.
 * @return true
 * @return false
 */
bool fsm_button_check_activity (fsm_button_t *p_fsm);

#endif /* FSM_BUTTON_H_*/