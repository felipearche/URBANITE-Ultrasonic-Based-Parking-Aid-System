/**
 * @file fsm_urbanite.c
 * @brief Urbanite project FSM main file.
 * @author Felipe Fernández-Arche
 * @author David Pedraza
 * @date 17/04/2025
 */

/* Includes ------------------------------------------------------------------*/
/* Standard C includes */
#include <stdlib.h>
#include <stdio.h>

/* HW dependent includes */
#include "port_system.h"

/* Project includes */
#include "fsm.h"
#include "fsm_urbanite.h"

// josueportiz
#include "port_ultrasound.h"

/* Typedefs --------------------------------------------------------------------*/
/**
 * @brief Structure to define the Urbanite FSM.
 * 
 */
struct fsm_urbanite_t
{
    fsm_t f; /*!< Urbanite FSM */
    fsm_button_t *p_fsm_button; /*!< Pointer to the button FSM */
    uint32_t on_off_press_time_ms; /*!< Time in ms to consider ON/OFF */
    uint32_t pause_display_time_ms; /*!< Time in ms to pause the display */
    bool is_paused; /*!< Flag to indicate if the system is paused */
    fsm_ultrasound_t *p_fsm_ultrasound_rear; /*!< Pointer to the ultrasound FSM */
    fsm_display_t *p_fsm_display_rear; /*!< Pointer to the display FSM */
};

/* Private functions -----------------------------------------------------------*/
/**
 * @brief Check if the button has been pressed for the required time to turn ON the Urbanite system.
 * 
 * @param p_this Pointer to an fsm_t struct that contains an `fsm_urbanite_t`.
 * @return true 
 * @return false 
 */
static bool check_on (fsm_t *p_this)
{
    fsm_urbanite_t *p_urbanite = (fsm_urbanite_t *)p_this; // Cast to fsm_urbanite_t

    // 1. Call function `fsm_button_get_duration()` to get the duration of the button press
    uint32_t button_duration = fsm_button_get_duration(p_urbanite->p_fsm_button);

    // 2. Return true if the duration is greater than 0 and greater than the required time to turn ON the system. Otherwise, return false
    return (button_duration > 0) && (button_duration > p_urbanite->on_off_press_time_ms);
}

/**
 * @brief Check if the button has been pressed for the required time to turn OFF the system.
 * 
 * @param p_this Pointer to an fsm_t struct that contains an `fsm_urbanite_t`.
 * @return true 
 * @return false 
 */
static bool check_off (fsm_t *p_this)
{
    // 1. It is enough to call the function `check_on()` because the required time to turn ON and OFF the system is the same.
    return check_on(p_this);
}

/**
 * @brief Check if a new measurement is ready.
 * 
 * @param p_this Pointer to an fsm_t struct that contains an `fsm_urbanite_t`.
 * @return true 
 * @return false 
 */
static bool check_new_measure (fsm_t *p_this)
{
    fsm_urbanite_t *p_urbanite = (fsm_urbanite_t *)p_this; // Cast to fsm_urbanite_t

    // 1. Call function `fsm_ultrasound_get_new_measurement_ready()` to check if a new measurement is ready and return the result.
    return fsm_ultrasound_get_new_measurement_ready(p_urbanite->p_fsm_ultrasound_rear);
}

/**
 * @brief Check if it has been required to pause the display.
 * 
 * @param p_this Pointer to an fsm_t struct that contains an `fsm_urbanite_t`.
 * @return true 
 * @return false 
 */
static bool check_pause_display (fsm_t *p_this)
{
    fsm_urbanite_t *p_urbanite = (fsm_urbanite_t *)p_this; // Cast to fsm_urbanite_t

    // 1. Call function `fsm_button_get_duration()` to get the duration of the button press
    uint32_t button_duration = fsm_button_get_duration(p_urbanite->p_fsm_button);

    // 2. Return true if the duration is greater than 0, less than the required time to turn ON the system, and greater than the required time to pause the display. Otherwise, return false
    return (button_duration > 0) && (button_duration < p_urbanite->on_off_press_time_ms) && (button_duration > p_urbanite->pause_display_time_ms);
}

