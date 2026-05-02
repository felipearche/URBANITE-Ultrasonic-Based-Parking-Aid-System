/**
 * @file stm32f4_ultrasound.c
 * @brief Portable functions to interact with the ultrasound FSM library. All portable functions must be implemented in this file.
 * @author Felipe Fernández-Arche
 * @author David Pedraza
 * @date 26/03/2025
 */

/* Standard C includes */
#include <stdio.h>
#include <math.h>

/* HW dependent includes */
#include "stm32f4_system.h"
#include "stm32f4_ultrasound.h"

/* Microcontroller dependent includes */
#include "port_ultrasound.h"
#include "port_system.h"

/* Typedefs --------------------------------------------------------------------*/
/**
 * @brief Structure to define the HW dependencies of an ultrasound sensor.
 * 
 */
typedef struct
{
    GPIO_TypeDef *p_trigger_port; /*!< GPIO where the trigger signal is connected */
    GPIO_TypeDef *p_echo_port; /*!< GPIO where the echo signal is connected */
    uint8_t trigger_pin; /*!< Pin/line where the trigger signal is connected */
    uint8_t echo_pin; /*!< Pin/line where the echo signal is connected */
    uint8_t echo_alt_fun; /*!< Alternate function for the echo signal */
    bool trigger_ready; /*!< Flag to indicate that a new measurement can be started */
    bool trigger_end; /*!< Flag to indicate that the trigger signal has been sent */
    bool echo_received; /*!< Flag to indicate that the echo signal has been received */
    uint32_t echo_init_tick; /*!< Tick time when the echo signal was received */
    uint32_t echo_end_tick; /*!< Tick time when the echo signal ended */
    uint32_t echo_overflows; /*!< Number of overflows of the timer during the echo signal */
} stm32f4_ultrasound_hw_t;

/* Global variables */
/**
 * @brief Array of elements that represents the HW characteristics of the ultrasounds connected to the STM32F4 platform.
 * 
 * This must be hidden from the user, so it is declared as static. To access the elements of this array, use the function `_stm32f4_ultrasound_get()`.
 * 
 * @hideinitializer
 */
static stm32f4_ultrasound_hw_t ultrasounds_arr[] = {
    [PORT_REAR_PARKING_SENSOR_ID] = {
        .p_trigger_port = STM32F4_REAR_PARKING_SENSOR_TRIGGER_GPIO,
        .trigger_pin = STM32F4_REAR_PARKING_SENSOR_TRIGGER_PIN,
        .p_echo_port = STM32F4_REAR_PARKING_SENSOR_ECHO_GPIO,
        .echo_pin = STM32F4_REAR_PARKING_SENSOR_ECHO_PIN,
    },
};  

/* Private functions ----------------------------------------------------------*/
/**
 * @brief Get the ultrasound struct with the given ID.
 *
 * @param button_id Ultrasound ID.
 *
 * @return Pointer to the ultrasound struct.
 * @return NULL If the ultrasound ID is not valid.
 */
stm32f4_ultrasound_hw_t *_stm32f4_ultrasound_get(uint32_t ultrasound_id)
{
    // Return the pointer to the button with the given ID. If the ID is not valid, return NULL.
    if (ultrasound_id < sizeof(ultrasounds_arr) / sizeof(ultrasounds_arr[0]))
    {
        return &ultrasounds_arr[ultrasound_id];
    }
    else
    {
        return NULL;
    }
}

/**
 * @brief Configure the timer that controls the duration of the trigger signal.
 * 
 * This function configures the timer to generate internal interrupts to control the raise and fall of the trigger signal. The duration of the trigger signal is defined in the PORT_PARKING_SENSOR_TRIGGER_UP_US macro. This function is called by the `port_ultrasound_init()` public function to configure the timer that controls the duration of the trigger signal.
 * 
 */
