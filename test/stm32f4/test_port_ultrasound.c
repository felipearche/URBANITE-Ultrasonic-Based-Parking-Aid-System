/**
 * @file test_port_ultrasound.c
 * @brief Unit test for the ultrasound port driver.
 *
 * It checks the configuration of the ultrasound transceiver peripheral and the GPIO pins and other configurations using the Unity framework.
 *
 * @author Sistemas Digitales II
 * @date 2025-01-01
 */

/* Includes ------------------------------------------------------------------*/
/* HW independent libraries */
#include <stdlib.h>
#include <unity.h>

/* HW dependent libraries */
#include "port_ultrasound.h"
#include "port_system.h"
#include "stm32f4_system.h"
#include "stm32f4_ultrasound.h"
#include "stm32f4xx.h"

/* Defines and enums ----------------------------------------------------------*/
#define PORT_REAR_PARKING_SENSOR_ID 0 /*!< Ultrasound identifier @hideinitializer */

// Trigger timer configuration
#define REAR_TRIGGER_TIMER TIM3                            /*!< Trigger signal timer @hideinitializer */
#define REAR_TRIGGER_TIMER_IRQ TIM3_IRQn                   /*!< Trigger signal timer IRQ @hideinitializer */
#define REAR_TRIGGER_TIMER_IRQ_PRIO 4                      /*!< Trigger signal timer IRQ priority @hideinitializer */
#define REAR_TRIGGER_TIMER_IRQ_SUBPRIO 0                   /*!< Trigger signal timer IRQ subpriority @hideinitializer */
#define REAR_TRIGGER_TIMER_PER_BUS RCC->APB1ENR            /*!< Trigger signal timer peripheral bus @hideinitializer */
#define REAR_TRIGGER_TIMER_PER_BUS_MASK RCC_APB1ENR_TIM3EN /*!< Trigger signal timer peripheral bus mask @hideinitializer */

// Echo timer configuration
#define REAR_ECHO_TIMER TIM2                             /*!< Echo signal timer @hideinitializer */
#define REAR_ECHO_TIMER_CCMR_CCS_Pos TIM_CCMR1_CC2S_Pos  /*!< Echo signal timer capture/compare channel selection @hideinitializer */
#define REAR_ECHO_TIMER_CCMR_ICF TIM_CCMR1_IC2F          /*!< Echo signal timer input capture filter @hideinitializer */
#define REAR_ECHO_TIMER_CCMR_PSC TIM_CCMR1_IC2PSC        /*!< Echo signal timer input capture prescaler @hideinitializer */
#define REAR_ECHO_TIMER_CCER_CCP_Pos TIM_CCER_CC2P_Pos   /*!< Echo signal timer capture/compare channel positive polarity @hideinitializer */
#define REAR_ECHO_TIMER_CCER_CCNP_Pos TIM_CCER_CC2NP_Pos /*!< Echo signal timer capture/compare channel negative polarity @hideinitializer */
#define REAR_ECHO_TIMER_CCER_CCE TIM_CCER_CC2E           /*!< Echo signal timer capture/compare channel @hideinitializer */
#define REAR_ECHO_TIMER_DIER_CCIE TIM_DIER_CC2IE         /*!< Echo signal timer enable capture/compare channel interrupt @hideinitializer */
#define REAR_ECHO_TIMER_IRQ TIM2_IRQn                    /*!< Echo signal timer IRQ @hideinitializer */
#define REAR_ECHO_TIMER_IRQ_PRIO 3                       /*!< Echo signal timer IRQ priority @hideinitializer */
#define REAR_ECHO_TIMER_IRQ_SUBPRIO 0                    /*!< Echo signal timer IRQ subpriority @hideinitializer */
#define REAR_ECHO_TIMER_PER_BUS RCC->APB1ENR             /*!< Echo signal timer peripheral bus @hideinitializer */
#define REAR_ECHO_TIMER_PER_BUS_MASK RCC_APB1ENR_TIM2EN  /*!< Echo signal timer peripheral bus mask @hideinitializer */

// Measurement timer configuration
#define MEASUREMENT_TIMER TIM5                            /*!< Ultrasound measurement timer @hideinitializer */
#define MEASUREMENT_TIMER_PER_BUS RCC->APB1ENR            /*!< Ultrasound measurement timer peripheral bus @hideinitializer */
#define MEASUREMENT_TIMER_PER_BUS_MASK RCC_APB1ENR_TIM5EN /*!< Ultrasound measurement timer peripheral bus mask @hideinitializer */
#define MEASUREMENT_TIMER_IRQ TIM5_IRQn                   /*!< Ultrasound measurement timer IRQ @hideinitializer */
#define MEASUREMENT_TIMER_IRQ_PRIO 5                      /*!< Ultrasound measurement timer IRQ priority @hideinitializer */
#define MEASUREMENT_TIMER_IRQ_SUBPRIO 0                   /*!< Ultrasound measurement timer IRQ subpriority @hideinitializer */

#define GPIOA_STLINK_MODER_MASK 0xFC000000 /*!< Mask to clear the bits of the GPIOA pins used by the ST-LINK in the MODER register */
#define GPIOA_STLINK_PUPDR_MASK 0xFC000000 /*!< Mask to clear the bits of the GPIOA pins used by the ST-LINK in the PUPDR register */
/* Private variables ---------------------------------------------------------*/
static char msg[200]; /*!< Buffer for the error messages */

/* Private functions ----------------------------------------------------------*/
void setUp(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
}

void tearDown(void)
{
    RCC->AHB1ENR &= ~RCC_AHB1ENR_GPIOAEN;
    RCC->AHB1ENR &= ~RCC_AHB1ENR_GPIOBEN;
}

void test_identifiers(void)
{
    UNITY_TEST_ASSERT_EQUAL_INT(0, PORT_REAR_PARKING_SENSOR_ID, __LINE__, "ERROR: PORT_REAR_PARKING_SENSOR_ID must be 0");
}

void test_pins_trigger(void)
{
    UNITY_TEST_ASSERT_EQUAL_INT(GPIOB, STM32F4_REAR_PARKING_SENSOR_TRIGGER_GPIO, __LINE__, "ERROR: STM32F4_REAR_PARKING_SENSOR_TRIGGER_GPIO GPIO must be GPIOB");
    UNITY_TEST_ASSERT_EQUAL_INT(0, STM32F4_REAR_PARKING_SENSOR_TRIGGER_PIN, __LINE__, "ERROR: STM32F4_REAR_PARKING_SENSOR_TRIGGER_PIN pin must be 0");
}

void test_pins_echo(void)
{
    UNITY_TEST_ASSERT_EQUAL_INT(GPIOA, STM32F4_REAR_PARKING_SENSOR_ECHO_GPIO, __LINE__, "ERROR: STM32F4_REAR_PARKING_SENSOR_ECHO_GPIO GPIO must be GPIOA");
    UNITY_TEST_ASSERT_EQUAL_INT(1, STM32F4_REAR_PARKING_SENSOR_ECHO_PIN, __LINE__, "ERROR: STM32F4_REAR_PARKING_SENSOR_ECHO_PIN pin must be 1");
}