/**
 * @brief Check if any of the elements of the system is active.
 * 
 * @param p_this Pointer to an fsm_t struct that contains an `fsm_urbanite_t`.
 * @return true 
 * @return false 
 */
static bool check_activity (fsm_t *p_this)
{
    fsm_urbanite_t *p_urbanite = (fsm_urbanite_t *)p_this; // Cast to fsm_urbanite_t

    // 1. Return true if any of the elements (button, ultrasound, or display) is active. Otherwise, return false
    //return fsm_button_check_activity(p_urbanite->p_fsm_button) || fsm_ultrasound_check_activity(p_urbanite->p_fsm_ultrasound_rear) || fsm_display_check_activity(p_urbanite->p_fsm_display_rear);
    bool vae1 = fsm_button_check_activity(p_urbanite->p_fsm_button);
    bool vae2 = fsm_ultrasound_check_activity(p_urbanite->p_fsm_ultrasound_rear);
    bool vae3 = fsm_display_check_activity(p_urbanite->p_fsm_display_rear);
    return vae1 || vae2 || vae3;
}

/**
 * @brief Check if all the elements of the system are inactive.
 * 
 * @param p_this Pointer to an fsm_t struct that contains an `fsm_urbanite_t`.
 * @return true 
 * @return false 
 */
static bool check_no_activity (fsm_t *p_this)
{
    // 1. Call function `check_activity()` and return the inverse of the result. The result will be true if all the elements of the system are inactive, otherwise it will be false.
    return !check_activity(p_this);
}

/**
 * @brief Check if any a new measurement is ready while the system is in low power mode.
 * 
 * @param p_this Pointer to an fsm_t struct that contains an `fsm_urbanite_t`.
 * @return true 
 * @return false 
 */
static bool check_activity_in_measure (fsm_t *p_this)
{
    //fsm_urbanite_t *p_urbanite = (fsm_urbanite_t *)p_this; // Cast to fsm_urbanite_t

    // 1. Call function `check_new_measure()` to check if a new measurement is ready and return the result.
    return check_new_measure(p_this);
}

/**
 * @brief Turn the Urbanite system ON.
 * 
 * @param p_this Pointer to an fsm_t struct that contains an `fsm_urbanite_t`.
 */
static void do_start_up_measure (fsm_t *p_this)
{
    fsm_urbanite_t *p_urbanite = (fsm_urbanite_t *)p_this; // Cast to fsm_urbanite_t

    // 1. Reset the duration of the button by calling `fsm_button_reset_duration`()` to avoid the system turning OFF again.
    fsm_button_reset_duration(p_urbanite->p_fsm_button);

    /* port_display_set_rgb(p_display->display_id, (rgb_color_t){255, 255, 255});
    port_system_delay_ms(1000); */
/*     port_display_set_rgb(p_urbanite->p_fsm_display_rear->display_id, (rgb_color_t){255, 255, 255});
    port_system_delay_ms(1000);
    port_display_set_rgb(p_urbanite->p_fsm_display_rear->display_id, (rgb_color_t){0, 0, 0}); */

    // 2. Start the ultrasound sensor by calling its appropriate function with the right parameter.
    fsm_ultrasound_start(p_urbanite->p_fsm_ultrasound_rear);

    // 3. Set the appropriate status of the display system by calling its function with the right parameter.
    fsm_display_set_status(p_urbanite->p_fsm_display_rear, true);

    // Print a message to help debug and log the status of the system.
    printf("[URBANITE][%ld] Urbanite system ON\n", port_system_get_millis());
    // Where `port_system_get_millis()` returns the current system tick.
}

/**
 * @brief Display the distance measured by the ultrasound sensor.
 * 
 * @param p_this Pointer to an fsm_t struct that contains an `fsm_urbanite_t`.
 */
