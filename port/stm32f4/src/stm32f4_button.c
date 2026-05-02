/**
 * @file stm32f4_button.c
 * @brief Portable functions to interact with the button FSM library. All portable functions must be implemented in this file.
 * @author Felipe Fernández-Arche
 * @author David Pedraza
 * @date 26/03/2025
 */

/* Includes ------------------------------------------------------------------*/
/* Standard C includes */
#include <stddef.h>  // Defines NULL

/* HW dependent includes */
#include "port_button.h" // Used to get general information about the buttons (ID, etc.)
#include "port_system.h" // Used to get the system tick

/* Microcontroller dependent includes */
// TO-DO alumnos: include the necessary files to interact with the GPIOs
#include "stm32f4_system.h"
#include "stm32f4_button.h"

/* Typedefs --------------------------------------------------------------------*/
/**
 * @brief Structure to define the HW dependencies of a button status.
 * 
 */
typedef struct
{
    GPIO_TypeDef *p_port; /*!< GPIO where the button is connected */
    uint8_t pin; /*!< Pin/line where the button is connected */
    uint8_t pupd_mode; /*!< Pull-up/Pull-down mode */
    bool flag_pressed; /*!< Flag to indicate that the button has been pressed */
} stm32f4_button_hw_t;

/* Global variables ------------------------------------------------------------*/
/**
 * @brief Array of elements that represents the HW characteristics of the buttons connected to the STM32F4 platform.
 * 
 * This must be hidden from the user, so it is declared as static. To access the elements of this array, use the function `_stm32f4_button_get()`.
 * 
 * @hideinitializer
 */
static stm32f4_button_hw_t buttons_arr[] = {
    [PORT_PARKING_BUTTON_ID] = {.p_port = STM32F4_PARKING_BUTTON_GPIO, .pin = STM32F4_PARKING_BUTTON_PIN, .pupd_mode = STM32F4_GPIO_PUPDR_NOPULL},
    
};

/* Private functions ----------------------------------------------------------*/
/**
 * @brief Get the button status struct with the given ID.
 *
 * @param button_id Button ID.
 *
 * @return Pointer to the button state struct.
 * @return NULL If the button ID is not valid.
 */
stm32f4_button_hw_t *_stm32f4_button_get(uint32_t button_id)
{
    // Return the pointer to the button with the given ID. If the ID is not valid, return NULL.
    if (button_id < sizeof(buttons_arr) / sizeof(buttons_arr[0]))
    {
        return &buttons_arr[button_id];
    }
    else
    {
        return NULL;
    }
}

/* Public functions -----------------------------------------------------------*/
void stm32f4_button_set_new_gpio(uint32_t button_id, GPIO_TypeDef *p_port, uint8_t pin)
{
    stm32f4_button_hw_t *p_button = _stm32f4_button_get(button_id);
    p_button->p_port = p_port;
    p_button->pin = pin;
}

void port_button_init(uint32_t button_id)
{
    // 1. Retrieve the button configuration struct calling `_stm32f4_button_get()`
    stm32f4_button_hw_t *p_button = _stm32f4_button_get(button_id);
    if (p_button == NULL) {
        return;  // Evitar acceso inválido a memoria
    }

    // 2. Call function `stm32f4_system_gpio_config()` with the right arguments to configure the button as input and no pull up neither pull down connection
    stm32f4_system_gpio_config(p_button->p_port, p_button->pin, STM32F4_GPIO_MODE_IN, STM32F4_GPIO_PUPDR_NOPULL);

    // 3. Call function `stm32f4_system_gpio_config_exti()` with the right arguments to configure interruption mode in both rising and falling edges, and enable the interrupt request
    stm32f4_system_gpio_config_exti(p_button->p_port, p_button->pin, STM32F4_TRIGGER_BOTH_EDGE | STM32F4_TRIGGER_ENABLE_INTERR_REQ);

    // 4. Call function `stm32f4_system_gpio_exti_enable()` with the right parameters to enable the interrupt line and set the priority level to 1 and the subpriority level to 0
    stm32f4_system_gpio_exti_enable(p_button->pin, 1, 0);
}