// Test trigger configuration
void _test_regs_trigger(void)
{
    // Retrieve previous configuration
    uint32_t prev_gpio_mode = STM32F4_REAR_PARKING_SENSOR_TRIGGER_GPIO->MODER;
    uint32_t prev_gpio_pupd = STM32F4_REAR_PARKING_SENSOR_TRIGGER_GPIO->PUPDR;

    // Call configuration function
    port_ultrasound_init(PORT_REAR_PARKING_SENSOR_ID);

    // Check that the mode is configured correctly
    uint32_t trigger_mode = ((STM32F4_REAR_PARKING_SENSOR_TRIGGER_GPIO->MODER) >> (STM32F4_REAR_PARKING_SENSOR_TRIGGER_PIN * 2)) & GPIO_MODER_MODER0_Msk;
    UNITY_TEST_ASSERT_EQUAL_UINT32(STM32F4_GPIO_MODE_OUT, trigger_mode, __LINE__, "ERROR: Ultrasound trigger mode is not configured as output");

    // Check that the pull up/down is configured correctly
    uint32_t trigger_pupd = ((STM32F4_REAR_PARKING_SENSOR_TRIGGER_GPIO->PUPDR) >> (STM32F4_REAR_PARKING_SENSOR_TRIGGER_PIN * 2)) & GPIO_PUPDR_PUPD0_Msk;
    UNITY_TEST_ASSERT_EQUAL_UINT32(STM32F4_GPIO_PUPDR_NOPULL, trigger_pupd, __LINE__, "ERROR: Ultrasound trigger pull up/down is not configured as no pull up/down");

    // Check that no other pins other than the needed have been modified:
    uint32_t mask = ~(GPIO_MODER_MODER0_Msk << (STM32F4_REAR_PARKING_SENSOR_TRIGGER_PIN * 2));
    uint32_t prev_gpio_mode_masked = prev_gpio_mode & mask;

    mask = ~(GPIO_PUPDR_PUPD0_Msk << (STM32F4_REAR_PARKING_SENSOR_TRIGGER_PIN * 2));
    uint32_t prev_gpio_pupd_masked = prev_gpio_pupd & mask;

    uint32_t curr_gpio_mode_masked = STM32F4_REAR_PARKING_SENSOR_TRIGGER_GPIO->MODER & mask;
    uint32_t curr_gpio_pupd_masked = STM32F4_REAR_PARKING_SENSOR_TRIGGER_GPIO->PUPDR & mask;

    UNITY_TEST_ASSERT_EQUAL_UINT32(prev_gpio_mode_masked, curr_gpio_mode_masked, __LINE__, "ERROR: GPIO MODE has been modified for other pins than the trigger");
    UNITY_TEST_ASSERT_EQUAL_UINT32(prev_gpio_pupd_masked, curr_gpio_pupd_masked, __LINE__, "ERROR: GPIO PUPD has been modified for other pins than the trigger");
}

void test_regs_trigger(void)
{
    GPIOB->MODER = ~0;
    GPIOB->PUPDR = ~0;
    _test_regs_trigger();
    GPIOB->MODER = 0;
    GPIOB->PUPDR = 0;
    _test_regs_trigger();
}

// Test echo configuration
void _test_regs_echo(void)
{
    // Retrieve previous configuration
    uint32_t prev_gpio_mode = STM32F4_REAR_PARKING_SENSOR_ECHO_GPIO->MODER;
    uint32_t prev_gpio_pupd = STM32F4_REAR_PARKING_SENSOR_ECHO_GPIO->PUPDR;

    // Call configuration function
    port_ultrasound_init(PORT_REAR_PARKING_SENSOR_ID);

    // Check that the mode is configured correctly
    uint32_t echo_mode = ((STM32F4_REAR_PARKING_SENSOR_ECHO_GPIO->MODER) >> (STM32F4_REAR_PARKING_SENSOR_ECHO_PIN * 2)) & GPIO_MODER_MODER0_Msk;
    UNITY_TEST_ASSERT_EQUAL_UINT32(STM32F4_GPIO_MODE_AF, echo_mode, __LINE__, "ERROR: Ultrasound echo mode is not configured as alternate");

    // Check that the pull up/down is configured correctly
    uint32_t echo_pupd = ((STM32F4_REAR_PARKING_SENSOR_ECHO_GPIO->PUPDR) >> (STM32F4_REAR_PARKING_SENSOR_ECHO_PIN * 2)) & GPIO_PUPDR_PUPD0_Msk;
    UNITY_TEST_ASSERT_EQUAL_UINT32(STM32F4_GPIO_PUPDR_NOPULL, echo_pupd, __LINE__, "ERROR: Ultrasound echo pull up/down is not configured as no pull up/down");

    // Check alternate function
    uint32_t echo_af = ((STM32F4_REAR_PARKING_SENSOR_ECHO_GPIO->AFR[STM32F4_REAR_PARKING_SENSOR_ECHO_PIN / 8]) >> ((STM32F4_REAR_PARKING_SENSOR_ECHO_PIN % 8) * 4)) & 0xF;
    UNITY_TEST_ASSERT_EQUAL_UINT32(STM32F4_AF1, echo_af, __LINE__, "ERROR: Ultrasound echo alternate function is not configured as AF1");

    // Check that no other pins other than the needed have been modified:
    uint32_t mask = ~(GPIO_MODER_MODER0_Msk << (STM32F4_REAR_PARKING_SENSOR_ECHO_PIN * 2));
    uint32_t prev_gpio_mode_masked = prev_gpio_mode & mask;

    mask = ~(GPIO_PUPDR_PUPD0_Msk << (STM32F4_REAR_PARKING_SENSOR_ECHO_PIN * 2));
    uint32_t prev_gpio_pupd_masked = prev_gpio_pupd & mask;

    uint32_t curr_gpio_mode_masked = STM32F4_REAR_PARKING_SENSOR_ECHO_GPIO->MODER & mask;
    uint32_t curr_gpio_pupd_masked = STM32F4_REAR_PARKING_SENSOR_ECHO_GPIO->PUPDR & mask;

    UNITY_TEST_ASSERT_EQUAL_UINT32(prev_gpio_mode_masked, curr_gpio_mode_masked, __LINE__, "ERROR: GPIO MODE has been modified for other pins than the echo");
    UNITY_TEST_ASSERT_EQUAL_UINT32(prev_gpio_pupd_masked, curr_gpio_pupd_masked, __LINE__, "ERROR: GPIO PUPD has been modified for other pins than the echo");
}

void test_regs_echo(void)
{
    // Keep default values on GPIOA 13, 14 and 15 which are used by the ST-Link (SWCLK, SWDIO and SWO respectively)
    // Set all GPIOA registers to 1 but GPIOA 13, 14 and 15
    GPIOA->MODER |= ~GPIOA_STLINK_MODER_MASK;
    GPIOA->PUPDR |= ~GPIOA_STLINK_PUPDR_MASK;
    _test_regs_echo();

    // Reset all GPIOA registers to 0 but GPIOA 13, 14 and 15
    GPIOA->MODER &= GPIOA_STLINK_MODER_MASK;
    GPIOA->PUPDR &= GPIOA_STLINK_PUPDR_MASK;
    _test_regs_echo();
}

/**
 * @brief Test the configuration of the timer that controls the duration of the trigger signal of the ultrasound sensor.
 *
 */
