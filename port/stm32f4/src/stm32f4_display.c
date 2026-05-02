/**
 * @file stm32f4_display.c
 * @brief Portable functions to interact with the display system FSM library. All portable functions must be implemented in this file.
 * @author Felipe Fernández-Arche Pineda
 * @author alumno2
 * @date 09/04/2025
 */

/* Standard C includes */
#include <stdio.h>

/* HW dependent includes */
#include "port_display.h"
#include "port_system.h"

/* Microcontroller dependent includes */
#include "stm32f4_system.h"
#include "stm32f4_display.h"

/* Defines --------------------------------------------------------------------*/

/* Typedefs --------------------------------------------------------------------*/
/**
 * @brief Structure to define the HW dependencies of an RGB LED.
 * 
 */
typedef struct
{
    GPIO_TypeDef *p_port_red; /*!< GPIO where the RED LED is connected */
    uint8_t pin_red; /*!< Pin/line where the RED LED is connected */
    GPIO_TypeDef *p_port_green; /*!< GPIO where the GREEN LED is connected */
    uint8_t pin_green; /*!< Pin/line where the GREEN LED is connected */
    GPIO_TypeDef *p_port_blue; /*!< GPIO where the BLUE LED is connected */
    uint8_t pin_blue; /*!< Pin/line where the BLUE LED is connected */
} stm32f4_display_hw_t;

/* Global variables */
static stm32f4_display_hw_t displays_arr[] = {
    [PORT_REAR_PARKING_DISPLAY_ID] = {
        .p_port_red = STM32F4_REAR_PARKING_DISPLAY_RGB_R_GPIO,
        .pin_red = STM32F4_REAR_PARKING_DISPLAY_RGB_R_PIN,
        .p_port_green = STM32F4_REAR_PARKING_DISPLAY_RGB_G_GPIO,
        .pin_green = STM32F4_REAR_PARKING_DISPLAY_RGB_G_PIN,
        .p_port_blue = STM32F4_REAR_PARKING_DISPLAY_RGB_B_GPIO,
        .pin_blue = STM32F4_REAR_PARKING_DISPLAY_RGB_B_PIN,
    },
};

/* Private functions -----------------------------------------------------------*/
/**
 * @brief Get the display struct with the given ID.
 * 
 * @param display_id Button ID.
 * @return true Pointer to the display struct.
 * @return false NULL If the display ID is not valid.
 */
stm32f4_display_hw_t *_stm32f4_display_get(uint32_t display_id) 
{
    // Return the pointer to the display with the given ID. If the ID is not valid, return NULL.
    // TO-DO alumnos
    if (display_id < (sizeof(displays_arr) / sizeof(displays_arr[0]))) {
        return &displays_arr[display_id];
    }
    return NULL;
}

/**
 * @brief Configure the timer that controls the PWM of each one of the RGB LEDs of the display system.
 * 
 * This function is called by the `port_display_init()` public function to configure the timer that controls the PWM of the RGB LEDs of the display.
 * 
 * @param display_id Display system identifier number.
 */
void _timer_pwm_config (uint32_t display_id)
{
    // Retrieve the display configuration struct calling _stm32f4_display_get().
    stm32f4_display_hw_t *display = _stm32f4_display_get(display_id);
    if (display == NULL) {
        // Invalid display ID, return without doing anything
        return;
    }

    // 1. Enable the clock source of the timer
    RCC->APB1ENR |= RCC_APB1ENR_TIM4EN; // Correct timer for PB6, PB8, PB9

    // 2. Disable the counter (register CR1) and enable the autoreload preload (bit ARPE)
    TIM4->CR1 &= ~TIM_CR1_CEN; // Disable the counter
    TIM4->CR1 |= TIM_CR1_ARPE; // Enable autoreload preload

    // 3. Reset the counter (register CNT), set the autoreload value (register ARR) and the prescaler (register PSC) for a frequency of 50 Hz.
    TIM4->CNT = 0; // Reset the counter
    TIM4->PSC = (SystemCoreClock / 10000) - 1; // Prescaler to reduce clock to 10 kHz
    // Set ARR to achieve 50 Hz from the reduced clock
    TIM4->ARR = (10000 / 50) - 1; // ARR for 50 Hz

    // 4. Configure GPIO pins PB6 (CH1), PB8 (CH3), PB9 (CH4) in alternate function mode with AF2
    stm32f4_system_gpio_config_alternate(display->p_port_red, display->pin_red, STM32F4_AF2);
    stm32f4_system_gpio_config_alternate(display->p_port_green, display->pin_green, STM32F4_AF2);
    stm32f4_system_gpio_config_alternate(display->p_port_blue, display->pin_blue, STM32F4_AF2);

    // 5. Disable the output compare (register CCER) for each one of the corresponding channels.
    TIM4->CCER &= ~(TIM_CCER_CC1E | TIM_CCER_CC3E | TIM_CCER_CC4E); // Disable channels 1, 2, and 3

    // 6. Clear the P and NP bits (CCxP and CCxNP) of the output compare register (CCER) for each one of the corresponding channels.
    TIM4->CCER &= ~(TIM_CCER_CC1P | TIM_CCER_CC1NP | TIM_CCER_CC3P | TIM_CCER_CC3NP | TIM_CCER_CC4P | TIM_CCER_CC4NP);

    // 7. Set both (i) mode PWM 1, and (ii) enable preload (register CCMRx) for each one of the corresponding channels.
    // CH1 (Red - PB6)
    TIM4->CCMR1 |= (6 << TIM_CCMR1_OC1M_Pos) | TIM_CCMR1_OC1PE;
    // CH3 (Green - PB8)
    TIM4->CCMR2 |= (6 << TIM_CCMR2_OC3M_Pos) | TIM_CCMR2_OC3PE;
    // CH4 (Blue - PB9)
    TIM4->CCMR2 |= (6 << TIM_CCMR2_OC4M_Pos) | TIM_CCMR2_OC4PE;

    // 8. Generate an update event (register EGR) by setting the UG bit.
    TIM4->EGR |= TIM_EGR_UG; // Generate an update event

    // Enable the timer counter
    TIM4->CR1 |= TIM_CR1_CEN;
}