bool port_button_get_pressed (uint32_t button_id)
{
    // Check if the button_id is within the valid range (0-15)
    if (button_id >=16) {
        return false;
    }

    // 1. Return the value of the field flag_pressed. Call the function `_stm32f4_button_get()` to get the button struct.
    stm32f4_button_hw_t *p_button = _stm32f4_button_get(button_id);
    if (p_button == NULL) {
        return false;
    }
    return p_button->flag_pressed;
}

bool port_button_get_value (uint32_t button_id)
{
    // Check if the button_id is within the valid range (0-15)
    if (button_id >= 16) {
        return false;
    }

    // 1. Call function `stm32f4_system_gpio_read()` with the right arguments to get the value of the button. Call the function `_stm32f4_button_get()` to get the button struct.
    stm32f4_button_hw_t *p_button = _stm32f4_button_get(button_id);
    if (p_button == NULL || p_button->p_port == NULL) {
        return false;
    }
    //stm32f4_system_gpio_read (p_button->p_port, p_button->pin);
    bool button_value = (p_button->p_port->IDR & (1 << p_button->pin)) != 0;
    return button_value; // Filtro anti-rebotes
}

void port_button_set_pressed (uint32_t button_id, bool pressed)
{
    // Check if the button_id is within the valid range (0-15)
    if (button_id >=16) {
        return;
    }

    // 1. Set the value of the field flag_pressed. Call the function `_stm32f4_button_get()` to get the button struct.
    stm32f4_button_hw_t *p_button = _stm32f4_button_get(button_id);
    if (p_button == NULL) {
        return;
    }
    p_button->flag_pressed = pressed;
}

bool port_button_get_pending_interrupt (uint32_t button_id)
{
    // Check if the button_id is within the valid range (0-15)
    if (button_id >= 16) {  
        return false;
    }

    // 1. Get the button configuration struct calling `_stm32f4_button_get()`
    stm32f4_button_hw_t *p_button = _stm32f4_button_get(button_id);
    if (p_button == NULL) {
        return false;
    }

    // 2. Get the pin of the button
    uint32_t button_pin = p_button->pin;

    // 3. Read the value of the PR register of the EXTI peripheral associated with the button pin
    bool pending_interrupt = (EXTI->PR & (1 << button_pin)) != 0;
    
    // 4. Return the value
    return pending_interrupt;
}

void port_button_clear_pending_interrupt (uint32_t button_id)
{
    // Check if the button_id is within the valid range (0-15)
    if (button_id >= 16) {  
        return;
    }

    // 1. Get the button configuration struct by calling `_stm32f4_button_get()`
    stm32f4_button_hw_t *p_button = _stm32f4_button_get(button_id);
    if (p_button == NULL) {
        return;
    }

    // 2. Get the pin of the button
    uint32_t button_pin = p_button->pin;

    // 3. Clear the corresponding bit of the PR register of the EXTI peripheral associated with the button pin
    if (EXTI->PR & (1 << button_pin)) {
        EXTI->PR = (1 << button_pin);
    }
}

void port_button_disable_interrupts(uint32_t button_id)
{
    // Check if the button_id is within the valid range (0-15)
    if (button_id >= 16) {  
        return;
    }

    // 1. Get the button configuration struct calling `_stm32f4_button_get()`
    stm32f4_button_hw_t *p_button = _stm32f4_button_get(button_id);
    if (p_button == NULL) {
        return;
    }

    // 2. Get the pin of the button
    uint32_t button_pin = p_button->pin;

    // 3. Disable the EXTI line associated with the button pin by calling the function `stm32f4_system_gpio_exti_disable()`
    stm32f4_system_gpio_exti_disable(button_pin);
}