void test_trigger_timer_config(void)
{
    // Retrieve previous configuration
    uint32_t prev_tim_trigger_cr1 = REAR_TRIGGER_TIMER->CR1;
    uint32_t prev_tim_trigger_dier = REAR_TRIGGER_TIMER->DIER;
    uint32_t prev_tim_trigger_sr = REAR_TRIGGER_TIMER->SR;

    // Call configuration function
    port_ultrasound_init(PORT_REAR_PARKING_SENSOR_ID);

    // Check that the ULTRASOUND timer for trigger signal is enabled in RCC
    uint32_t tim_trigger_rcc = (REAR_TRIGGER_TIMER_PER_BUS)&REAR_TRIGGER_TIMER_PER_BUS_MASK;
    UNITY_TEST_ASSERT_EQUAL_UINT32(REAR_TRIGGER_TIMER_PER_BUS_MASK, tim_trigger_rcc, __LINE__, "ERROR: ULTRASOUND timer for trigger signal is not enabled in RCC");

    // Check that the ULTRASOUND timer for trigger signal is disabled
    uint32_t tim_trigger_en = (REAR_TRIGGER_TIMER->CR1) & TIM_CR1_CEN_Msk;
    UNITY_TEST_ASSERT_EQUAL_UINT32(0, tim_trigger_en, __LINE__, "ERROR: ULTRASOUND timer for trigger signal must be disabled after configuration");

    // Check that the ULTRASOUND timer for trigger signal is configured with auto-reload preload enabled
    uint32_t tim_trigger_arpe = (REAR_TRIGGER_TIMER->CR1) & TIM_CR1_ARPE_Msk;
    UNITY_TEST_ASSERT_EQUAL_UINT32(TIM_CR1_ARPE_Msk, tim_trigger_arpe, __LINE__, "ERROR: ULTRASOUND timer for trigger signal must be configured with auto-reload preload enabled");

    // Check that the ULTRASOUND timer for trigger signal has cleared the update interrupt
    uint32_t tim_trigger_sr = (REAR_TRIGGER_TIMER->SR) & TIM_SR_UIF_Msk;
    UNITY_TEST_ASSERT_EQUAL_UINT32(0, tim_trigger_sr, __LINE__, "ERROR: ULTRASOUND timer for trigger signal must have cleared the update interrupt");

    // Check that the ULTRASOUND timer for trigger signal has cleared the interrupt
    uint32_t tim_trigger_dier = (REAR_TRIGGER_TIMER->DIER) & TIM_DIER_UIE_Msk;
    UNITY_TEST_ASSERT_EQUAL_UINT32(TIM_DIER_UIE_Msk, tim_trigger_dier, __LINE__, "ERROR: ULTRASOUND timer for trigger signal must have enabled the interrupt");

    // Check that no other bits other than the needed have been modified:
    uint32_t prev_tim_trigger_cr1_masked = prev_tim_trigger_cr1 & ~(TIM_CR1_ARPE_Msk | TIM_CR1_CEN_Msk);
    uint32_t prev_tim_trigger_dier_masked = prev_tim_trigger_dier & ~TIM_DIER_UIE_Msk;
    uint32_t prev_tim_trigger_sr_masked = prev_tim_trigger_sr & ~TIM_SR_UIF_Msk;

    uint32_t curr_tim_trigger_cr1_masked = REAR_TRIGGER_TIMER->CR1 & ~(TIM_CR1_ARPE_Msk | TIM_CR1_CEN_Msk);
    uint32_t curr_tim_trigger_dier_masked = REAR_TRIGGER_TIMER->DIER & ~TIM_DIER_UIE_Msk;
    uint32_t curr_tim_trigger_sr_masked = REAR_TRIGGER_TIMER->SR & ~TIM_SR_UIF_Msk;

    UNITY_TEST_ASSERT_EQUAL_UINT32(prev_tim_trigger_cr1_masked, curr_tim_trigger_cr1_masked, __LINE__, "ERROR: The register CR1 of the ULTRASOUND timer for trigger signal has been modified for other bits than the needed");
    UNITY_TEST_ASSERT_EQUAL_UINT32(prev_tim_trigger_dier_masked, curr_tim_trigger_dier_masked, __LINE__, "ERROR: The register DIER of the ULTRASOUND timer for trigger signal has been modified for other bits than the needed");
    UNITY_TEST_ASSERT_EQUAL_UINT32(prev_tim_trigger_sr_masked, curr_tim_trigger_sr_masked, __LINE__, "ERROR: The register SR of the ULTRASOUND timer for trigger signal has been modified for other bits than the needed");
}

void test_trigger_timer_priority(void)
{
    uint32_t Priority = NVIC_GetPriority(REAR_TRIGGER_TIMER_IRQ);
    uint32_t PriorityGroup = NVIC_GetPriorityGrouping();
    uint32_t pPreemptPriority;
    uint32_t pSubPriority;

    NVIC_DecodePriority(Priority, PriorityGroup, &pPreemptPriority, &pSubPriority);

    TEST_ASSERT_EQUAL(REAR_TRIGGER_TIMER_IRQ_PRIO, pPreemptPriority);
    TEST_ASSERT_EQUAL(REAR_TRIGGER_TIMER_IRQ_SUBPRIO, pSubPriority);
}

/**
 * @brief Test the configuration of the ULTRASOUND trigger signal
 *
 */
void test_trigger_timer_duration()
{
    uint32_t prev_tim_trigger_cr1 = REAR_TRIGGER_TIMER->CR1;

    // Call configuration function to set the trigger signal
    port_ultrasound_init(PORT_REAR_PARKING_SENSOR_ID);

    // Disable ULTRASOUND trigger signal interrupts to avoid any interference
    NVIC_DisableIRQ(REAR_TRIGGER_TIMER_IRQ);

    // Check the computation of the ARR and PSC for the ULTRASOUND trigger signal
    uint32_t us_test = 10;
    uint32_t arr = REAR_TRIGGER_TIMER->ARR;
    uint32_t psc = REAR_TRIGGER_TIMER->PSC;
    uint32_t tim_trigger_dur_us = round((((double)(arr) + 1.0) / ((double)SystemCoreClock / 1000000.0)) * ((double)(psc) + 1));
    sprintf(msg, "ERROR: ULTRASOUND timer for trigger signal ARR and PSC are not configured correctly for a duration of %ld us", us_test);
    UNITY_TEST_ASSERT_INT_WITHIN(1, us_test, tim_trigger_dur_us, __LINE__, msg);

    // Check that the ULTRASOUND timer for trigger signal is enabled
    uint32_t tim_trigger_en = (REAR_TRIGGER_TIMER->CR1) & TIM_CR1_CEN_Msk;
    UNITY_TEST_ASSERT_EQUAL_UINT32(false, tim_trigger_en, __LINE__, "ERROR: ULTRASOUND timer for trigger should not be enabled  after setting the configuration");

    // Check that the trigger_end flag is cleared
    bool trigger_end = port_ultrasound_get_trigger_end(PORT_REAR_PARKING_SENSOR_ID);
    UNITY_TEST_ASSERT_EQUAL_UINT32(false, trigger_end, __LINE__, "ERROR: ULTRASOUND trigger_end flag must be cleared after setting the configuration");

    // Check that the meas_end flag is cleared
    bool trigger_ready = port_ultrasound_get_trigger_ready(PORT_REAR_PARKING_SENSOR_ID);
    UNITY_TEST_ASSERT_EQUAL_UINT32(true, trigger_ready, __LINE__, "ERROR: ULTRASOUND trigger_ready flag must be set after setting the configuration");

    // Check that no other bits other than the needed have been modified:
    uint32_t prev_tim_trigger_cr1_masked = prev_tim_trigger_cr1 & ~TIM_CR1_CEN_Msk;
    uint32_t curr_tim_trigger_cr1_masked = REAR_TRIGGER_TIMER->CR1 & ~TIM_CR1_CEN_Msk;
    UNITY_TEST_ASSERT_EQUAL_UINT32(prev_tim_trigger_cr1_masked, curr_tim_trigger_cr1_masked, __LINE__, "ERROR: The register CR1 of the ULTRASOUND timer for trigger signal has been modified for other bits than the needed");
}