static void _timer_trigger_setup(void)
{
    // 1. Enable the clock of the timer that controls the trigger signal. Check the reference manual of the STM32F4 to know if the timer is connected to the APB1 or APB2 bus (i.e. RCC->APB1ENR or RCC->APB2ENR).
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

    // 2. Disable the counter of the timer (register CR1 of the timer) because we want to configure it.
    TIM3->CR1 &= ~TIM_CR1_CEN;

    // 3. Enable the autoreload preload (bit ARPE of the register CR1) to enable the autoreload register.
    TIM3->CR1 |= TIM_CR1_ARPE;

    // 4. Set the counter of the timer to 0 (register CNT) to start counting from 0.
    TIM3->CNT = 0;

    // 5. Compute the prescaler and the auto-reload register to set the duration of the trigger signal. The duration of the trigger signal is defined in the PORT_PARKING_SENSOR_TRIGGER_UP_US macro.
    double system_clock = (double)SystemCoreClock;
    double trigger_duration_us = (double)PORT_PARKING_SENSOR_TRIGGER_UP_US;
    double psc = round(system_clock / (65535.0 * 1000000.0 / trigger_duration_us)) - 1.0;
    double arr = round(system_clock / ((psc + 1.0) * 1000000.0 / trigger_duration_us)) - 1.0;

    if (arr > 65535.0)
    {
        psc += 1.0;
        arr = round(system_clock / ((psc + 1.0) * 1000000.0 / trigger_duration_us)) - 1.0;
    }

    // 6. Load the values computed for ARR and PSC into the corresponding registers of the timer.
    TIM3->PSC = (uint32_t)psc;
    TIM3->ARR = (uint32_t)arr;

    // 7. The PSC and ARR values are currently in the preload registers. To load them into the active registers we need an update event. We achieve this by setting the UG bit of the EGR register. It is important to do this at the end of the configuration of the timer.
    TIM3->EGR |= TIM_EGR_UG;

    // 8. Clear the update interrupt flag (bit UIF of the register SR) to avoid an unwanted interrupt.
    TIM3->SR &= ~TIM_SR_UIF;

    // 9. Enable the interrupts of the timer by setting the UIE bit of the DIER register.
    TIM3->DIER |= TIM_DIER_UIE;

    // 10. Set the priority of the timer interrupt in the NVIC using the NVIC_SetPriority() function and the TIMx_IRQn interrupt with the level of priority and sub-priority shown in the main page.
    NVIC_SetPriority(TIM3_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 4, 0));
}

/**
 * @brief Configure the timer that controls the duration of the echo signal.
 * 
 * This function configures the timer as input capture to measure the duration of the echo signal. `port_ultrasound_init()` public function to configure the timer.
 * 
 * @param ultrasound_id Ultrasound ID. This ID is used to configure the timer that controls the echo signal of the ultrasound sensor.
 */