/**
 * @brief Configure the HW specifications of a given display.
 * 
 * @param display_id Display ID. This index is used to select the element of the displays_arr[] array
 */
void port_display_init (uint32_t display_id)
{
    // 1. Retrieve the display configuration struct calling _stm32f4_display_get().
    stm32f4_display_hw_t *display = _stm32f4_display_get(display_id);
    if (display == NULL) {
        // Invalid display ID, return without doing anything
        return;
    }

    // 2. Call function stm32f4_system_gpio_config() with the right arguments to configure each RGB LED as in alternate mode and no pull up neither pull down connection.
    stm32f4_system_gpio_config(display->p_port_red, display->pin_red, STM32F4_GPIO_MODE_AF, STM32F4_GPIO_PUPDR_NOPULL);
    stm32f4_system_gpio_config(display->p_port_green, display->pin_green, STM32F4_GPIO_MODE_AF, STM32F4_GPIO_PUPDR_NOPULL);
    stm32f4_system_gpio_config(display->p_port_blue, display->pin_blue, STM32F4_GPIO_MODE_AF, STM32F4_GPIO_PUPDR_NOPULL);

    // 3. Call function stm32f4_system_gpio_config_alternate() with the right arguments to configure the alternate function of the each RGB LED.
    stm32f4_system_gpio_config_alternate(display->p_port_red, display->pin_red, STM32F4_AF2);
    stm32f4_system_gpio_config_alternate(display->p_port_green, display->pin_green, STM32F4_AF2);
    stm32f4_system_gpio_config_alternate(display->p_port_blue, display->pin_blue, STM32F4_AF2);

    // 4. Call function _timer_pwm_config() to configure the timer and the PWM signal of the display.
    _timer_pwm_config(display_id);

    // 5. Call function port_display_set_rgb() to set the RGB LED to off.
    port_display_set_rgb(display_id, (rgb_color_t){0, 0, 0});

    // Disable the timer after configuration
    TIM4->CR1 &= ~TIM_CR1_CEN; // Ensure the timer is disabled
}

/**
 * @brief Set the Capture/Compare register values for each channel of the RGB LED given a color.
 * 
 * This function disables the timer associated to the RGB LEDs, sets the Capture/Compare register values for each channel of the RGB LED, and enables the timer.
 * 
 * @param display_id Display system identifier number.
 * @param color RGB color to set.
 */
void port_display_set_rgb (uint32_t display_id, rgb_color_t color)
{
    // 1. Retrieve the individual RGB values from the color parameter and follow the flowchart to set the RGB LED color by configuring the PWM signals appropriately.
    
    // Check if the display ID is the one we want to handle
    if (display_id != PORT_REAR_PARKING_DISPLAY_ID) {
        return;
    }

    // Disable the timer before making any changes (CR1 register, bit CEN = 0)
    TIM4->CR1 &= ~TIM_CR1_CEN;

    // Retrieve the individual RGB components
    uint8_t r = color.r;
    uint8_t g = color.g;
    uint8_t b = color.b;

    // If all RGB values are 0, disable the capture/compare register for all channels (CCER register)
    if (r == 0 && g == 0 && b == 0) {
        TIM4->CCER &= ~(TIM_CCER_CC1E | TIM_CCER_CC3E | TIM_CCER_CC4E);
    } else {
        // If red component is 0, disable the corresponding channel
        if (r == 0) {
            TIM4->CCER &= ~TIM_CCER_CC1E;
        } else {
            // For the channel of the red LED:
            // · Set the duty cycle in compare/compare register (CCR1)
            // · Enable the channel (CCER register)
            TIM4->CCR1 = (r * TIM4->ARR) / 255; // Scale RGB value to ARR
            TIM4->CCER |= TIM_CCER_CC1E;
        }

        // If green component is 0, disable the corresponding channel
        if (g == 0) {
            TIM4->CCER &= ~TIM_CCER_CC3E;
        } else {
            // For the channel of the green LED:
            // · Set the duty cycle in compare/compare register (CCR3)
            // · Enable the channel (CCER register)
            TIM4->CCR3 = (g * TIM4->ARR) / 255; // Scale RGB value to ARR
            TIM4->CCER |= TIM_CCER_CC3E;
        }

        // If blue component is 0, disable the corresponding channel
        if (b == 0) {
            TIM4->CCER &= ~TIM_CCER_CC4E;
        } else {
            // For the channel of the blue LED:
            // · Set the duty cycle in compare/compare register (CCR4)
            // · Enable the channel (CCER register)
            TIM4->CCR4 = (b * TIM4->ARR) / 255; // Scale RGB value to ARR
            TIM4->CCER |= TIM_CCER_CC4E;
        }
    }

    // Set the UG bit in the EGR register to update all settings
    TIM4->EGR |= TIM_EGR_UG;

    // Enable the timer (CR1 register, bit CEN = 1)
    TIM4->CR1 |= TIM_CR1_CEN;
}