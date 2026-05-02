/**
 * @file fsm_display.c
 * @brief Display system FSM main file.
 * @author alumno1
 * @author alumno2
 * @date fecha
 */

/* Includes ------------------------------------------------------------------*/
/* Standard C includes */
#include <stdlib.h>
#include <stdio.h>

/* HW dependent includes */
#include "port_display.h"
#include "port_system.h"

/* Project includes */
#include "fsm.h"
#include "fsm_display.h"

/* Typedefs --------------------------------------------------------------------*/
/**
 * @brief Structure of the Display FSM.
 * 
 */
struct fsm_display_t
{
    fsm_t f; /*!< Display system FSM */
    int32_t distance_cm; /*!< Distance in cm to the object */
    bool new_color; /*!< Flag to indicate if a new color has to be set */
    bool status; /*!< Flag to indicate if the display is active */
    bool idle; /*!< Flag to indicate if the display being active is idle, or not */
    uint32_t display_id; /*!< Unique display system identifier number */
};

/* Private functions -----------------------------------------------------------*/
/**
 * @brief Interpolates a value between two points.
 *
 * This function performs a linear interpolation between two points (x0, y0) and (x1, y1)
 * for a given value x. It returns the corresponding interpolated y value as an 8-bit unsigned integer.
 * If x1 equals x0, the function returns y0 to avoid division by zero.
 *
 * @param x  The value for which to interpolate.
 * @param x0 The lower bound of the x range.
 * @param x1 The upper bound of the x range.
 * @param y0 The value corresponding to x0.
 * @param y1 The value corresponding to x1.
 * @return uint8_t The interpolated value between y0 and y1 for the given x.
 */
static uint8_t _interpolate(int32_t x, int32_t x0, int32_t x1, uint8_t y0, uint8_t y1) {
    if (x1 == x0) return y0;
    return (uint8_t)(y0 + ((y1 - y0) * (x - x0)) / (x1 - x0));
}
/**
 * @brief Set color levels of the RGB LEDs according to the distance.
 * 
 * This function sets the levels of an RGB LED according to the distance measured by the ultrasound sensor. This RGB LED structure is later passed to the `port_display_set_rgb()` function to set the color of the RGB LED.
 * 
 * @param p_color Pointer to an `rgb_color_t` struct that will store the levels of the RGB LED.
 * @param distance_cm Distance measured by the ultrasound sensor in centimeters.
 */
void _compute_display_levels (rgb_color_t *p_color, int32_t distance_cm)
{
    if (distance_cm >= DANGER_MIN_CM && distance_cm <= WARNING_MIN_CM) {
        p_color->r = 255;
        p_color->g = 0;
        p_color->b = 0;
    } else if (distance_cm > WARNING_MIN_CM && distance_cm <= NO_PROBLEM_MIN_CM) {
        p_color->r = _interpolate(distance_cm, WARNING_MIN_CM, NO_PROBLEM_MIN_CM, 255, 238);
        p_color->g = _interpolate(distance_cm, WARNING_MIN_CM, NO_PROBLEM_MIN_CM, 0, 238);
        p_color->b = 0;
    } else if (distance_cm > NO_PROBLEM_MIN_CM && distance_cm <= INFO_MIN_CM) {
        p_color->r = _interpolate(distance_cm, NO_PROBLEM_MIN_CM, INFO_MIN_CM, 238, 0);
        p_color->g = _interpolate(distance_cm, NO_PROBLEM_MIN_CM, INFO_MIN_CM, 238, 255);
        p_color->b = 0;
    } else if (distance_cm > INFO_MIN_CM && distance_cm <= OK_MIN_CM) {
        p_color->r = _interpolate(distance_cm, INFO_MIN_CM, OK_MIN_CM, 0, 26);
        p_color->g = _interpolate(distance_cm, INFO_MIN_CM, OK_MIN_CM, 255, 90);
        p_color->b = _interpolate(distance_cm, INFO_MIN_CM, OK_MIN_CM, 0, 82);
    } else if (distance_cm > OK_MIN_CM && distance_cm <= OK_MAX_CM) {
        p_color->r = _interpolate(distance_cm, OK_MIN_CM, OK_MAX_CM, 26, 0);
        p_color->g = _interpolate(distance_cm, OK_MIN_CM, OK_MAX_CM, 90, 0);
        p_color->b = _interpolate(distance_cm, OK_MIN_CM, OK_MAX_CM, 82, 255);
    } else if (distance_cm > OK_MAX_CM) {
        p_color->r = 0;
        p_color->g = 0;
        p_color->b = 255;
    } else {
        p_color->r = 0;
        p_color->g = 0;
        p_color->b = 0;
    }
} // esto hace que salga error en el test_fsm_display.c, porque el test espera valores fijos, y hemos puesto que sea continuo