static void do_display_distance (fsm_t *p_this)
{
    fsm_urbanite_t *p_urbanite = (fsm_urbanite_t *)p_this; // Cast to fsm_urbanite_t

    // 1. Get the distance measured by the ultrasound sensor by calling `fsm_ultrasound_get_distance()` with the right ultrasound sensor.
    uint32_t distance_cm = fsm_ultrasound_get_distance(p_urbanite->p_fsm_ultrasound_rear);

    // 2. If the system is paused:
    if (p_urbanite->is_paused) {
        // If the distance is less than WARNING_MIN_CM / 2 cm, set the distance to the display and set the display status to true.
        if (distance_cm < (WARNING_MIN_CM / 2)) {
            fsm_display_set_distance(p_urbanite->p_fsm_display_rear, distance_cm);
            fsm_display_set_status(p_urbanite->p_fsm_display_rear, true);
        } else {
            // Otherwise, set the display status to false.
            fsm_display_set_status(p_urbanite->p_fsm_display_rear, false);
        }
    } else {
        // If the system is not paused, set the distance to the display.
        fsm_display_set_distance(p_urbanite->p_fsm_display_rear, distance_cm);
    }

    // 3. Print a message to help debug and log the distance measured by the ultrasound sensor.
    printf("[URBANITE][%ld] Distance: %ld cm\n", port_system_get_millis(), distance_cm);
    // Where `port_system_get_millis()` returns the current system tick.
}

/**
 * @brief Pause or resume the display system.
 * 
 * @param p_this Pointer to an fsm_t struct that contains an `fsm_urbanite_t`.
 */
static void do_pause_display (fsm_t *p_this)
{
    fsm_urbanite_t *p_urbanite = (fsm_urbanite_t *)p_this; // Cast to fsm_urbanite_t

    // 1. Reset the duration of the button by calling `fsm_button_reset_duration()` to avoid the system to pause again.
    fsm_button_reset_duration(p_urbanite->p_fsm_button);

    // 2. Invert the pause status. If the system is paused, set the pause status to false. If the system is not paused, set the pause status to true.
    p_urbanite->is_paused = !p_urbanite->is_paused;

    // 3. Activate or deactivate the display depending on the new pause status by calling `fsm_display_set_status()` with the right parameter.
    fsm_display_set_status(p_urbanite->p_fsm_display_rear, !p_urbanite->is_paused);

    // 4. Print status depending on the pause status.
    if (p_urbanite->is_paused) {
        printf("[URBANITE][%ld] Urbanite system display PAUSE\n", port_system_get_millis());
    } else {
        printf("[URBANITE][%ld] Urbanite system display RESUME\n", port_system_get_millis());
    }
    // Where `port_system_get_millis()` returns the current system tick.
}

/**
 * @brief Turn the Urbanite system OFF.
 * 
 * @param p_this Pointer to an fsm_t struct that contains an `fsm_urbanite_t`.
 */
static void do_stop_urbanite (fsm_t *p_this)
{
    fsm_urbanite_t *p_urbanite = (fsm_urbanite_t *)p_this; // Cast to fsm_urbanite_t

    // 1. Reset the duration of the button by calling `fsm_button_reset_duration()` to avoid the system to turn ON again.
    fsm_button_reset_duration(p_urbanite->p_fsm_button);

    // 2. Stop the ultrasound sensor by calling `fsm_ultrasound_stop()` with the right parameters.
    fsm_ultrasound_stop(p_urbanite->p_fsm_ultrasound_rear);

    // 3. Turn the display system off by calling `fsm_display_set_status()` with the right parameter.
    fsm_display_set_status(p_urbanite->p_fsm_display_rear, false);

    // 4. If the system is paused, remove the pause status to avoid the system to turn ON again with the display paused.
    p_urbanite->is_paused = false;

    // 5. Print a message to help debug and log the status of the system.
    printf("[URBANITE][%ld] Urbanite system OFF\n", port_system_get_millis());
    // Where `port_system_get_millis()` returns the current system tick.
}

/**
 * @brief Start the low power mode while the Urbanite is awakened by a debug breakpoint or similar in the SLEEP_WHILE_OFF state.
 * 
 * @param p_this Pointer to an fsm_t struct that contains an `fsm_urbanite_t`.
 */
static void do_sleep_while_off (fsm_t *p_this)
{
    // 1. Call function `port_system_sleep()` to start the low power mode
    port_system_sleep();
}