void _timer_echo_setup (uint32_t ultrasound_id)
{
    // 1. Enable the clock of the timer that controls the echo signal. Check the reference manual of the STM32F4 to know if the timer is connected to the APB1 or APB2 bus (i.e. RCC->APB1ENR or RCC->APB2ENR).
    if (ultrasound_id == PORT_REAR_PARKING_SENSOR_ID)
    {
        RCC->APB1ENR |= RCC_APB1ENR_TIM2EN; // Enable TIM2
    }

    // 2. Disable the counter of the timer (register CR1 of the timer) because we want to configure it.
    TIM2->CR1 &= ~TIM_CR1_CEN;

    // 3. Set the values of the prescaler and the auto-reload registers. Configure PSC for a resolution of 1 us (1 MHz) and ARR high enough to avoid too many overflows, e.g. it maximum value.
    TIM2->PSC = (uint32_t)(SystemCoreClock / 1000000) - 1; // 1 MHz resolution
    TIM2->ARR = 0xFFFF; // Maximum value to avoid too many overflows

    // 4. Set the auto-reload preload bit (ARPE) in the control register (CR1) to enable the auto-reload register. Also enable the update generation bit (UG) in the event generation register (EGR) to force the update of the registers.
    TIM2->CR1 |= TIM_CR1_ARPE;
    TIM2->EGR |= TIM_EGR_UG;

    // 5. Set the direction as input in the Capture/Compare mode register (CCMRx). x is 1 for channels 1 and 2, and 2 for channels 3 and 4. Check the table "Table 11. Alternate function" in the datasheet to identify the channel related to timer associated to the pin of the echo signal.
    TIM2->CCMR1 &= ~(TIM_CCMR1_CC2S);
    TIM2->CCMR1 |= (1 << TIM_CCMR1_CC2S_Pos);

    // 6. Disable digital filtering by clearing the ICxF bits in the Capture/Compare mode register (CCMRx).
    TIM2->CCMR1 &= ~(TIM_CCMR1_IC2F);

    // 7. Select the edge of the active transition in the Capture/Compare enable register (CCER). Set the bits CCxNP and CCxP to detect both rising and falling edges.
    TIM2->CCER |= TIM_CCER_CC2P | TIM_CCER_CC2NP;

    // 8. Program the input prescaler to capture each valid transition. Set the ICxPSC bits in the Capture/Compare mode register (CCMRx) to 0.
    TIM2->CCMR1 &= ~(TIM_CCMR1_IC2PSC);

    // 9. Enable the Capture/compare enable register (CCER) for the corresponding channel.
    TIM2->CCER |= TIM_CCER_CC2E;

    // 10. Enable the Capture/Compare interrupts bit (CCxIE) for the corresponding channel in the DMA/interrupt enable register (DIER).
    TIM2->DIER |= TIM_DIER_CC2IE;

    // 11. Enable the update interrupt bit (UIE) in the DMA/interrupt enable register (DIER).
    TIM2->DIER |= TIM_DIER_UIE;

    // 12. Set the priority of the timer interrupt in the NVIC using the NVIC_SetPriority() function and the TIMx_IRQn interrupt with the level of priority and sub-priority shown in the main page.
    NVIC_SetPriority(TIM2_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 3, 0));
}

/**
 * @brief Configure the timer that controls the duration of the new measurement.
 * 
 * This function configures the timer to generate an internal interrupt to control the duration of a measurement. The duration of a measurement is defined in the PORT_PARKING_SENSOR_TIMEOUT_MS macro. This function is called by the `port_ultrasound_init()` public function to configure the timer that controls the duration of the new measurement.
 * 
 */
void _timer_new_measurement_setup(void)
{
    // 1. Follow the same steps as in the `_timer_trigger_setup()` function to configure the timer that controls the duration of the new measurement.
    RCC->APB1ENR |= RCC_APB1ENR_TIM5EN;

    TIM5->CR1 &= ~TIM_CR1_CEN;

    TIM5->CR1 |= TIM_CR1_ARPE;

    TIM5->CNT = 0;

    double system_clock = (double)SystemCoreClock;
    double timeout_duration_ms = (double)PORT_PARKING_SENSOR_TIMEOUT_MS;
    double psc = round(system_clock / (65535.0 * 1000.0 / timeout_duration_ms)) - 1.0;
    double arr = round(system_clock / ((psc + 1.0) * 1000.0 / timeout_duration_ms)) - 1.0;

    if (arr > 65535.0)
    {
        psc += 1.0;
        arr = round(system_clock / ((psc + 1.0) * 1000.0 / timeout_duration_ms)) - 1.0;
    }

    TIM5->PSC = (uint32_t)psc;
    TIM5->ARR = (uint32_t)arr;

    TIM5->EGR |= TIM_EGR_UG;

    TIM5->SR &= ~TIM_SR_UIF;

    TIM5->DIER |= TIM_DIER_UIE;

    NVIC_SetPriority(TIM5_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 5, 0));
}

/* Public functions -----------------------------------------------------------*/
void stm32f4_ultrasound_set_new_trigger_gpio(uint32_t ultrasound_id, GPIO_TypeDef *p_port, uint8_t pin)
{
    stm32f4_ultrasound_hw_t *p_ultrasound = _stm32f4_ultrasound_get(ultrasound_id);
    p_ultrasound->p_trigger_port = p_port;
    p_ultrasound->trigger_pin = pin;
}

