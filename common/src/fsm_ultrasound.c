/**
 * @file fsm_ultrasound.c
 * @brief Ultrasound sensor FSM main file.
 * @author Felipe Fernández-Arche
 * @author David Pedraza
 * @date 25/03/2025
 */

/* Includes ------------------------------------------------------------------*/
/* Standard C includes */
#include <stdlib.h>
#include <string.h>

/* HW dependent includes */
#include "port_ultrasound.h"
#include "port_system.h"

/* Project includes */
#include "fsm.h"
#include "fsm_ultrasound.h"

/* Typedefs --------------------------------------------------------------------*/
/**
 * @brief Structure to define the Ultrasound FSM.
 * 
 */
struct fsm_ultrasound_t
{
    fsm_t f; /*!< Ultrasound FSM */
    uint32_t distance_cm; /*!< How much time the ultrasound has been pressed */
    bool status; /*!< Indicate if the ultrasound sensor is active or not */
    bool new_measurement; /*!< Flag to indicate if a new measurement has been completed */
    uint32_t ultrasound_id; /*!< Ultrasound ID. Must be unique. */
    uint32_t distance_arr[FSM_ULTRASOUND_NUM_MEASUREMENTS]; /*!< Array to store the last distance measurements */
    uint32_t distance_idx; /*!< Index to store the last distance measurement */
};

/* Private functions -----------------------------------------------------------*/
// Comparison function for qsort
/**
 * @brief Compare function to sort arrays.
 * 
 * This function is used to compare two elements. It will be used for the qsort() function to sort the array of distances.
 * 
 * @param a Pointer to the first element to compare.
 * @param b Pointer to the second element to compare.
 * @return int Result of the comparison.
 */
int _compare(const void *a, const void *b)
{
    return (*(uint32_t *)a - *(uint32_t *)b);
}

/* State machine input or transition functions */
/**
 * @brief Check if the ultrasound sensor is active and ready to start a new measurement.
 *
 * @param p_this Pointer to an fsm_t struct that contains an `fsm_ultrasound_t`.
 * @return true
 * @return false
 */
static bool check_on(fsm_t *p_this)
{
    fsm_ultrasound_t *p_fsm_ultrasound = (fsm_ultrasound_t *)p_this;

    // 1. Call function `port_ultrasound_get_trigger_ready()` and retrieve the status of the trigger signal
    bool trigger_ready = port_ultrasound_get_trigger_ready(p_fsm_ultrasound->ultrasound_id);

    // 2. Check if the internal status flag of the ultrasound FSM is active
    bool is_active = p_fsm_ultrasound->status;

    // 3. Return true if the ultrasound sensor flag is active and the trigger is ready. Otherwise, return false
    return (is_active && trigger_ready);
}

/**
 * @brief Check if the ultrasound sensor has been set to be inactive (OFF).
 *
 * @param p_this Pointer to an fsm_t struct that contains an `fsm_ultrasound_t`.
 * @return true
 * @return false
 */
static bool check_off (fsm_t *p_this)
{
    fsm_ultrasound_t *p_fsm_ultrasound = (fsm_ultrasound_t *)p_this;

    // 1. Return the inverse of the flag status.
    return !p_fsm_ultrasound->status;
}

/**
 * @brief Check if the ultrasound sensor has finished the trigger signal.
 *
 * @param p_this Pointer to an fsm_t struct that contains an `fsm_ultrasound_t`.
 * @return true
 * @return false
 */
static bool check_trigger_end(fsm_t *p_this)
{
    fsm_ultrasound_t *p_fsm_ultrasound = (fsm_ultrasound_t *)p_this;

    // 1. Call function `port_ultrasound_get_trigger_end()` and retrieve the status of the trigger signal and return it. It will be true if the time to trigger the ultrasound sensor has finished.
    bool status = port_ultrasound_get_trigger_end(p_fsm_ultrasound->ultrasound_id);
    return status;
}

/**
 * @brief Check if the ultrasound sensor has received the init (rising edge in the input capture) of the echo signal.
 *
 * @param p_this Pointer to an fsm_t struct that contains an `fsm_ultrasound_t`.
 * @return true
 * @return false
 */