/**
 * @brief Start the low power mode while the Urbanite is awakened by a debug breakpoint or similar in the SLEEP_WHILE_ON state.
 * 
 * @param p_this Pointer to an fsm_t struct that contains an `fsm_urbanite_t`.
 */
static void do_sleep_while_on (fsm_t *p_this)
{
    // 1. Call function `port_system_sleep()` to start the low power mode
    port_system_sleep();
}

/**
 * @brief Start the low power mode while the Urbanite is measuring the distance and it is waiting for a new measurement.
 * 
 * @param p_this Pointer to an fsm_t struct that contains an `fsm_urbanite_t`.
 */
static void do_sleep_while_measure (fsm_t *p_this)
{
    // 1. Call function `port_system_sleep()` to start the low power mode
    port_system_sleep();
}

/**
 * @brief Start the low power mode while the Urbanite is OFF.
 * 
 * @param p_this Pointer to an fsm_t struct that contains an `fsm_urbanite_t`.
 */
static void do_sleep_off (fsm_t *p_this)
{
    // 1. Call function `port_system_sleep()` to start the low power mode
    port_system_sleep();
}

/**
 * @brief Array representing the transitions table of the FSM Urbanite.
 * 1. When check_on() returns true, the state will change from OFF to MEASURE, and the function do_start_up_measure() will be called.
 * 2. When check_no_activity() returns true, the state will change from OFF to SLEEP_WHILE_OFF, and the function do_sleep_off() will be called.
 * 3. When check_activity() returns true, the state will change from SLEEP_WHILE_OFF to OFF.
 * 4. When check_no_activity() returns true, the state will remain in SLEEP_WHILE_OFF, and the function do_sleep_while_off() will be called.
 * 5. When check_off() returns true, the state will change from MEASURE to OFF, and the function do_stop_urbanite() will be called.
 * 6. When check_pause_display() returns true, the state will remain in MEASURE, and the function do_pause_display() will be called.
 * 7. When check_new_measure() returns true, the state will remain in MEASURE, and the function do_display_distance() will be called.
 * 8. When check_no_activity() returns true, the state will change from MEASURE to SLEEP_WHILE_ON, and the function do_sleep_while_measure() will be called.
 * 9. When check_activity_in_measure() returns true, the state will change from SLEEP_WHILE_ON to MEASURE.
 * 10. When check_no_activity() returns true, the state will remain in SLEEP_WHILE_ON, and the function do_sleep_while_on() will be called.
 */
static fsm_trans_t fsm_trans_urbanite[] = {
    {OFF, check_on, MEASURE, do_start_up_measure},
    {OFF, check_no_activity, SLEEP_WHILE_OFF, do_sleep_off},
    {SLEEP_WHILE_OFF, check_activity, OFF, NULL},
    {SLEEP_WHILE_OFF, check_no_activity, SLEEP_WHILE_OFF, do_sleep_while_off},
    {MEASURE, check_off, OFF, do_stop_urbanite},
    {MEASURE, check_pause_display, MEASURE, do_pause_display},
    {MEASURE, check_new_measure, MEASURE, do_display_distance},
    {MEASURE, check_no_activity, SLEEP_WHILE_ON, do_sleep_while_measure}, //esto es lo que falla
    {SLEEP_WHILE_ON, check_activity_in_measure, MEASURE, NULL},
    {SLEEP_WHILE_ON, check_no_activity, SLEEP_WHILE_ON, do_sleep_while_on},
    {-1, NULL, -1, NULL},
};