void test_trigger_timer_timeout(void)
{
    // Call configuration function to set the trigger signal
    port_ultrasound_init(PORT_REAR_PARKING_SENSOR_ID);

    // Enable ULTRASOUND trigger signal interrupts to test the timeout
    NVIC_EnableIRQ(REAR_TRIGGER_TIMER_IRQ);

    // Enable the timer
    REAR_TRIGGER_TIMER->CR1 |= TIM_CR1_CEN;

    // Wait for the timeout
    port_system_delay_ms(1); // Wait a time higher than the trigger signal duration

    // Disable ULTRASOUND trigger signal interrupts to avoid any interference
    NVIC_DisableIRQ(REAR_TRIGGER_TIMER_IRQ);

    // Check that the trigger_end flag is set
    bool trigger_end = port_ultrasound_get_trigger_end(PORT_REAR_PARKING_SENSOR_ID);
    UNITY_TEST_ASSERT_EQUAL_UINT32(true, trigger_end, __LINE__, "ERROR: ULTRASOUND trigger_end flag must be set after the timeout");
}

/**
 * @brief Test the configuration of the timer that controls the measurement of the echo signal of the ultrasound sensor.
 *
 */
void test_echo_timer_config(void)
{
    // Retrieve previous configuration
    uint32_t prev_tim_echo_cr1 = REAR_ECHO_TIMER->CR1;
    uint32_t prev_tim_echo_dier = REAR_ECHO_TIMER->DIER;
    uint32_t prev_tim_echo_ccmr = REAR_ECHO_TIMER->CCMR1;
    uint32_t prev_tim_echo_ccer = REAR_ECHO_TIMER->CCER;

    // Call configuration function
    port_ultrasound_init(PORT_REAR_PARKING_SENSOR_ID);

    // Check that the ULTRASOUND timer for echo signal is enabled in RCC
    uint32_t tim_echo_rcc = (REAR_ECHO_TIMER_PER_BUS)&REAR_ECHO_TIMER_PER_BUS_MASK;
    UNITY_TEST_ASSERT_EQUAL_UINT32(REAR_ECHO_TIMER_PER_BUS_MASK, tim_echo_rcc, __LINE__, "ERROR: ULTRASOUND timer for echo signal is not enabled in RCC");

    // Check that the ULTRASOUND timer for echo signal is disabled
    uint32_t tim_echo_en = (REAR_ECHO_TIMER->CR1) & TIM_CR1_CEN_Msk;
    UNITY_TEST_ASSERT_EQUAL_UINT32(0, tim_echo_en, __LINE__, "ERROR: ULTRASOUND timer for echo signal must be disabled after configuration");

    // Check that the ULTRASOUND timer for echo signal channel is configured with the correct capture/compare channel selection
    uint32_t tim_echo_ccmr_ccs = (REAR_ECHO_TIMER->CCMR1) & (0x1 << REAR_ECHO_TIMER_CCMR_CCS_Pos);
    UNITY_TEST_ASSERT_EQUAL_UINT32(0x1 << REAR_ECHO_TIMER_CCMR_CCS_Pos, tim_echo_ccmr_ccs, __LINE__, "ERROR: The channel of the ULTRASOUND timer for echo signal has not been selected correctly");

    // Check that filtering is disabled
    uint32_t tim_echo_ccmr_icf = (REAR_ECHO_TIMER->CCMR1) & REAR_ECHO_TIMER_CCMR_ICF;
    UNITY_TEST_ASSERT_EQUAL_UINT32(0, tim_echo_ccmr_icf, __LINE__, "ERROR: The input capture filter of the ULTRASOUND timer for echo signal must be disabled");

    // Check the edge detection is configured as both edges
    uint32_t tim_echo_ccer_ccp = (REAR_ECHO_TIMER->CCER) & ((0x1 << REAR_ECHO_TIMER_CCER_CCP_Pos) | (0x1 << REAR_ECHO_TIMER_CCER_CCNP_Pos));
    UNITY_TEST_ASSERT_EQUAL_UINT32((0x1 << REAR_ECHO_TIMER_CCER_CCP_Pos) | (0x1 << REAR_ECHO_TIMER_CCER_CCNP_Pos), tim_echo_ccer_ccp, __LINE__, "ERROR: The edge detection of the ULTRASOUND timer for echo signal must be configured as both edges");

    // Check the input capture is enabled
    uint32_t tim_echo_ccer_cce = (REAR_ECHO_TIMER->CCER) & REAR_ECHO_TIMER_CCER_CCE;
    UNITY_TEST_ASSERT_EQUAL_UINT32(REAR_ECHO_TIMER_CCER_CCE, tim_echo_ccer_cce, __LINE__, "ERROR: The input capture of the ULTRASOUND timer for echo signal must be enabled");

    // Check the input prescaler is configured as no prescaler
    uint32_t tim_echo_ccmr_psc = (REAR_ECHO_TIMER->CCMR1) & REAR_ECHO_TIMER_CCMR_PSC;
    UNITY_TEST_ASSERT_EQUAL_UINT32(0, tim_echo_ccmr_psc, __LINE__, "ERROR: The input capture prescaler of the ULTRASOUND timer for echo signal must be configured as no prescaler");

    // Check that the ULTRASOUND timer for echo signal has enabled the update interrupt
    uint32_t tim_echo_dier_uie = (REAR_ECHO_TIMER->DIER) & TIM_DIER_UIE_Msk;
    UNITY_TEST_ASSERT_EQUAL_UINT32(TIM_DIER_UIE_Msk, tim_echo_dier_uie, __LINE__, "ERROR: ULTRASOUND timer for echo signal must have enabled the update interrupts");

    // Check that the interrupts for the input capture channel is enabled
    uint32_t tim_echo_dier_ccie = (REAR_ECHO_TIMER->DIER) & REAR_ECHO_TIMER_DIER_CCIE;
    UNITY_TEST_ASSERT_EQUAL_UINT32(REAR_ECHO_TIMER_DIER_CCIE, tim_echo_dier_ccie, __LINE__, "ERROR: ULTRASOUND timer for echo signal must have enabled the interrupt for the input capture channel");

    // Check that no other bits other than the needed have been modified:
    uint32_t prev_tim_echo_cr1_masked = prev_tim_echo_cr1 & ~TIM_CR1_CEN_Msk;
    uint32_t prev_tim_echo_dier_masked = prev_tim_echo_dier & ~TIM_DIER_UIE_Msk;
    uint32_t prev_tim_echo_ccmr_masked = prev_tim_echo_ccmr & ~((0x1 << REAR_ECHO_TIMER_CCMR_CCS_Pos) | REAR_ECHO_TIMER_CCMR_ICF | REAR_ECHO_TIMER_CCMR_PSC);
    uint32_t prev_tim_echo_ccer_masked = prev_tim_echo_ccer & ~((0x1 << REAR_ECHO_TIMER_CCER_CCP_Pos) | (0x1 << REAR_ECHO_TIMER_CCER_CCNP_Pos) | (0x1 << REAR_ECHO_TIMER_CCER_CCE));

    uint32_t curr_tim_echo_cr1_masked = REAR_ECHO_TIMER->CR1 & ~TIM_CR1_CEN_Msk;
    uint32_t curr_tim_echo_dier_masked = REAR_ECHO_TIMER->DIER & ~TIM_DIER_UIE_Msk;
    uint32_t curr_tim_echo_ccmr_masked = REAR_ECHO_TIMER->CCMR1 & ~((0x1 << REAR_ECHO_TIMER_CCMR_CCS_Pos) | REAR_ECHO_TIMER_CCMR_ICF | REAR_ECHO_TIMER_CCMR_PSC);
    uint32_t curr_tim_echo_ccer_masked = REAR_ECHO_TIMER->CCER & ~((0x1 << REAR_ECHO_TIMER_CCER_CCP_Pos) | (0x1 << REAR_ECHO_TIMER_CCER_CCNP_Pos) | (0x1 << REAR_ECHO_TIMER_CCER_CCE));

    UNITY_TEST_ASSERT_EQUAL_UINT32(prev_tim_echo_cr1_masked, curr_tim_echo_cr1_masked, __LINE__, "ERROR: The register CR1 of the ULTRASOUND timer for echo signal has been modified for other bits than the needed");
    UNITY_TEST_ASSERT_EQUAL_UINT32(prev_tim_echo_dier_masked, curr_tim_echo_dier_masked, __LINE__, "ERROR: The register DIER of the ULTRASOUND timer for echo signal has been modified for other bits than the needed");
    UNITY_TEST_ASSERT_EQUAL_UINT32(prev_tim_echo_ccmr_masked, curr_tim_echo_ccmr_masked, __LINE__, "ERROR: The register CCMR of the ULTRASOUND timer for echo signal has been modified for other bits than the needed");
    UNITY_TEST_ASSERT_EQUAL_UINT32(prev_tim_echo_ccer_masked, curr_tim_echo_ccer_masked, __LINE__, "ERROR: The register CCER of the ULTRASOUND timer for echo signal has been modified for other bits than the needed");
}