static bool check_echo_init(fsm_t *p_this)
{
    fsm_ultrasound_t *p_fsm_ultrasound = (fsm_ultrasound_t *)p_this;

    // 1. Call function `port_ultrasound_get_echo_init_tick()` to retrieve the time tick of the echo signal
    uint32_t time_tick = port_ultrasound_get_echo_init_tick(p_fsm_ultrasound->ultrasound_id);

    // 2. Return true if the time tick is higher than 0. Otherwise, return false
    if (time_tick > 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

/**
 * @brief Check if the ultrasound sensor has received the end (falling edge in the input capture) of the echo signal.
 *
 * @param p_this Pointer to an fsm_t struct that contains an `fsm_ultrasound_t`.
 * @return true
 * @return false
 */
static bool check_echo_received(fsm_t *p_this)
{
    fsm_ultrasound_t *p_fsm_ultrasound = (fsm_ultrasound_t *)p_this;

    // 1. Call function `port_ultrasound_get_echo_received()` to retrieve and return the status of the echo signal: it will be true if both the init and end ticks have been received.
    bool status = port_ultrasound_get_echo_received(p_fsm_ultrasound->ultrasound_id);
    return status;
}

/**
 * @brief Check if a new measurement is ready.
 *
 * @param p_this Pointer to an fsm_t struct that contains an `fsm_ultrasound_t`.
 * @return true
 * @return false
 */
static bool check_new_measurement(fsm_t *p_this)
{
    fsm_ultrasound_t *p_fsm_ultrasound = (fsm_ultrasound_t *)p_this;

    // 1. Call function `port_ultrasound_get_trigger_ready()` to retrieve and return the status of the trigger signal. If it is true means that the ultrasound sensor is ready to start a new measurement.
    bool status = port_ultrasound_get_trigger_ready(p_fsm_ultrasound->ultrasound_id);
    return status;
}

/* State machine output or action functions */
/**
 * @brief Start a measurement of the ultrasound transceiver for the first time after the FSM is started.
 *
 * @param p_this Pointer to an fsm_t struct than contains an `fsm_ultrasound_t`.
 */
static void do_start_measurement(fsm_t *p_this)
{
    fsm_ultrasound_t *p_fsm_ultrasound = (fsm_ultrasound_t *)p_this;

    // 1. Call function `port_ultrasound_start_measurement()` with the right parameters to start a new measurement.
    port_ultrasound_start_measurement(p_fsm_ultrasound->ultrasound_id);
}

/**
 * @brief Start a new measurement of the ultrasound transceiver.
 * 
 * This function is called when the ultrasound sensor has finished a measurement and is ready to start a new one.
 * 
 * @param p_this Pointer to an fsm_t struct than contains an `fsm_ultrasound_t`.
 */
static void do_start_new_measurement(fsm_t *p_this)
{
    fsm_ultrasound_t *p_fsm_ultrasound = (fsm_ultrasound_t *)p_this;

    // 1. Call function `do_start_measurement()` with the right parameters to start a new measurement.
    port_ultrasound_start_measurement(p_fsm_ultrasound->ultrasound_id);
}

/**
 * @brief Stop the trigger signal of the ultrasound sensor.
 * 
 * This function is called when the time to trigger the ultrasound sensor has finished. It stops the trigger signal and the trigger timer.
 *
 * @param p_this Pointer to an fsm_t struct than contains an `fsm_ultrasound_t`.
 */
static void do_stop_trigger(fsm_t *p_this)
{
    fsm_ultrasound_t *p_fsm_ultrasound = (fsm_ultrasound_t *)p_this;

    // 1. Call function `port_ultrasound_stop_trigger_timer()` to stop the timer that controls the trigger signal.
    port_ultrasound_stop_trigger_timer(p_fsm_ultrasound->ultrasound_id);

    // 2. Call function `port_ultrasound_set_trigger_end()` to set the trigger signal to low.
    port_ultrasound_set_trigger_end(p_fsm_ultrasound->ultrasound_id, false);
}

/**
 * @brief Set the distance measured by the ultrasound sensor.
 * 
 * This function is called when the ultrasound sensor has received the echo signal. It calculates the distance in cm and stores it in the array of distances.
 * 
 * When the array is full, it computes the median of the array and resets the index of the array.
 * 
 * @param p_this Pointer to an fsm_t struct than contains an `fsm_ultrasound_t`.
 */
static void do_set_distance(fsm_t *p_this)
{
    fsm_ultrasound_t *p_fsm_ultrasound = (fsm_ultrasound_t *)p_this;

    // 1. Retrieve the echo init tick, echo end tick, and echo overflows from the ultrasound sensor.
    uint32_t echo_init_tick = port_ultrasound_get_echo_init_tick(p_fsm_ultrasound->ultrasound_id);
    uint32_t echo_end_tick = port_ultrasound_get_echo_end_tick(p_fsm_ultrasound->ultrasound_id);
    uint32_t echo_overflows = port_ultrasound_get_echo_overflows(p_fsm_ultrasound->ultrasound_id);

    // 2. Calculate the time that the sound has taken to go back and forth by using the equation that relates the time with the distance and the number of overflows.
    uint32_t total_ticks;
    if (echo_end_tick >= echo_init_tick)
    {
        total_ticks = echo_end_tick - echo_init_tick + echo_overflows * 65536;
    }
    else
    {
        total_ticks = (65536 - echo_init_tick) + echo_end_tick + (echo_overflows - 1) * 65536;
    }
    double total_time = (double)total_ticks; // en micros cuidado unidades

    // 3. Calculate the distance in cm taking into account the speed of sound.
    double distance_m = ((total_time / 1000000) * SPEED_OF_SOUND_MS) / 2.0; // Divide by 2 to get one-way distance
    uint32_t distance_cm = (uint32_t)(distance_m * 100.0);                  // Convert to cm

    // 4. Store the distance in the array of distances in the position of the index.
    p_fsm_ultrasound->distance_arr[p_fsm_ultrasound->distance_idx] = distance_cm;

    // 5. If the array is full, sort the array by calling the function qsort().
    if (p_fsm_ultrasound->distance_idx >= FSM_ULTRASOUND_NUM_MEASUREMENTS - 1)
    {
        qsort(p_fsm_ultrasound->distance_arr, FSM_ULTRASOUND_NUM_MEASUREMENTS, sizeof(uint32_t), _compare);

        // 6. If the array is full, compute the median of the array and store it in the field distance_cm.
        if (FSM_ULTRASOUND_NUM_MEASUREMENTS % 2 == 0)
        {
            p_fsm_ultrasound->distance_cm = (p_fsm_ultrasound->distance_arr[FSM_ULTRASOUND_NUM_MEASUREMENTS / 2 - 1] + p_fsm_ultrasound->distance_arr[FSM_ULTRASOUND_NUM_MEASUREMENTS / 2]) / 2;
        }
        else
        {
            p_fsm_ultrasound->distance_cm = p_fsm_ultrasound->distance_arr[FSM_ULTRASOUND_NUM_MEASUREMENTS / 2];
        }

        // 7. If the array is full, set the flag new_measurement to indicate that a new measurement is ready.
        p_fsm_ultrasound->new_measurement = true;
    }

    // 8. Increase the distance index. If the index is higher or equal FSM_ULTRASOUND_NUM_MEASUREMENTS, reset the index.
    p_fsm_ultrasound->distance_idx = (p_fsm_ultrasound->distance_idx + 1) % FSM_ULTRASOUND_NUM_MEASUREMENTS;

    // 9. Call function `port_ultrasound_stop_echo_timer()` to stop the timer that controls the input capture of the echo signal.
    port_ultrasound_stop_echo_timer(p_fsm_ultrasound->ultrasound_id);

    // 10. Call function `port_ultrasound_reset_echo_ticks()` to reset the time ticks of the echo signal.
    port_ultrasound_reset_echo_ticks(p_fsm_ultrasound->ultrasound_id);
}

/**
 * @brief Stop the ultrasound sensor.
 * 
 * This function is called when the ultrasound sensor is stopped. It stops the ultrasound sensor and resets the echo ticks.
 * 
 * @param p_this Pointer to an fsm_t struct than contains an `fsm_ultrasound_t`.
 */
static void do_stop_measurement(fsm_t *p_this)
{
    fsm_ultrasound_t *p_fsm_ultrasound = (fsm_ultrasound_t *)p_this;

    // 1. Call function `port_ultrasound_stop_ultrasound()` with the right parameters to stop the ultrasound sensor.
    port_ultrasound_stop_ultrasound(p_fsm_ultrasound->ultrasound_id);
}

/**
 * @brief Array representing the transitions table of the FSM ultrasound.
 * 1. When check_on() returns true, the state will change from WAIT_START to TRIGGER_START, and the function do_start_measurement() will be called.
 * 2. When check_trigger_end() returns true, the state will change from TRIGGER_START to WAIT_ECHO_START, and the function do_stop_trigger() will be called.
 * 3. When check_echo_init() returns true, the state will change from WAIT_ECHO_START to WAIT_ECHO_END.
 * 4. When check_echo_received() returns true, the state will change from WAIT_ECHO_END to SET_DISTANCE, and the function do_set_distance() will be called.
 * 5. When check_new_measurement() returns true, the state will change from SET_DISTANCE to TRIGGER_START, and the function do_start_new_measurement() will be called.
 * 6. When check_off() returns true, the state will change from SET_DISTANCE to WAIT_START, and the function do_stop_measurement() will be called.
 */
static fsm_trans_t fsm_trans_ultrasound[] = {
    {WAIT_START, check_on, TRIGGER_START, do_start_measurement},
    {TRIGGER_START, check_trigger_end, WAIT_ECHO_START, do_stop_trigger},
    {WAIT_ECHO_START, check_echo_init, WAIT_ECHO_END, NULL},
    {WAIT_ECHO_END, check_echo_received, SET_DISTANCE, do_set_distance},
    {SET_DISTANCE, check_new_measurement, TRIGGER_START, do_start_new_measurement},
    {SET_DISTANCE, check_off, WAIT_START, do_stop_measurement},
    {-1, NULL, -1, NULL},
};

/* Other auxiliary functions */
/**
 * @brief Initialize a ultrasound FSM.
 * 
 * This function initializes the default values of the FSM struct and calls to the port to initialize the associated HW given the ID.
 * 
 * The FSM stores the distance of the last ultrasound trigger. The user should ask for it using the function `fsm_ultrasound_get_distance()`.
 * 
 * The FSM contains information of the ultrasound ID. This ID is a unique identifier that is managed by the user in the port. That is where the user provides identifiers and HW information for all the ultrasounds on his system. The FSM does not have to know anything of the underlying HW.
 * 
 * @param p_fsm_ultrasound 
 * @param ultrasound_id 
 */
void fsm_ultrasound_init(fsm_ultrasound_t *p_fsm_ultrasound, uint32_t ultrasound_id)
{
    // 1. Call the fsm_init() to initialize the FSM. Pass the address of the fsm_t struct and the transition table.
    fsm_init(&p_fsm_ultrasound->f, fsm_trans_ultrasound);

    // 2. Initialize the fields of the p_fsm: distance_cm, distance_idx, and distance_arr, to 0. To do the latter, you can use the function memset().
    p_fsm_ultrasound->distance_cm = 0;
    p_fsm_ultrasound->distance_idx = 0;
    memset(p_fsm_ultrasound->distance_arr, 0, sizeof(p_fsm_ultrasound->distance_arr));

    // 3. Initialize the field status and new_measurement to false.
    p_fsm_ultrasound->status = false;
    p_fsm_ultrasound->new_measurement = false;
    p_fsm_ultrasound->ultrasound_id = ultrasound_id;

    // 4. Call function `port_ultrasound_init()` to initialize the HW of the ultrasound sensor.
    port_ultrasound_init(ultrasound_id);
}

/* Public functions -----------------------------------------------------------*/
fsm_ultrasound_t *fsm_ultrasound_new(uint32_t ultrasound_id)
{
    fsm_ultrasound_t *p_fsm_ultrasound = malloc(sizeof(fsm_ultrasound_t)); /* Do malloc to reserve memory of all other FSM elements, although it is interpreted as fsm_t (the first element of the structure) */
    fsm_ultrasound_init(p_fsm_ultrasound, ultrasound_id);                  /* Initialize the FSM */
    return p_fsm_ultrasound;
}

// Other auxiliary functions
void fsm_ultrasound_destroy(fsm_ultrasound_t *p_fsm_ultrasound)
{
    // 1. Implement this function analogously to the `fsm_button_destroy()` function.
    free(&p_fsm_ultrasound->f);
}

void fsm_ultrasound_fire(fsm_ultrasound_t *p_fsm)
{
    // 1. Call function fsm_fire() with the received pointer to fsm_t.
    fsm_fire(&p_fsm->f);
}

void fsm_ultrasound_set_state(fsm_ultrasound_t *p_fsm, int8_t state)
{
    p_fsm->f.current_state = state;
}

fsm_t *fsm_ultrasound_get_inner_fsm(fsm_ultrasound_t *p_fsm)
{
    fsm_ultrasound_t *p_fsm_ultrasound = (fsm_ultrasound_t *)p_fsm;

    // 1. Return the address of the f field of the struct.
    return &(p_fsm_ultrasound->f);
}

uint32_t fsm_ultrasound_get_state(fsm_ultrasound_t *p_fsm)
{
    fsm_ultrasound_t *p_fsm_ultrasound = (fsm_ultrasound_t *)p_fsm;

    // 1. Retrieve and return the field current_state of the FSM (field f of the struct).
    return p_fsm_ultrasound->f.current_state;
}

uint32_t fsm_ultrasound_get_distance(fsm_ultrasound_t *p_fsm)
{
    fsm_ultrasound_t *p_fsm_ultrasound = (fsm_ultrasound_t *)p_fsm;

    // 1. Retrieve and return the field distance_cm.
    uint32_t distance = p_fsm_ultrasound->distance_cm;

    // 2. Reset the field new_measurement.
    p_fsm_ultrasound->new_measurement = false;

    return distance;
}

void fsm_ultrasound_stop(fsm_ultrasound_t *p_fsm)
{
    fsm_ultrasound_t *p_fsm_ultrasound = (fsm_ultrasound_t *)p_fsm;

    // 1. Reset the field status.
    p_fsm_ultrasound->status = false;

    // 2. Call function `port_ultrasound_stop_ultrasound()` with the right parameters.
    port_ultrasound_stop_ultrasound(p_fsm_ultrasound->ultrasound_id);
}

void fsm_ultrasound_start(fsm_ultrasound_t *p_fsm)
{
    fsm_ultrasound_t *p_fsm_ultrasound = (fsm_ultrasound_t *)p_fsm;

    // 1. Set the field status.
    p_fsm_ultrasound->status = true;

    // 2. Reset the field distance_idx.
    p_fsm_ultrasound->distance_idx = 0;

    // 3. Reset the field distance_cm.
    p_fsm_ultrasound->distance_cm = 0;

    // 4. Call function `port_ultrasound_reset_echo_ticks()` with the right parameters.
    port_ultrasound_reset_echo_ticks(p_fsm_ultrasound->ultrasound_id);

    // 5. Call function `port_ultrasound_set_trigger_ready()` with the right parameters to indicate that the ultrasound sensor is ready to start a new measurement.
    port_ultrasound_set_trigger_ready(p_fsm_ultrasound->ultrasound_id, true);

    // 6. Call function `port_ultrasound_start_new_measurement_timer()` to force the new measurement timer to start to provoke the first interrupt.
    port_ultrasound_start_new_measurement_timer();
}

bool fsm_ultrasound_get_status(fsm_ultrasound_t *p_fsm)
{
    fsm_ultrasound_t *p_fsm_ultrasound = (fsm_ultrasound_t *)p_fsm;

    // 1. Retrieve and return the field status.
    bool field_status = p_fsm_ultrasound->status;
    return field_status;
}

void fsm_ultrasound_set_status(fsm_ultrasound_t *p_fsm, bool status)
{
    fsm_ultrasound_t *p_fsm_ultrasound = (fsm_ultrasound_t *)p_fsm;

    // 1. Update the field status with the received value
    p_fsm_ultrasound->status = status;
}

bool fsm_ultrasound_get_ready(fsm_ultrasound_t *p_fsm)
{
    fsm_ultrasound_t *p_fsm_ultrasound = (fsm_ultrasound_t *)p_fsm;

    // 1. Call function `port_ultrasound_get_trigger_ready()` with the ultrasound ID and return the result.
    bool id = port_ultrasound_get_trigger_ready(p_fsm_ultrasound->ultrasound_id);
    return id;
}

bool fsm_ultrasound_get_new_measurement_ready(fsm_ultrasound_t *p_fsm)
{
    fsm_ultrasound_t *p_fsm_ultrasound = (fsm_ultrasound_t *)p_fsm;

    // 1. Retrieve and return the field new_measurement.
    bool measure = p_fsm_ultrasound->new_measurement;
    return measure;
}

bool fsm_ultrasound_check_activity (fsm_ultrasound_t *p_fsm)
{
    // 1. Return false always.
    return false;
}