/**
 * @brief Create a new Urbanite FSM.
 * 
 * This FSM implements a system that measures the distance to the nearest obstacle and shows it on a display. The display is a RGB LED that changes its color depending on the distance to the obstacle. The system is activated by pressing a button. The system is deactivated by pressing the same button for the same time.
 * 
 * The basic implementation of this FSM assumes that the system is mounted on a car and the distance is measured by an ultrasound sensor located at the rear of the car. The display is located on the dashboard of the car. The button is assumed that activates when the driver is parking the car. The system can add more sensors and displays to cover more areas of the car.
 * 
 * A short press of the button pauses the display if it disturbs the driver, but the system continues measuring the distance. In case the driver wants to activate the display again, he must press the button again and the display will show the last distance measured. A long press of the button deactivates the ultrasounds and the displays.
 * 
 * When the system is OFF it does not measure the distance and the display is OFF. The system is in a low power mode.
 * 
 * @param p_fsm_urbanite Pointer to the Urbanite FSM.
 * @param p_fsm_button Pointer to the button FSM that activates the system and disables the display if it disturbs the driver.
 * @param on_off_press_time_ms Button press time in milliseconds to turn the system ON or OFF
 * @param pause_display_time_ms Time in milliseconds to pause the display after a short press of the button
 * @param p_fsm_ultrasound_rear Pointer to the ultrasound FSM that measures the distance to the rear obstacle.
 * @param p_fsm_display_rear Pointer to the display FSM that shows the distance to the rear obstacle.
 */
static void fsm_urbanite_init (fsm_urbanite_t *p_fsm_urbanite, fsm_button_t *p_fsm_button, uint32_t on_off_press_time_ms, uint32_t pause_display_time_ms, fsm_ultrasound_t *p_fsm_ultrasound_rear, fsm_display_t *p_fsm_display_rear)
{
    // 1. Call function fsm_init() with the received pointer to fsm_t and the transition table.
    fsm_init(&(p_fsm_urbanite->f), fsm_trans_urbanite);

    // 2. Initialize the fields p_fsm_button, on_off_press_time_ms, pause_display_time_ms, p_fsm_ultrasound_rear, and p_fsm_display_rear of the Urbanite FSM with the received parameters
    p_fsm_urbanite->p_fsm_button = p_fsm_button;
    p_fsm_urbanite->on_off_press_time_ms = on_off_press_time_ms;
    p_fsm_urbanite->pause_display_time_ms = pause_display_time_ms;
    p_fsm_urbanite->p_fsm_ultrasound_rear = p_fsm_ultrasound_rear;
    p_fsm_urbanite->p_fsm_display_rear = p_fsm_display_rear;

    // 3. Initialize the field is_paused to false
    p_fsm_urbanite->is_paused = false;
}

/* Public functions -----------------------------------------------------------*/
fsm_urbanite_t * fsm_urbanite_new (fsm_button_t *p_fsm_button, uint32_t on_off_press_time_ms, uint32_t pause_display_time_ms, fsm_ultrasound_t *p_fsm_ultrasound_rear, fsm_display_t *p_fsm_display_rear)
{
    // 1. Allocate memory for the `fsm_urbanite_t` struct in the same way as the `fsm_button_new()`, `fsm_ultrasound_new()` and `fsm_display_new()` functions.
    fsm_urbanite_t *p_fsm_urbanite = (fsm_urbanite_t *)malloc(sizeof(fsm_urbanite_t));
    if (p_fsm_urbanite == NULL) {
        return NULL; // Return NULL if memory allocation fails
    }

    // 2. Initialize the fsm_t struct of the Urbanite FSM by calling the `fsm_urbanite_init()` function.
    fsm_urbanite_init(p_fsm_urbanite, p_fsm_button, on_off_press_time_ms, pause_display_time_ms, p_fsm_ultrasound_rear, p_fsm_display_rear);

    // 3. Return the pointer to the Urbanite FSM once it has been created and initialized.
    return p_fsm_urbanite;
}

void fsm_urbanite_fire (fsm_urbanite_t *p_fsm)
{
    // 1. Call function fsm_fire() with the received pointer to fsm_t
    fsm_fire(&(p_fsm->f));
}

void fsm_urbanite_destroy (fsm_urbanite_t *p_fsm)
{
    // 1. Implement this function analogously to the `fsm_button_destroy()`, `fsm_ultrasound_destroy()` and `fsm_display_destroy()` functions.
    
    // Destroy the button FSM
    fsm_button_destroy(p_fsm->p_fsm_button);

    // Destroy the ultrasound FSM
    fsm_ultrasound_destroy(p_fsm->p_fsm_ultrasound_rear);

    // Destroy the display FSM
    fsm_display_destroy(p_fsm->p_fsm_display_rear);

    // Free the memory allocated for the Urbanite FSM
    free(p_fsm);
}