void test_echo_timer_priority(void)
{
    uint32_t Priority = NVIC_GetPriority(REAR_ECHO_TIMER_IRQ);
    uint32_t PriorityGroup = NVIC_GetPriorityGrouping();
    uint32_t pPreemptPriority;
    uint32_t pSubPriority;

    NVIC_DecodePriority(Priority, PriorityGroup, &pPreemptPriority, &pSubPriority);

    TEST_ASSERT_EQUAL(REAR_ECHO_TIMER_IRQ_PRIO, pPreemptPriority);
    TEST_ASSERT_EQUAL(REAR_ECHO_TIMER_IRQ_SUBPRIO, pSubPriority);
}

/**
 * @brief Test the configuration of the precision of the time tick of the ULTRASOUND echo signal
 *
 */
void test_echo_timer_precison()
{
    uint32_t prev_tim_echo_cr1 = REAR_ECHO_TIMER->CR1;

    // Call configuration function to set the echo signal
    port_ultrasound_init(PORT_REAR_PARKING_SENSOR_ID);

    // Disable ULTRASOUND echo signal interrupts to avoid any interference
    NVIC_DisableIRQ(REAR_ECHO_TIMER_IRQ);

    // Check the computation of the ARR and PSC for the ULTRASOUND echo signal
    uint32_t us_test = 65536;
    uint16_t arr = REAR_ECHO_TIMER->ARR;
    uint16_t psc = REAR_ECHO_TIMER->PSC;
    uint32_t tim_echo_dur_us = round((((double)(arr) + 1.0) / ((double)SystemCoreClock / 1000000.0)) * ((double)(psc) + 1));
    sprintf(msg, "ERROR: ULTRASOUND timer for echo signal ARR and PSC are not configured correctly for a precision of %ld us", us_test);
    UNITY_TEST_ASSERT_EQUAL_UINT32(us_test, tim_echo_dur_us, __LINE__, msg);

    // Check that the ULTRASOUND timer for echo signal is enabled
    uint32_t tim_echo_en = (REAR_ECHO_TIMER->CR1) & TIM_CR1_CEN_Msk;
    UNITY_TEST_ASSERT_EQUAL_UINT32(false, tim_echo_en, __LINE__, "ERROR: ULTRASOUND timer for echo should not be enabled after setting the configuration");

    // Check that the echo_init_tick time is 0
    uint32_t echo_init_tick = port_ultrasound_get_echo_init_tick(PORT_REAR_PARKING_SENSOR_ID);
    UNITY_TEST_ASSERT_EQUAL_UINT32(0, echo_init_tick, __LINE__, "ERROR: ULTRASOUND echo_init_tick flag must be 0 after setting the configuration");

    // Check that the echo_end_tick time is 0
    uint32_t echo_end_tick = port_ultrasound_get_echo_end_tick(PORT_REAR_PARKING_SENSOR_ID);
    UNITY_TEST_ASSERT_EQUAL_UINT32(0, echo_end_tick, __LINE__, "ERROR: ULTRASOUND echo_end_tick flag must be 0 after setting the configuration");

    // Check that the echo_overflows count is 0
    uint32_t echo_overflows = port_ultrasound_get_echo_overflows(PORT_REAR_PARKING_SENSOR_ID);
    UNITY_TEST_ASSERT_EQUAL_UINT32(0, echo_overflows, __LINE__, "ERROR: ULTRASOUND echo_overflows must be 0 after setting the configuration");

    // Check that the echo_received flag is cleared
    bool echo_received = port_ultrasound_get_echo_received(PORT_REAR_PARKING_SENSOR_ID);
    UNITY_TEST_ASSERT_EQUAL_UINT32(false, echo_received, __LINE__, "ERROR: ULTRASOUND echo_received flag must be cleared after setting the configuration");

    // Check that no other bits than the needed have been modified:
    uint32_t prev_tim_echo_cr1_masked = prev_tim_echo_cr1 & ~TIM_CR1_CEN_Msk;
    uint32_t curr_tim_echo_cr1_masked = REAR_ECHO_TIMER->CR1 & ~TIM_CR1_CEN_Msk;
    UNITY_TEST_ASSERT_EQUAL_UINT32(prev_tim_echo_cr1_masked, curr_tim_echo_cr1_masked, __LINE__, "ERROR: The register CR1 of the ULTRASOUND timer for echo signal has been modified and it should not have been");
}

/**
 * @brief Test the configuration of the timer that controls the measurement time of the ultrasound sensor.
 *
 */