void stm32f4_ultrasound_set_new_echo_gpio(uint32_t ultrasound_id, GPIO_TypeDef *p_port, uint8_t pin)
{
    stm32f4_ultrasound_hw_t *p_ultrasound = _stm32f4_ultrasound_get(ultrasound_id);
    p_ultrasound->p_echo_port = p_port;
    p_ultrasound->echo_pin = pin;
}

void port_ultrasound_set_trigger_ready(uint32_t ultrasound_id, bool trigger_ready)
{
    /* Get the ultrasound sensor */
    stm32f4_ultrasound_hw_t *p_ultrasound = _stm32f4_ultrasound_get(ultrasound_id);

    if (p_ultrasound != NULL)
    {
        // 1. Set the value of the field trigger_ready with the received value.
        p_ultrasound->trigger_ready = trigger_ready;
    }
}

void port_ultrasound_set_trigger_end(uint32_t ultrasound_id, bool trigger_end)
{
    /* Get the ultrasound sensor */
    stm32f4_ultrasound_hw_t *p_ultrasound = _stm32f4_ultrasound_get(ultrasound_id);

    if (p_ultrasound != NULL)
    {
        // 1. Set the value of the field trigger_end with the received value.
        p_ultrasound->trigger_end = trigger_end;
    }
}

void port_ultrasound_init(uint32_t ultrasound_id)
{
    /* Get the ultrasound sensor */
    stm32f4_ultrasound_hw_t *p_ultrasound = _stm32f4_ultrasound_get(ultrasound_id);

    // 1. Initialize the fields of the ultrasound struct. Set the variables related to ticks to 0 and the flags to false but the trigger_ready to true.
    if (p_ultrasound == NULL) {
        return;
    }

    p_ultrasound->trigger_ready = true;
    p_ultrasound->trigger_end = false;
    p_ultrasound->echo_received = false;
    p_ultrasound->echo_init_tick = 0;
    p_ultrasound->echo_end_tick = 0;
    p_ultrasound->echo_overflows = 0;

    /* Trigger pin configuration */
    // 2. Configure the trigger pin as output and no pull up neither pull down connection.
    stm32f4_system_gpio_config(p_ultrasound->p_trigger_port, p_ultrasound->trigger_pin, STM32F4_GPIO_MODE_OUT, STM32F4_GPIO_PUPDR_NOPULL);

    /* Echo pin configuration */
    // 3. Configure the echo pin as alternate function and no pull up neither pull down connection.
    stm32f4_system_gpio_config(p_ultrasound->p_echo_port, p_ultrasound->echo_pin, STM32F4_GPIO_MODE_AF, STM32F4_GPIO_PUPDR_NOPULL);

    // 4. Configure the alternate function of the echo pin.
    stm32f4_system_gpio_config_alternate(p_ultrasound->p_echo_port, p_ultrasound->echo_pin, STM32F4_AF1);

    /* Configure timers */
    // 5. Call the 3 private functions that configure the timers of the trigger, echo and new measurement.
    _timer_trigger_setup();
    _timer_echo_setup(ultrasound_id);
    _timer_new_measurement_setup();

}

bool port_ultrasound_get_trigger_ready(uint32_t ultrasound_id)
{
    /* Get the ultrasound sensor */
    stm32f4_ultrasound_hw_t *p_ultrasound = _stm32f4_ultrasound_get(ultrasound_id);

    if (p_ultrasound != NULL)
    {
        // 1. Return the value of the field trigger_ready.
        return p_ultrasound->trigger_ready;
    }
    else
    {
        return false;
    }
}

bool port_ultrasound_get_trigger_end(uint32_t ultrasound_id)
{
    /* Get the ultrasound sensor */
    stm32f4_ultrasound_hw_t *p_ultrasound = _stm32f4_ultrasound_get(ultrasound_id);

    if (p_ultrasound != NULL)
    {
        // 1. Return the value of the field trigger_end.
        return p_ultrasound->trigger_end;
    }
    else
    {
        return false;
    }
}

