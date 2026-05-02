/**
 * @file fsm_button.c
 * @brief Button FSM main file.
 * @author Felipe Fernández-Arche
 * @author David Pedraza
 * @date 25/03/25
 */

/* Includes ------------------------------------------------------------------*/
/* Standard C includes */
#include <stdlib.h>

/* HW dependent includes */
#include "port_button.h"
#include "port_system.h"

/* Project includes */
#include "fsm_button.h"

/**
 * @brief Structure of the Button FSM.
 * 
 */
struct fsm_button_t
{
    fsm_t f; /*!< Button FSM */
    uint32_t debounce_time_ms; /*!< Button debounce time in ms */
    uint32_t next_timeout; /*!< Next timeout for the anti-debounce in ms */
    uint32_t tick_pressed; /*!< Number of ticks when the button was pressed */
    uint32_t duration; /*!< How much time the button has been pressed */
    uint32_t button_id; /*!< Button ID. Must be unique */
};

/* State machine input or transition functions */
/**
 * @brief Check if the button has been pressed.
 * 
 * @param p_this Pointer to an fsm_t struct than contains an `fsm_button_t`.
 * @return true 
 * @return false 
 */
static bool check_button_pressed (fsm_t *p_this) {
    // Cast the fsm_t pointer to fsm_button_t pointer to access button-specific data
    fsm_button_t *p_fsm_button = (fsm_button_t *)p_this;
    // 1. Call function `port_button_get_pressed()` and retrieve the status
    bool status = port_button_get_pressed(p_fsm_button->button_id);
    // 2. Return the status
    return status;
}

/**
 * @brief Check if the button has been released.
 * 
 * @param p_this Pointer to an fsm_t struct than contains an `fsm_button_t`.
 * @return true 
 * @return false 
 */
static bool check_button_released (fsm_t *p_this) {
    // Cast the fsm_t pointer to fsm_button_t pointer to access button-specific data
    fsm_button_t *p_fsm_button = (fsm_button_t *)p_this;
    // 1. Call function `port_button_get_pressed()` and retrieve the status
    bool status = port_button_get_pressed(p_fsm_button->button_id);
    // 2. Return the inverse of the status
    return !status;
}

/**
 * @brief Check if the debounce-time has passed.
 * 
 * @param p_this Pointer to an fsm_t struct than contains an `fsm_button_t`.
 * @return true 
 * @return false 
 */
static bool check_timeout (fsm_t *p_this) {
    // Cast the fsm_t pointer to fsm_button_t pointer to access button-specific data
    fsm_button_t *p_fsm_button = (fsm_button_t *)p_this;
    // 1. Call function `port_system_get_millis()` and retrieve the current system tick
    uint32_t current_tick = port_system_get_millis();
    // 2. Check if the current system tick is higher than the field next_timeout
    if (current_tick > p_fsm_button->next_timeout) {
        // 3. Return true if it is higher
        return true;
    } else {
        // Otherwise return false
        return false;
    }
}

/* State machine output or action functions */
/**
 * @brief Store the system tick when the button was pressed.
 * 
 * @param p_this Pointer to an fsm_t struct than contains an `fsm_button_t`.
 */
static void do_store_tick_pressed(fsm_t *p_this) {
    // Cast the fsm_t pointer to fsm_button_t pointer to access button-specific data
    fsm_button_t *p_fsm_button = (fsm_button_t *)p_this;
    // 1. Call function `port_system_get_millis()` and retrieve the current system tick
    uint32_t current_tick = port_system_get_millis();
    // 2. Store it in the field tick_pressed
    p_fsm_button->tick_pressed = current_tick;
    // 3. Update the field next_timeout considering the current tick and the field debounce_time_ms
    p_fsm_button->next_timeout = current_tick + p_fsm_button->debounce_time_ms;
}

/**
 * @brief Store the duration of the button press.
 * 
 * @param p_this Pointer to an fsm_t struct than contains an fsm_button_t.
 */
static void do_set_duration (fsm_t *p_this) {
    // Cast the fsm_t pointer to fsm_button_t pointer to access button-specific data
    fsm_button_t *p_fsm_button = (fsm_button_t *)p_this;
    // 1. Call function `port_system_get_millis()` and retrieve the current system tick
    uint32_t current_tick = port_system_get_millis();
    // 2. Update the field duration considering it and the field tick_pressed
    p_fsm_button->duration = current_tick - p_fsm_button->tick_pressed;
    // 3. Update the field next_timeout considering the current tick and the field debounce_time_ms
    p_fsm_button->next_timeout = current_tick + p_fsm_button->debounce_time_ms;
}