void test_meas_timer_config(void)
{
    // Retrieve previous configuration
    uint32_t prev_tim_meas_cr1 = MEASUREMENT_TIMER->CR1;
    uint32_t prev_tim_meas_dier = MEASUREMENT_TIMER->DIER;
    uint32_t prev_tim_meas_sr = MEASUREMENT_TIMER->SR;

    // Call configuration function
    port_ultrasound_init(PORT_REAR_PARKING_SENSOR_ID);

    // Check that the ULTRASOUND timer for measurement is enabled in RCC
    uint32_t tim_meas_rcc = (MEASUREMENT_TIMER_PER_BUS)&MEASUREMENT_TIMER_PER_BUS_MASK;
    UNITY_TEST_ASSERT_EQUAL_UINT32(MEASUREMENT_TIMER_PER_BUS_MASK, tim_meas_rcc, __LINE__, "ERROR: ULTRASOUND timer for measurement is not enabled in RCC");

    // Check that the ULTRASOUND timer for measurement is disabled
    uint32_t tim_meas_en = (MEASUREMENT_TIMER->CR1) & TIM_CR1_CEN_Msk;
    UNITY_TEST_ASSERT_EQUAL_UINT32(0, tim_meas_en, __LINE__, "ERROR: ULTRASOUND timer for measurement must be disabled after configuration");

    // Check that the ULTRASOUND timer for measurement is configured with auto-reload preload enabled
    uint32_t tim_meas_arpe = (MEASUREMENT_TIMER->CR1) & TIM_CR1_ARPE_Msk;
    UNITY_TEST_ASSERT_EQUAL_UINT32(TIM_CR1_ARPE_Msk, tim_meas_arpe, __LINE__, "ERROR: ULTRASOUND timer for measurement must be configured with auto-reload preload enabled");

    // Check that the ULTRASOUND timer for measurement has cleared the update interrupt
    uint32_t tim_meas_sr = (MEASUREMENT_TIMER->SR) & TIM_SR_UIF_Msk;
    UNITY_TEST_ASSERT_EQUAL_UINT32(0, tim_meas_sr, __LINE__, "ERROR: ULTRASOUND timer for measurement must have cleared the update interrupt");

    // Check that the ULTRASOUND timer for measurement has cleared the interrupt
    uint32_t tim_meas_dier = (MEASUREMENT_TIMER->DIER) & TIM_DIER_UIE_Msk;
    UNITY_TEST_ASSERT_EQUAL_UINT32(TIM_DIER_UIE_Msk, tim_meas_dier, __LINE__, "ERROR: ULTRASOUND timer for measurement must have enabled the interrupt");

    // Check that no other bits other than the needed have been modified:
    uint32_t prev_tim_meas_cr1_masked = prev_tim_meas_cr1 & ~(TIM_CR1_ARPE_Msk | TIM_CR1_CEN_Msk);
    uint32_t prev_tim_meas_dier_masked = prev_tim_meas_dier & ~TIM_DIER_UIE_Msk;
    uint32_t prev_tim_meas_sr_masked = prev_tim_meas_sr & ~TIM_SR_UIF_Msk;

    uint32_t curr_tim_meas_cr1_masked = MEASUREMENT_TIMER->CR1 & ~(TIM_CR1_ARPE_Msk | TIM_CR1_CEN_Msk);
    uint32_t curr_tim_meas_dier_masked = MEASUREMENT_TIMER->DIER & ~TIM_DIER_UIE_Msk;
    uint32_t curr_tim_meas_sr_masked = MEASUREMENT_TIMER->SR & ~TIM_SR_UIF_Msk;

    UNITY_TEST_ASSERT_EQUAL_UINT32(prev_tim_meas_cr1_masked, curr_tim_meas_cr1_masked, __LINE__, "ERROR: The register CR1 of the ULTRASOUND timer for measurement has been modified for other bits than the needed");
    UNITY_TEST_ASSERT_EQUAL_UINT32(prev_tim_meas_dier_masked, curr_tim_meas_dier_masked, __LINE__, "ERROR: The register DIER of the ULTRASOUND timer for measurement has been modified for other bits than the needed");
    UNITY_TEST_ASSERT_EQUAL_UINT32(prev_tim_meas_sr_masked, curr_tim_meas_sr_masked, __LINE__, "ERROR: The register SR of the ULTRASOUND timer for measurement has been modified for other bits than the needed");
}

/**
 * @brief Test the priority of the measurement timer.
 *
 */
void test_meas_timer_priority(void)
{
    uint32_t Priority = NVIC_GetPriority(MEASUREMENT_TIMER_IRQ);
    uint32_t PriorityGroup = NVIC_GetPriorityGrouping();
    uint32_t pPreemptPriority;
    uint32_t pSubPriority;

    NVIC_DecodePriority(Priority, PriorityGroup, &pPreemptPriority, &pSubPriority);

    TEST_ASSERT_EQUAL(MEASUREMENT_TIMER_IRQ_PRIO, pPreemptPriority);
    TEST_ASSERT_EQUAL(MEASUREMENT_TIMER_IRQ_SUBPRIO, pSubPriority);
}

/**
 * @brief Test the configuration of the ULTRASOUND measurement timer
 *
 */
void test_meas_timer_duration()
{
    uint32_t prev_tim_meas_cr1 = MEASUREMENT_TIMER->CR1;

    // Call configuration function to set the measurement
    port_ultrasound_init(PORT_REAR_PARKING_SENSOR_ID);

    // Disable ULTRASOUND measurement interrupts to avoid any interference
    NVIC_DisableIRQ(MEASUREMENT_TIMER_IRQ);

    // Check the computation of the ARR and PSC for the ULTRASOUND measurement
    uint32_t ms_test = 100;
    uint32_t arr = MEASUREMENT_TIMER->ARR;
    uint32_t psc = MEASUREMENT_TIMER->PSC;
    uint32_t tim_meas_dur_ms = round((((double)(arr) + 1.0) / ((double)SystemCoreClock / 1000.0)) * ((double)(psc) + 1));
    sprintf(msg, "ERROR: ULTRASOUND timer for measurement ARR and PSC are not configured correctly for a duration of %ld ms", ms_test);
    UNITY_TEST_ASSERT_INT_WITHIN(1, ms_test, tim_meas_dur_ms, __LINE__, msg);

    // Check that the ULTRASOUND timer for measurement is enabled
    uint32_t tim_meas_en = (MEASUREMENT_TIMER->CR1) & TIM_CR1_CEN_Msk;
    UNITY_TEST_ASSERT_EQUAL_UINT32(false, tim_meas_en, __LINE__, "ERROR: ULTRASOUND timer for measurements should not be enabled after setting the configuration");

    // Check that no other bits other than the needed have been modified:
    uint32_t prev_tim_meas_cr1_masked = prev_tim_meas_cr1 & ~TIM_CR1_CEN_Msk;
    uint32_t curr_tim_meas_cr1_masked = MEASUREMENT_TIMER->CR1 & ~TIM_CR1_CEN_Msk;
    UNITY_TEST_ASSERT_EQUAL_UINT32(prev_tim_meas_cr1_masked, curr_tim_meas_cr1_masked, __LINE__, "ERROR: The register CR1 of the ULTRASOUND timer for measurement has been modified for other bits than the needed");
}