void port_ultrasound_stop_trigger_timer(uint32_t ultrasound_id)
{
    /* Get the ultrasound sensor */
    stm32f4_ultrasound_hw_t *p_ultrasound = _stm32f4_ultrasound_get(ultrasound_id);

    if (p_ultrasound != NULL)
    {
        // 1. Set the trigger pin to low. You can use the BSRR or call the `stm32f4_system_gpio_write()` function.
        stm32f4_system_gpio_write(p_ultrasound->p_trigger_port, p_ultrasound->trigger_pin, 0);

        // 2. Disable the trigger timer (register CR1 of the timer).
        TIM3->CR1 &= ~TIM_CR1_CEN;
    }
}

void port_ultrasound_stop_echo_timer(uint32_t ultrasound_id)
{
    // 1. Disable the echo timer (register CR1 of the timer). Enclose the configuration of the echo timer within a conditional statement.
    if (ultrasound_id == PORT_REAR_PARKING_SENSOR_ID)
    {
        TIM2->CR1 &= ~TIM_CR1_CEN;
    }
}

void port_ultrasound_reset_echo_ticks(uint32_t ultrasound_id)
{
    /* Get the ultrasound sensor */
    stm32f4_ultrasound_hw_t *p_ultrasound = _stm32f4_ultrasound_get(ultrasound_id);

    if (p_ultrasound != NULL)
    {
        // 1. Reset the time ticks of the echo signal, and the overflows. Also, reset the flag echo_received.
        p_ultrasound->echo_init_tick = 0;
        p_ultrasound->echo_end_tick = 0;
        p_ultrasound->echo_overflows = 0;
        p_ultrasound->echo_received = false;
    }
}

uint32_t port_ultrasound_get_echo_init_tick	(uint32_t ultrasound_id)
{
    /* Get the ultrasound sensor */
    stm32f4_ultrasound_hw_t *p_ultrasound = _stm32f4_ultrasound_get(ultrasound_id);

    if (p_ultrasound != NULL)
    {
        // 1. Return the value of the field echo_init_tick.
        return p_ultrasound->echo_init_tick;
    }
    else
    {
        return 0;
    }
}

void port_ultrasound_set_echo_init_tick(uint32_t ultrasound_id, uint32_t echo_init_tick)
{
    /* Get the ultrasound sensor */
    stm32f4_ultrasound_hw_t *p_ultrasound = _stm32f4_ultrasound_get(ultrasound_id);

    if (p_ultrasound != NULL)
    {
        // 1. Set the value of the field echo_init_tick with the received value.
        p_ultrasound->echo_init_tick = echo_init_tick;
    }
}

uint32_t port_ultrasound_get_echo_end_tick(uint32_t ultrasound_id)
{
    /* Get the ultrasound sensor */
    stm32f4_ultrasound_hw_t *p_ultrasound = _stm32f4_ultrasound_get(ultrasound_id);

    if (p_ultrasound != NULL)
    {
        // 1. Return the value of the field echo_end_tick.
        return p_ultrasound->echo_end_tick;
    }
    else
    {
        return 0;
    }
}

void port_ultrasound_set_echo_end_tick(uint32_t ultrasound_id, uint32_t echo_end_tick)
{
    /* Get the ultrasound sensor */
    stm32f4_ultrasound_hw_t *p_ultrasound = _stm32f4_ultrasound_get(ultrasound_id);

    if (p_ultrasound != NULL)
    {
        // 1. Set the value of the field echo_end_tick with the received value.
        p_ultrasound->echo_end_tick = echo_end_tick;
    }
}

bool port_ultrasound_get_echo_received(uint32_t ultrasound_id)
{
    /* Get the ultrasound sensor */
    stm32f4_ultrasound_hw_t *p_ultrasound = _stm32f4_ultrasound_get(ultrasound_id);

    if (p_ultrasound != NULL)
    {
        // 1. Return the value of the field echo_received.
        return p_ultrasound->echo_received;
    }
    else
    {
        return false;
    }
}

void port_ultrasound_set_echo_received(uint32_t ultrasound_id, bool echo_received)
{
    /* Get the ultrasound sensor */
    stm32f4_ultrasound_hw_t *p_ultrasound = _stm32f4_ultrasound_get(ultrasound_id);

    if (p_ultrasound != NULL)
    {
        // 1. Set the value of the field echo_received with the received value.
        p_ultrasound->echo_received = echo_received;
    }
}

