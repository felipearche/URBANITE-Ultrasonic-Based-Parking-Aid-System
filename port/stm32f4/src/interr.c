/**
 * @file interr.c
 * @brief Interrupt service routines for the STM32F4 platform.
 * @author SDG2. Román Cárdenas (r.cardenas@upm.es) and Josué Pagán (j.pagan@upm.es)
 * @date 2025-01-01
 */
// Include HW dependencies:
#include "port_system.h"
#include "stm32f4_system.h"
#include "port_button.h"
#include "port_ultrasound.h"
#include "stm32f4_button.h"
#include "stm32f4_ultrasound.h"

// Include headers of different port elements:

//------------------------------------------------------
// INTERRUPT SERVICE ROUTINES
//------------------------------------------------------
/**
 * @brief Interrupt service routine for the System tick timer (SysTick).
 *
 * @note This ISR is called when the SysTick timer generates an interrupt.
 * The program flow jumps to this ISR and increments the tick counter by one millisecond.
 *
 * > **TO-DO alumnos:**
 * >
 * > ✅ 1. **Increment the System tick counter `msTicks` in 1 count.** To do so, use the function `port_system_get_millis()` and `port_system_set_millis()`.
 *
 * @warning **The variable `msTicks` must be declared volatile!** Just because it is modified by a call of an ISR, in order to avoid [*race conditions*](https://en.wikipedia.org/wiki/Race_condition). **Added to the definition** after *static*.
 *
 */
void SysTick_Handler(void)
{
    // 1. Call the function port_system_get_millis() in the ISR SysTick_Handler() to read the current value of msTicks and store it in a local variable. Don't forget to declare the variable.
    uint32_t current_ticks = port_system_get_millis();

    // 2. Call the function port_system_set_millis() in the ISR SysTick_Handler() to write the value of msTicks with the previous value incremented by 1.
    port_system_set_millis(current_ticks + 1);
}

/**
 * @brief This function handles Px10-Px15 global interrupts.
 *
 * First, this function identifies the line/ pin which has raised the interruption. Then, perform the desired action. Before leaving it cleans the interrupt pending register.
 *
 */
void EXTI15_10_IRQHandler(void)
{
    /* ISR parking button */
    if (port_button_get_pending_interrupt(PORT_PARKING_BUTTON_ID))
    {
        // aqui
        // 1. Call the function `port_system_systick_resume()` to resume the systick timer at the beginning of the ISR
        port_system_systick_resume();
        // 1. Retrieve the values of the GPIO of the user button using the PORT_PARKING_BUTTON_ID index to get the port and pin from the buttons_arr using its getter function `port_button_get_value()`.
        bool button_value = port_button_get_value(PORT_PARKING_BUTTON_ID);

        // 2. If the value is true means that the button as been released, therefore set the field flag_pressed to false
        if (button_value)
        {
            port_button_set_pressed(PORT_PARKING_BUTTON_ID, false);
        }

        // 3. If the value is false means that the button as been pressed, therefore set the field flag_pressed to true
        else
        {
            port_button_set_pressed(PORT_PARKING_BUTTON_ID, true);
        }

        // 4. Clean the corresponding bit of the PR register
        port_button_clear_pending_interrupt(PORT_PARKING_BUTTON_ID);
    }
}

/**
 * @brief Interrupt service routine for TIM2.
 *
 * This timer controls the duration of the echo signal of the ultrasound sensor by means of the input capture mode.
 *
 * @note This ISR is called when the TIM2 timer generates an interrupt.
 * The program flow jumps to this ISR and handles the timer interrupt.
 */
void TIM2_IRQHandler(void)
{
    // 1. Call the function `port_system_systick_resume()` to resume the systick timer at the beginning of the ISR
    port_system_systick_resume();

    // 1. Check if the UIF flag is set. If so, this means that the ARR register has overflowed.
    if (TIM2->SR & TIM_SR_UIF)
    {
        // 1. Call the function `port_system_systick_resume()` to resume the systick timer at the beginning of the ISR
        // Increment the echo_overflows counter.
        uint32_t overflows = port_ultrasound_get_echo_overflows(PORT_REAR_PARKING_SENSOR_ID);
        port_ultrasound_set_echo_overflows(PORT_REAR_PARKING_SENSOR_ID, overflows + 1);

        // Remember to clear the UIF flag.
        TIM2->SR &= ~TIM_SR_UIF;
    }

    // 2. Check if the CCxIF flag is set. If so, this means that the input capture event has occurred.
    if (TIM2->SR & TIM_SR_CC2IF)
    {
        // Read the value of the CCRx register to get the current tick.
        uint32_t current_tick = TIM2->CCR2;

        // If both the echo_init_tick and echo_end_tick are 0, this means that the echo signal has not started yet.
        if (port_ultrasound_get_echo_init_tick(PORT_REAR_PARKING_SENSOR_ID) == 0 && port_ultrasound_get_echo_end_tick(PORT_REAR_PARKING_SENSOR_ID) == 0)
        {
            // Update the echo_init_tick with the current tick.
            port_ultrasound_set_echo_init_tick(PORT_REAR_PARKING_SENSOR_ID, current_tick);
        }
        else
        {
            // Update the echo_end_tick with the current tick and set the echo_received flag to true.
            port_ultrasound_set_echo_end_tick(PORT_REAR_PARKING_SENSOR_ID, current_tick);
            port_ultrasound_set_echo_received(PORT_REAR_PARKING_SENSOR_ID, true);
        }

        // Clear the CCxIF flag.
        TIM2->SR &= ~TIM_SR_CC2IF;
    }
}

/**
 * @brief Interrupt service routine for TIM3.
 *
 * @note This ISR is called when the TIM3 timer generates an interrupt.
 * The program flow jumps to this ISR and handles the timer interrupt.
 */
void TIM3_IRQHandler(void)
{
    // 1. Clear the interrupt flag UIF in the status register SR.
    TIM3->SR &= ~TIM_SR_UIF;

    // 2. Call the function `port_ultrasound_set_trigger_end()` to set the flag that indicates that the time of the trigger signal has expired.
    port_ultrasound_set_trigger_end(PORT_REAR_PARKING_SENSOR_ID, true);
}

/**
 * @brief Interrupt service routine for the TIM5 timer.
 *
 * This timer controls the duration of the measurements of the ultrasound sensor. When the interrupt occurs it means that the time of the a measurement has expired and a new measurement can be started.
 *
 */
void TIM5_IRQHandler(void)
{
    // 1. Clear the interrupt flag UIF in the status register SR.
    TIM5->SR &= ~TIM_SR_UIF;

    // 2. Call the function `port_ultrasound_set_trigger_ready()` to set the flag that indicates that a new measurement can be started.
    port_ultrasound_set_trigger_ready(PORT_REAR_PARKING_SENSOR_ID, true);
}