void test_meas_timer_timeout(void)
{
    // Call configuration function to set the measurement
    port_ultrasound_init(PORT_REAR_PARKING_SENSOR_ID);

    // Enable ULTRASOUND measurement interrupts to test the timeout
    NVIC_EnableIRQ(MEASUREMENT_TIMER_IRQ);

    // Enable the timer
    MEASUREMENT_TIMER->CR1 |= TIM_CR1_CEN;

    // Wait for the timeout
    port_system_delay_ms(101); // Wait a time higher than the measurement duration

    // Disable ULTRASOUND measurement interrupts to avoid any interference
    NVIC_DisableIRQ(MEASUREMENT_TIMER_IRQ);

    // Check that the meas_end flag is set
    bool trigger_ready = port_ultrasound_get_trigger_ready(PORT_REAR_PARKING_SENSOR_ID);
    UNITY_TEST_ASSERT_EQUAL_UINT32(true, trigger_ready, __LINE__, "ERROR: ULTRASOUND trigger_ready flag must be set after the measurement timer timeout");
}

void test_start_measurement(void)
{
    // Call configuration function to set the measurement
    port_ultrasound_init(PORT_REAR_PARKING_SENSOR_ID);

    // Call port function to start the measurement
    port_ultrasound_start_measurement(PORT_REAR_PARKING_SENSOR_ID);

    // Retrieve the current configuration of the NVIC interrupts before disabling them
    uint32_t tim_trigger_irq = NVIC->ISER[REAR_TRIGGER_TIMER_IRQ / 32] & (1 << (REAR_TRIGGER_TIMER_IRQ % 32));
    uint32_t tim_echo_irq = NVIC->ISER[REAR_ECHO_TIMER_IRQ / 32] & (1 << (REAR_ECHO_TIMER_IRQ % 32));
    uint32_t tim_meas_irq = NVIC->ISER[MEASUREMENT_TIMER_IRQ / 32] & (1 << (MEASUREMENT_TIMER_IRQ % 32));

    // Disable all interrupts to avoid any interference
    NVIC_DisableIRQ(REAR_TRIGGER_TIMER_IRQ);
    NVIC_DisableIRQ(REAR_ECHO_TIMER_IRQ);
    NVIC_DisableIRQ(MEASUREMENT_TIMER_IRQ);

    // Check that the trigger pin has been set to high
    uint32_t trigger_pin = STM32F4_REAR_PARKING_SENSOR_TRIGGER_GPIO->ODR & (1 << STM32F4_REAR_PARKING_SENSOR_TRIGGER_PIN);
    
    UNITY_TEST_ASSERT_EQUAL_UINT32(1 << STM32F4_REAR_PARKING_SENSOR_TRIGGER_PIN, trigger_pin, __LINE__, "ERROR: The trigger pin must be set to high after starting the measurement");

    // Check that NVIC interrupts have been enabled for all the timers
    UNITY_TEST_ASSERT_EQUAL_UINT32(1 << (REAR_TRIGGER_TIMER_IRQ % 32), tim_trigger_irq, __LINE__, "ERROR: The NVIC interrupt for the ULTRASOUND trigger timer has not been enabled");

    UNITY_TEST_ASSERT_EQUAL_UINT32(1 << (REAR_ECHO_TIMER_IRQ % 32), tim_echo_irq, __LINE__, "ERROR: The NVIC interrupt for the ULTRASOUND echo timer has not been enabled");

    UNITY_TEST_ASSERT_EQUAL_UINT32(1 << (MEASUREMENT_TIMER_IRQ % 32), tim_meas_irq, __LINE__, "ERROR: The NVIC interrupt for the ULTRASOUND measurement timer has not been enabled");

    // Check that all the timers have been enabled
    uint32_t tim_trigger_en = (REAR_TRIGGER_TIMER->CR1) & TIM_CR1_CEN_Msk;
    UNITY_TEST_ASSERT_EQUAL_UINT32(TIM_CR1_CEN_Msk, tim_trigger_en, __LINE__, "ERROR: The ULTRASOUND trigger timer has not been enabled");

    uint32_t tim_echo_en = (REAR_ECHO_TIMER->CR1) & TIM_CR1_CEN_Msk;
    UNITY_TEST_ASSERT_EQUAL_UINT32(TIM_CR1_CEN_Msk, tim_echo_en, __LINE__, "ERROR: The ULTRASOUND echo timer has not been enabled");

    uint32_t tim_meas_en = (MEASUREMENT_TIMER->CR1) & TIM_CR1_CEN_Msk;
    UNITY_TEST_ASSERT_EQUAL_UINT32(TIM_CR1_CEN_Msk, tim_meas_en, __LINE__, "ERROR: The ULTRASOUND measurement timer has not been enabled");
}

/**
 * @brief Test the generalization of the trigger port driver. Particularly, test that the port driver functions work with the ultrasounds_arr array and not with the specific GPIOx peripheral.
 *
 */
void test_trigger_port_generalization(void)
{
    // Change the GPIO and pin in the ultrasounds_arr array to a different one
    GPIO_TypeDef *p_expected_gpio_port = GPIOC;
    uint8_t expected_gpio_pin = 6;
    stm32f4_ultrasound_set_new_trigger_gpio(PORT_REAR_PARKING_SENSOR_ID, p_expected_gpio_port, expected_gpio_pin);

    // TEST init function
    // Enable RCC for the specific GPIO
    if (STM32F4_REAR_PARKING_SENSOR_TRIGGER_GPIO == GPIOA)
    {
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN; /* GPIOA_CLK_ENABLE */
    }
    else if (STM32F4_REAR_PARKING_SENSOR_TRIGGER_GPIO == GPIOB)
    {
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN; /* GPIOB_CLK_ENABLE */
    }
    else if (STM32F4_REAR_PARKING_SENSOR_TRIGGER_GPIO == GPIOC)
    {
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN; /* GPIOC_CLK_ENABLE */
    }

    // Clean/set all configurations
    SYSCFG->EXTICR[STM32F4_REAR_PARKING_SENSOR_TRIGGER_PIN / 4] = 0;
    EXTI->RTSR = 0;
    EXTI->FTSR = 0;
    EXTI->EMR = 0;
    EXTI->IMR = 0;

    STM32F4_REAR_PARKING_SENSOR_TRIGGER_GPIO->MODER |= (STM32F4_GPIO_MODE_IN << (STM32F4_REAR_PARKING_SENSOR_TRIGGER_PIN * 2U));      // Wrong configuration to check that the GPIO is not being modified
    STM32F4_REAR_PARKING_SENSOR_TRIGGER_GPIO->PUPDR |= (STM32F4_GPIO_PUPDR_PULLUP << (STM32F4_REAR_PARKING_SENSOR_TRIGGER_PIN * 2U)); // Wrong configuration to check that the GPIO is not being modified

    // Disable RCC for the specific GPIO
    if (STM32F4_REAR_PARKING_SENSOR_TRIGGER_GPIO == GPIOA)
    {
        RCC->AHB1ENR &= ~RCC_AHB1ENR_GPIOAEN;
    }
    else if (STM32F4_REAR_PARKING_SENSOR_TRIGGER_GPIO == GPIOB)
    {
        RCC->AHB1ENR &= ~RCC_AHB1ENR_GPIOBEN;
    }
    else if (STM32F4_REAR_PARKING_SENSOR_TRIGGER_GPIO == GPIOC)
    {
        RCC->AHB1ENR &= ~RCC_AHB1ENR_GPIOCEN;
    }

    // Expected configuration of the GPIO
    stm32f4_system_gpio_config(p_expected_gpio_port, expected_gpio_pin, STM32F4_GPIO_MODE_OUT, STM32F4_GPIO_PUPDR_NOPULL);
    uint32_t expected_gpio_mode = STM32F4_REAR_PARKING_SENSOR_TRIGGER_GPIO->MODER;
    uint32_t expected_gpio_pupd = STM32F4_REAR_PARKING_SENSOR_TRIGGER_GPIO->PUPDR;

    // Call configuration function
    port_ultrasound_init(PORT_REAR_PARKING_SENSOR_ID);

    // Check that the expected GPIO peripheral has not been modified. Otherwise, the port driver is not generalizing the GPIO peripheral and it is not working with the ultrasounds_arr array but with the specific GPIO nad pin.
    uint32_t curr_gpio_mode = STM32F4_REAR_PARKING_SENSOR_TRIGGER_GPIO->MODER;
    uint32_t curr_gpio_pupd = STM32F4_REAR_PARKING_SENSOR_TRIGGER_GPIO->PUPDR;

    UNITY_TEST_ASSERT_EQUAL_UINT32(expected_gpio_mode, curr_gpio_mode, __LINE__, "ERROR: The configuration function is not generalizing the GPIO and/or pin but working with the specific GPIO and pin for the trigger signal");
    UNITY_TEST_ASSERT_EQUAL_UINT32(expected_gpio_pupd, curr_gpio_pupd, __LINE__, "ERROR: The configuration function is not generalizing the GPIO and/or pin but working with the specific GPIO and pin for the trigger signal");
}