uint32_t port_ultrasound_get_echo_overflows(uint32_t ultrasound_id)
{
    /* Get the ultrasound sensor */
    stm32f4_ultrasound_hw_t *p_ultrasound = _stm32f4_ultrasound_get(ultrasound_id);

    if (p_ultrasound != NULL)
    {
        // 1. Return the value of the field echo_overflows.
        return p_ultrasound->echo_overflows;
    }
    else
    {
        return 0;
    }
}

void port_ultrasound_set_echo_overflows(uint32_t ultrasound_id, uint32_t echo_overflows)
{
    /* Get the ultrasound sensor */
    stm32f4_ultrasound_hw_t *p_ultrasound = _stm32f4_ultrasound_get(ultrasound_id);

    if (p_ultrasound != NULL)
    {
        // 1. Set the value of the field echo_overflows with the received value.
        p_ultrasound->echo_overflows = echo_overflows;
    }
}

void port_ultrasound_start_measurement(uint32_t ultrasound_id)
{
    /* Get the ultrasound sensor */
    stm32f4_ultrasound_hw_t *p_ultrasound = _stm32f4_ultrasound_get(ultrasound_id);

    if (p_ultrasound != NULL)
    {
        // 1. Reset the flag trigger_ready to indicate that a new measurement has started.
        p_ultrasound->trigger_ready = false;

        // 2. Reset the counters (CNT) of the trigger timer, the echo timer, and the new measurement timer. Enclose the configuration of the trigger and echo timers within a conditional statement.
        if (ultrasound_id == PORT_REAR_PARKING_SENSOR_ID)
        {
            TIM3->CNT = 0;
            TIM2->CNT = 0;
            TIM5->CNT = 0;
        }

        // 3. Set the trigger pin to high. You can use the BSRR or call the `stm32f4_system_gpio_write()` function.
        stm32f4_system_gpio_write(p_ultrasound->p_trigger_port, p_ultrasound->trigger_pin, 1);

        // 4. Enable the timers interrupts in the NVIC using the NVIC_EnableIRQ() function and the TIMx_IRQn interrupts.
        NVIC_EnableIRQ(TIM5_IRQn);
        if (ultrasound_id == PORT_REAR_PARKING_SENSOR_ID)
        {
            NVIC_EnableIRQ(TIM3_IRQn);
            NVIC_EnableIRQ(TIM2_IRQn);
        }

        // 5. Enable the timers of the new measurement, the trigger timer, and the echo timer (register CR1 of the timers). Enclose the configuration of the trigger and echo timer within a conditional statement.
        TIM5->CR1 |= TIM_CR1_CEN;
        if (ultrasound_id == PORT_REAR_PARKING_SENSOR_ID)
        {
            TIM3->CR1 |= TIM_CR1_CEN;
            TIM2->CR1 |= TIM_CR1_CEN;
        }
    }
}

void port_ultrasound_start_new_measurement_timer(void)
{
    // 1. Enable the interrupt of the new measurement timer in the NVIC.
    NVIC_EnableIRQ(TIM5_IRQn);

    // 2. Enable the new measurement timer (register CR1 of the timer).
    TIM5->CR1 |= TIM_CR1_CEN;
}

void port_ultrasound_stop_new_measurement_timer(void)
{
    // 1. Disable the new measurement timer (register CR1 of the timer).
    TIM5->CR1 &= ~TIM_CR1_CEN;
}

void port_ultrasound_stop_ultrasound(uint32_t ultrasound_id)
{
    // 1. Call all the 4 functions to stop the trigger timer, the echo timer, the new measurement timer, and to reset the echo ticks.
    port_ultrasound_stop_trigger_timer(ultrasound_id);
    port_ultrasound_stop_echo_timer(ultrasound_id);
    port_ultrasound_stop_new_measurement_timer();
    port_ultrasound_reset_echo_ticks(ultrasound_id);
}