/**
 * @brief Check if a new color has to be set.
 * 
 * @param p_this Pointer to an fsm_t struct than contains an `fsm_display_t`.
 * @return true If a new color has to be set
 * @return false If a new color does not have to be set
 */
static bool check_set_new_color (fsm_t *p_this)
{
    fsm_display_t *p_display = (fsm_display_t *)p_this; // Cast to fsm_display_t
    // 1. Return the flag new_color
    return p_display->new_color;
}

/**
 * @brief Check if the display is set to be active (ON), independently if it is idle or not.
 * 
 * @param p_this Pointer to an fsm_t struct than contains an `fsm_display_t`.
 * @return true If the display system has been indicated to be active independently if it is idle or not.
 * @return false If the display system has been indicated to be inactive.
 */
static bool check_active (fsm_t *p_this)
{
    fsm_display_t *p_display = (fsm_display_t *)p_this; // Cast to fsm_display_t
    // 1. Return the flag status
    return p_display->status;
}

/**
 * @brief Check if the display is set to be inactive (OFF).
 * 
 * @param p_this Pointer to an fsm_t struct than contains an `fsm_display_t`.
 * @return true If the display system has been indicated to be inactive.
 * @return false If the display system has been indicated to be active.
 */
static bool check_off (fsm_t *p_this)
{
    fsm_display_t *p_display = (fsm_display_t *)p_this; // Cast to fsm_display_t
    // 1. Return the inverse of the status flag
    return !p_display->status;
}

/**
 * @brief Turn the display system ON for the first time.
 * 
 * @param p_this Pointer to an fsm_t struct than contains an `fsm_display_t`.
 */
static void do_set_on (fsm_t *p_this)
{
    fsm_display_t *p_display = (fsm_display_t *)p_this; // Cast to fsm_display_t

    // 1. Call function `port_display_set_rgb()` with the RGB LED ID with no color (all the duty cycles to 0).
    port_display_set_rgb(p_display->display_id, (rgb_color_t){0, 0, 0});
}

/**
 * @brief Set the color of the RGB LED according to the distance measured by the ultrasound sensor.
 * 
 * @param p_this Pointer to an fsm_t struct than contains an `fsm_display_t`.
 */
static void do_set_color (fsm_t *p_this)
{
    fsm_display_t *p_display = (fsm_display_t *)p_this; // Cast to fsm_display_t
    rgb_color_t color;

    // 1. Compute the levels of the RGB LEDs according to the distance
    int32_t distancia = p_display->distance_cm;
    _compute_display_levels(&color, distancia);

    // 2. Call function port_display_set_rgb() with the RGB LED ID and the color
    if(distancia >= DANGER_MIN_CM && distancia <= WARNING_MIN_CM){
        port_display_set_rgb(p_display->display_id, (rgb_color_t){0, 0, 0});
        port_system_delay_ms(25);
        port_display_set_rgb(p_display->display_id, color);
    }
    if(distancia > WARNING_MIN_CM && distancia <= NO_PROBLEM_MIN_CM){
        port_display_set_rgb(p_display->display_id, (rgb_color_t){0, 0, 0});
        port_system_delay_ms(100);
        port_display_set_rgb(p_display->display_id, color);
    }
    if(distancia > NO_PROBLEM_MIN_CM && distancia <= INFO_MIN_CM){
        port_display_set_rgb(p_display->display_id, (rgb_color_t){0, 0, 0});
        port_system_delay_ms(250);
        port_display_set_rgb(p_display->display_id, color);
    }
    if(distancia > INFO_MIN_CM && distancia <= OK_MIN_CM){
        port_display_set_rgb(p_display->display_id, (rgb_color_t){0, 0, 0});
        port_system_delay_ms(500);
        port_display_set_rgb(p_display->display_id, color);
    }
    if(distancia > OK_MIN_CM && distancia <= OK_MAX_CM){
        port_display_set_rgb(p_display->display_id, (rgb_color_t){0, 0, 0});
        port_system_delay_ms(1000);
        port_display_set_rgb(p_display->display_id, color);
    }
    else{
        port_display_set_rgb(p_display->display_id, color);
    }

    // 3. Reset the flag new_color to indicate that the color has been set
    p_display->new_color = false;

    // 4. Set the display system to idle
    p_display->idle = true;
} // LED PARPADEA BLINK

/**
 * @brief Turn the display system OFF.
 * 
 * @param p_this Pointer to an fsm_t struct than contains an `fsm_display_t`.
 */