/**
 * @brief Test the generalization of the echo port driver. Particularly, test that the port driver functions work with the ultrasounds_arr array and not with the specific GPIOx peripheral.
 */
void test_echo_port_generalization(void)
{
    // Change the GPIO and pin in the ultrasounds_arr array to a different one
    GPIO_TypeDef *p_expected_gpio_port = GPIOC;
    uint8_t expected_gpio_pin = 6;
    stm32f4_ultrasound_set_new_echo_gpio(PORT_REAR_PARKING_SENSOR_ID, p_expected_gpio_port, expected_gpio_pin);

    // TEST init function
    // Enable RCC for the specific GPIO
    if (STM32F4_REAR_PARKING_SENSOR_ECHO_GPIO == GPIOA)
    {
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN; /* GPIOA_CLK_ENABLE */
    }
    else if (STM32F4_REAR_PARKING_SENSOR_ECHO_GPIO == GPIOB)
    {
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN; /* GPIOB_CLK_ENABLE */
    }
    else if (STM32F4_REAR_PARKING_SENSOR_ECHO_GPIO == GPIOC)
    {
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN; /* GPIOC_CLK_ENABLE */
    }

    // Clean/set all configurations
    SYSCFG->EXTICR[STM32F4_REAR_PARKING_SENSOR_ECHO_PIN / 4] = 0;
    EXTI->RTSR = 0;
    EXTI->FTSR = 0;
    EXTI->EMR = 0;
    EXTI->IMR = 0;

    STM32F4_REAR_PARKING_SENSOR_ECHO_GPIO->MODER |= (STM32F4_GPIO_MODE_OUT << (STM32F4_REAR_PARKING_SENSOR_ECHO_PIN * 2U));     // Wrong configuration to check that the GPIO is not being modified
    STM32F4_REAR_PARKING_SENSOR_ECHO_GPIO->PUPDR |= (STM32F4_GPIO_PUPDR_PULLUP << (STM32F4_REAR_PARKING_SENSOR_ECHO_PIN * 2U)); // Wrong configuration to check that the GPIO is not being modified

    // Disable RCC for the specific GPIO
    if (STM32F4_REAR_PARKING_SENSOR_ECHO_GPIO == GPIOA)
    {
        RCC->AHB1ENR &= ~RCC_AHB1ENR_GPIOAEN;
    }
    else if (STM32F4_REAR_PARKING_SENSOR_ECHO_GPIO == GPIOB)
    {
        RCC->AHB1ENR &= ~RCC_AHB1ENR_GPIOBEN;
    }
    else if (STM32F4_REAR_PARKING_SENSOR_ECHO_GPIO == GPIOC)
    {
        RCC->AHB1ENR &= ~RCC_AHB1ENR_GPIOCEN;
    }

    // Expected configuration of the GPIO
    stm32f4_system_gpio_config(p_expected_gpio_port, expected_gpio_pin, STM32F4_GPIO_MODE_IN, STM32F4_GPIO_PUPDR_NOPULL);
    uint32_t expected_gpio_mode = STM32F4_REAR_PARKING_SENSOR_ECHO_GPIO->MODER;
    uint32_t expected_gpio_pupd = STM32F4_REAR_PARKING_SENSOR_ECHO_GPIO->PUPDR;

    // Call configuration function
    port_ultrasound_init(PORT_REAR_PARKING_SENSOR_ID);

    // Check that the expected GPIO peripheral has not been modified. Otherwise, the port driver is not generalizing the GPIO peripheral and it is not working with the ultrasounds_arr array but with the specific GPIO nad pin.
    uint32_t curr_gpio_mode = STM32F4_REAR_PARKING_SENSOR_ECHO_GPIO->MODER;
    uint32_t curr_gpio_pupd = STM32F4_REAR_PARKING_SENSOR_ECHO_GPIO->PUPDR;

    UNITY_TEST_ASSERT_EQUAL_UINT32(expected_gpio_mode, curr_gpio_mode, __LINE__, "ERROR: The configuration function is not generalizing the GPIO and/or pin but working with the specific GPIO and pin for the echo signal");
    UNITY_TEST_ASSERT_EQUAL_UINT32(expected_gpio_pupd, curr_gpio_pupd, __LINE__, "ERROR: The configuration function is not generalizing the GPIO and/or pin but working with the specific GPIO and pin for the echo signal");
}

int main(void)
{
    port_system_init();
    UNITY_BEGIN();
    // Test ultrasound port driver
    RUN_TEST(test_identifiers);

    // Test ultrasound trigger signal configuration
    RUN_TEST(test_pins_trigger);
    RUN_TEST(test_regs_trigger);
    RUN_TEST(test_trigger_timer_config);
    RUN_TEST(test_trigger_timer_priority);
    RUN_TEST(test_trigger_timer_duration);
    RUN_TEST(test_trigger_timer_timeout);

    // Test ultrasound echo signal configuration
    RUN_TEST(test_pins_echo);
    RUN_TEST(test_regs_echo);
    RUN_TEST(test_echo_timer_config);
    RUN_TEST(test_echo_timer_priority);
    RUN_TEST(test_echo_timer_precison);

    // Test measurement timer configuration
    RUN_TEST(test_meas_timer_config);
    RUN_TEST(test_meas_timer_priority);
    RUN_TEST(test_meas_timer_duration);
    RUN_TEST(test_meas_timer_timeout);

    // Test start measurement
    RUN_TEST(test_start_measurement);

    // Test generalization of the port driver
    RUN_TEST(test_trigger_port_generalization);
    RUN_TEST(test_echo_port_generalization);

    exit(UNITY_END());
}