/**
 * @brief Array used to represent the transition table of the button.
 * 1. When check_button_pressed() returns true, the state will change from BUTTON_RELEASED to BUTTON_PRESSED_WAIT, and the function do_store_tick_pressed() will be called.
 * 2. When check_timeout() returns true, the state will change from BUTTON_PRESSED_WAIT to BUTTON_PRESSED.
 * 3. When check_button_released() returns true, the state will change from BUTTON_PRESSED to BUTTON_RELEASED_WAIT, and the function do_set_duration() will be called.
 * 4. When check_timeout() returns true, the state will change from BUTTON_RELEASED_WAIT to BUTTON_RELEASED.
 * 
 */
static fsm_trans_t fsm_trans_button [] = {
    { BUTTON_RELEASED, check_button_pressed, BUTTON_PRESSED_WAIT, do_store_tick_pressed },
    { BUTTON_PRESSED_WAIT, check_timeout, BUTTON_PRESSED, NULL },
    { BUTTON_PRESSED, check_button_released, BUTTON_RELEASED_WAIT, do_set_duration },
    { BUTTON_RELEASED_WAIT, check_timeout, BUTTON_RELEASED, NULL },
    { -1, NULL, -1, NULL },
};

/* Other auxiliary functions */
/**
 * @brief Initialize a button FSM.
 * 
 * This function initializes the default values of the FSM struct and calls to the port to initialize the associated HW given the ID.
 * 
 * This FSM implements an anti-debounce mechanism. Debounces (or very fast button presses) lasting less than the debounce_time_ms are filtered out.
 * 
 * The FSM stores the duration of the last button press. The user should ask for it using the function `fsm_button_get_duration()`.
 * 
 * At start and reset, the duration value must be 0 ms. A value of 0 ms means that there has not been a new button press.
 * 
 * @param p_fsm_button Pointer to the button FSM.
 * @param debounce_time Anti-debounce time in milliseconds
 * @param button_id Unique button identifier number
 */
void fsm_button_init(fsm_button_t *p_fsm_button, uint32_t debounce_time, uint32_t button_id)
{
    // 1. Call the fsm_init() to initialize the FSM. Pass the address of the fsm_t struct and the transition table
    fsm_init(&p_fsm_button->f, fsm_trans_button);
    // 2. Initialize the fields debounce_time_ms, button_id of p_fsm_button with the received values
    p_fsm_button->debounce_time_ms = debounce_time;
    p_fsm_button->button_id = button_id;
    // 3. Initialize the fields tick_pressed, duration of p_fsm_button with 0
    p_fsm_button->tick_pressed = 0;
    p_fsm_button->duration = 0;
    // 4. Call the function `port_button_init()`
    port_button_init(p_fsm_button->button_id);
}

/* Public functions -----------------------------------------------------------*/
fsm_button_t * fsm_button_new (uint32_t debounce_time, uint32_t button_id)
{
    fsm_button_t *p_fsm_button = malloc(sizeof(fsm_button_t));
    fsm_button_init(p_fsm_button, debounce_time, button_id);
    return p_fsm_button;
}

/* FSM-interface functions. These functions are used to interact with the FSM */
void fsm_button_fire (fsm_button_t *p_fsm)
{
    // 1. Call the fsm_fire() function. Pass the address of the fsm_t struct.
    fsm_fire(&p_fsm->f);
}

void fsm_button_destroy (fsm_button_t *p_fsm)
{
    free(&p_fsm->f);
}

fsm_t * fsm_button_get_inner_fsm (fsm_button_t *p_fsm)
{
    // 1. Return the address of the f field of the struct.
    return &p_fsm->f;
}

uint32_t fsm_button_get_state (fsm_button_t *p_fsm)
{
    // 1. Retrieve and return the field current_state of the FSM (field f of the struct).
    return p_fsm->f.current_state;
}

uint32_t fsm_button_get_duration (fsm_button_t * p_fsm) {
    // 1. Retrieve and return the field duration
    return p_fsm->duration;
}

void fsm_button_reset_duration (fsm_button_t * p_fsm) {
    // 1. Set to 0 the field duration
    p_fsm->duration = 0;
}

/**
 * @brief This function returns the debounce time of the button FSM.
 * 
 * @param p_fsm Pointer to an fsm_button_t struct.
 * @return uint32_t Debounce time in milliseconds.
 */
uint32_t fsm_button_get_debounce_time_ms (fsm_button_t * p_fsm)	{
    // 1. Retrieve and return the field debounce_time_ms
    return p_fsm->debounce_time_ms;
}

bool fsm_button_check_activity (fsm_button_t *p_fsm) {
    // 1. Get the field current_state of the FSM (field f of the struct).
    uint32_t current_state = p_fsm->f.current_state;

    // 2. Return false if the current state is BUTTON_RELEASED. Otherwise, return true.
    return current_state != BUTTON_RELEASED;
}