static void do_set_off (fsm_t *p_this)
{
    fsm_display_t *p_display = (fsm_display_t *)p_this; // Cast to fsm_display_t

    // 1. Call function port_display_set_rgb() with the RGB LED ID with no color (COLOR_OFF)
    port_display_set_rgb(p_display->display_id, (rgb_color_t){0, 0, 0});
    // 2. Reset the flag idle to indicate that the display system is not idle
    p_display->idle = false;
}

/**
 * @brief 
 * 1. When check_active() returns true, the state will change from WAIT_DISPLAY to SET_DISPLAY, and the function do_set_on() will be called.
 * 2. When check_set_new_color() returns true, the state will change from SET_DISPLAY to SET_DISPLAY, and the function do_set_color() will be called.
 * 3. When check_off() returns true, the state will change from SET_DISPLAY to WAIT_DISPLAY, and the function do_set_off() will be called.
 */
static fsm_trans_t fsm_trans_display[] = {
    {WAIT_DISPLAY, check_active, SET_DISPLAY, do_set_on},
    {SET_DISPLAY, check_set_new_color, SET_DISPLAY, do_set_color},
    {SET_DISPLAY, check_off, WAIT_DISPLAY, do_set_off},
    {-1, NULL, -1, NULL},
};

/**
 * @brief Initialize a display system FSM.
 * 
 * This function initializes the default values of the FSM struct and calls to the port to initialize the associated HW given the ID.
 * 
 * @param p_fsm_display Pointer to the display FSM.
 * @param display_id Unique display identifier number.
 */
static void fsm_display_init (fsm_display_t *p_fsm_display, uint32_t display_id)
{
    // 1. Call the fsm_init() to initialize the FSM. Pass the address of the fsm_t struct and the transition table.
    fsm_init(&(p_fsm_display->f), fsm_trans_display);

    // 2. Initialize the display_id.
    p_fsm_display->display_id = display_id;

    // 3. Set the distance_cm to -1 or any other invalid value in the range of the distance.
    p_fsm_display->distance_cm = -1;

    // 4. Initialize the flags new_color, status, and idle to false.
    p_fsm_display->new_color = false;
    p_fsm_display->status = false;
    p_fsm_display->idle = false;

    // 5. Call function port_display_init() to initialize the HW.
    port_display_init(display_id);
}

/* Public functions -----------------------------------------------------------*/
fsm_display_t *fsm_display_new(uint32_t display_id)
{
    fsm_display_t *p_fsm_display = malloc(sizeof(fsm_display_t)); /* Do malloc to reserve memory of all other FSM elements, although it is interpreted as fsm_t (the first element of the structure) */
    fsm_display_init(p_fsm_display, display_id); /* Initialize the FSM */
    return p_fsm_display;
}

void fsm_display_destroy (fsm_display_t *p_fsm)
{
    // 1. Implement this function analogously to the fsm_display_destroy() function.
    free(p_fsm);
}

void fsm_display_fire (fsm_display_t *p_fsm)
{
    // 1. Call the fsm_fire() function. Pass the address of the fsm_t struct.
    fsm_fire(&(p_fsm->f));
}

void fsm_display_set_distance (fsm_display_t *p_fsm, uint32_t distance_cm)
{
    // 1. Set the distance in cm in the display system FSM.
    p_fsm->distance_cm = distance_cm;

    // 2. Set the new_color field accordingly to indicate that a new color has to be set.
    p_fsm->new_color = true;
}

bool fsm_display_get_status (fsm_display_t *p_fsm)
{
    // 1. Retrieve and return the field status.
    return p_fsm->status;
}

void fsm_display_set_status (fsm_display_t *p_fsm, bool status)
{
    // 1. Update the field status with the received value.
    p_fsm->status = status;
}

bool fsm_display_check_activity (fsm_display_t *p_fsm)
{
    // 1. Return true if the display system is active and it is not idle. Otherwise, return false.
    return p_fsm->status && !p_fsm->idle;
}

fsm_t *fsm_display_get_inner_fsm (fsm_display_t *p_fsm)
{
    // 1. Return the address of the f field of the struct.
    return &(p_fsm->f);
}

uint32_t fsm_display_get_state (fsm_display_t *p_fsm)
{
    // 1. Retrieve and return the field current_state of the FSM (field f of the struct).
    return p_fsm->f.current_state;
}

void fsm_display_set_state (fsm_display_t *p_fsm, int8_t state)
{
    // 1. Set the current state of the FSM (field f of the struct) to the provided state.
    p_fsm->f.current_state = state;
}