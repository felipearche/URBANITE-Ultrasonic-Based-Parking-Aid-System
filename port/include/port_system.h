/**
 * @file port_system.h
 * @brief Header for port_system.c file.
 * @author SDG2. Román Cárdenas (r.cardenas@upm.es) and Josué Pagán (j.pagan@upm.es)
 * @date 2025-01-01
 */

#ifndef PORT_SYSTEM_H_
#define PORT_SYSTEM_H_

/* Includes del sistema */
#include <stdint.h>


/**
 * @brief Initializes the system.
 */
uint32_t port_system_init(void);

/**
 * @brief Returns the number of milliseconds since the system started.
 *
 * @retval number of milliseconds since the system started.
 */
uint32_t port_system_get_millis(void);

/**
 * @brief Sets the number of milliseconds since the system started.
 *
 * @param ms New number of milliseconds since the system started.
 */
void port_system_set_millis(uint32_t ms);

/**
 * @brief Delays the program execution for the specified number of milliseconds.
 *
 * @param ms Number of milliseconds to delay.
 */
void port_system_delay_ms(uint32_t ms);

/**
 * @brief Delays the program execution until the specified number of milliseconds since the system started.
 *
 * @param t Pointer to the variable that stores the number of milliseconds to delay until.
 * @param ms Number of milliseconds to delay until.
 *
 * @note This function modifies the value of the variable pointed by t to the number of milliseconds to delay until.
 * @note This function is useful to implement periodic tasks.
 */
void port_system_delay_until_ms(uint32_t *t, uint32_t ms);

/**
 * @brief Set the system in stop mode for low power consumption.
 * 
 * The function sets the state of the power regulator in stop mode.
 * After that, the function sets the system mode in sleep mode, to enter in stop mode when calling __WFI() (wait for interruption).
 * The system remains in this line until an external interruption or timer interruption occurs.
 * If an interruption occurs the system wakes up and resets the system mode.
 * 
 */
void port_system_power_stop();

/**
 * @brief Set the system in sleep mode for low power consumption.
 * 
 * The function sets the state of the power regulator in sleep mode.
 * After that, the function sets the system mode in sleep mode, to enter in sleep mode when calling __WFI() (wait for interruption).
 * The system remains in this line until an external interruption or timer interruption occurs.
 * 
 */
void port_system_power_sleep();

/**
 * @brief Resume Tick increment.
 * 
 * The SysTick timer is the source of time base. It is used to generate interrupts at regular time intervals. Once this function is called, the SysTick interrupt will be enabled and so Tick increment is resumed.
 * 
 */
void port_system_systick_resume();

/**
 * @brief Suspend Tick increment.
 * 
 * The SysTick timer is the source of time base. It is used to generate interrupts at regular time intervals. Once this function is called, the SysTick interrupt will be disabled so it saves more energy and it does not generate any interruption.
 * 
 */
void port_system_systick_suspend();

/**
 * @brief Enable low power consumption in sleep mode.
 * 
 */
void port_system_sleep ();

#endif /* PORT_SYSTEM_H_ */
