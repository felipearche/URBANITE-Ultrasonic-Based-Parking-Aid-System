/**
 * @file port_button.h
 * @brief Header for port_button.c file.
 * @author Felipe Fernández-Arche
 * @author David Pedraza
 * @date 25/03/2024
 */

#ifndef PORT_BUTTON_H_
#define PORT_BUTTON_H_

/* Includes ------------------------------------------------------------------*/
/* Standard C includes */
#include <stdint.h>
#include <stdbool.h>

/* HW dependent includes */
#include "port_system.h"

/* Defines and enums ----------------------------------------------------------*/
/* Defines */
#define PORT_PARKING_BUTTON_ID 0 /*!< User button identifier that represents the rear button (activation of the parking aid system) */
#define PORT_PARKING_BUTTON_DEBOUNCE_TIME_MS 200 /*!< Button debounce time in ms */

/* Function prototypes and explanation -------------------------------------------------*/
/**
 * @brief Configure the HW specifications of a given button.
 * 
 * @param button_id Button ID. This index is used to select the element of the buttons_arr[] array
 */
void port_button_init (uint32_t button_id);

/**
 * @brief Return the status of the button (pressed or not).
 * 
 * @param button_id Button ID. This index is used to get the correct button status struct.
 * @return true If the button has been pressed
 * @return false If the button has not been pressed
 */
bool port_button_get_pressed (uint32_t button_id);

/**
 * @brief Get the value of the GPIO connected to the button.
 * 
 * @param button_id Button ID. This index is used to select the element of the buttons_arr[] array
 * @return true 
 * @return false 
 */
bool port_button_get_value (uint32_t button_id);

/**
 * @brief Set the status of the button (pressed or not).
 * 
 * This function is used to force the status of the button. It is used to simulate the button press in the tests.
 * 
 * @param button_id Button ID. This index is used to get the correct button struct of the buttons_arr[] array.
 * @param pressed Status of the button.
 */
void port_button_set_pressed (uint32_t button_id, bool pressed);

/**
 * @brief Get the status of the interrupt line connected to the button.
 * 
 * This function is used to check if the interrupt line of the button is pending. It is called from the ISR to check if the button has been pressed.
 * 
 * @param button_id Button ID. This index is used to get the correct button struct of the buttons_arr[] array.
 * @return true 
 * @return false 
 */
bool port_button_get_pending_interrupt (uint32_t button_id);

/**
 * @brief Clear the pending interrupt of the button.
 * 
 * This function is used to clear the pending interrupt of the button. It is called from the ISR to avoid unwanted interrupts.
 * 
 * @param button_id Button ID. This index is used to get the correct button struct of the buttons_arr[] array.
 */
void port_button_clear_pending_interrupt (uint32_t button_id);

/**
 * @brief Disable the interrupts of the button.
 * 
 * This function is used to disable the interrupts of the button. It is used in the unit tests to avoid unwanted interrupts.
 * 
 * @param button_id Button ID. This index is used to get the correct button struct of the buttons_arr[] array.
 */
void port_button_disable_interrupts (uint32_t button_id);

#endif /*PORT_BUTTON_H_ */

2. La funci´on _timer_check_status_config() es la funci´on privada que configura el temporizador
del dashboard para que se ejecute cada PORT_WHEEL_DASHBOARD_CHECK_TIME_MS milisegundos y emule
el comportamiento del cuentakil´ometros y el consumo de gasolina.
Los apartados no guardan relaci´on entre s´ı. Puede contestarlos en el orden que desee.
(a) Complete:
1. la funci´on _timer_check_status_config() en stm32f4_dashboard.c para que configure los reg-
istros ARR y PSC del temporizador para un periodo de PORT_WHEEL_DASHBOARD_CHECK_TIME_MS
milisegundos.
2. dicha funci´on para que habilite la cuenta del temporizador antes de salir.
(b) Complete la ISR correspondiente al timer TIM4 en interr.c para que se ejecute cuando
el temporizador se desborde. En esta funci´on llame a port_dashboard_set_check_timer_status()
con el valor correspondiente para indicar que ha pasado el tiempo, y gestione el flag de interrupci´on
correctamente.
Page 2
(c) Complete la funci´on do_update_values() para que actualice los valores del cuentakil´ometros
y el dep´osito de gasolina seg´un indica la explicaci´on de la FSM en el enunciado. Haga uso
de las constantes MENU_IDX_... para acceder a los valores del men´u de la FSM del cockpit (menu_values)
y NO use valores a pincho. Estos valores solo se deben actualizar si el dep´osito de gasolina
no est´a vac´ıo.
(d) Modifique la funci´on port_dashboard_set_intensity() para que, si la intensidad es 0, ponga
el registro CCRx del temporizador del PWM a 0 y desactive el canal del PWM. En
tal caso, se debe retornar sin hacer nada m´as. En caso contrario se debe continuar con el
c´odigo existente.
Solution:
1. En la funci´on _timer_check_status_config(), en stm32f4 dashboard.c hay completar:
1 void _ti mer_ chec k_s tatu s_c onfi g ( uint32_t dashboard_id ) {
2 // ...
3 double scc = ( double ) SystemCoreClock ;
4 double psc = round (((( scc / 1000.0) * ms ) / (65535.0 + 1.0) ) - 1.0) ;
5 double arr = round (((( scc / 1000.0) * ms ) / ( psc + 1.0) ) - 1.0) ;
6 while ( arr > 0 xFFFF ) {
7 psc += 1.0;
8 arr = round ((( scc / 1000.0) * ms ) / ( psc + 1.0) - 1.0) ;
9 }
10 TIM4 - > ARR = ( uint32_t ) ( round ( arr ) ) ;
11 TIM4 - > PSC = ( uint32_t ) ( round ( psc ) ) ;
12 // ...
13 TIM4 - > CR1 |= TIM_CR1_CEN ; // Start the timer
14 }
O cualquier pareja de valores de ARR y PSC que den un periodo de 10000 milisegundos. Por
ejemplo:
1 TIM4 - > ARR = 65519;
2 TIM4 - > PSC = 2441;
3 // ...
4 TIM4 - > CR1 |= TIM_CR1_CEN ; // Start the timer
2. En interr.c hay que incluir la ISR del temporizador:
1
2 void TIM4_IRQHandler ( void ) {
3 TIM4 - > SR &= ~ TIM_SR_UIF ;
4 p o r t _ d a s h b o a r d _ s e t _ c h e c k _ t i m e r _ s t a t u s ( PORT_WHEEL_DASHBOARD_ID , true ) ;
5 }
3. En do_update_values(), hay que:
1 if ( p_fsm_cockpit - > menu_values [ MENU_IDX_TANK ] > 0) {
2 p_fsm_cockpit - > menu_values [ MENU_IDX_KM ] += 10;
3 p_fsm_cockpit - > menu_values [ MENU_IDX_TANK ] -= 1;
4 }
4. En stm32f4 dashboard.c en la funci´on port_dashboard_set_intensity():
1 if ( intensity == 0) {
2 DASHBOARD_PWM_TIM - > CCR1 = 0;
3 DASHBOARD_PWM_TIM - > CCER &= ~ TIM_CCER_CC1E ;
4 return ;
5 }
6 else {...}
06:49

Solution: Hay que completar c´odigos en:
• Incluir #include ‘‘port_button.h’’ en interr.c
• Corregir el valor del reloj de temporizador RCC->APB1ENR |= RCC_APB1ENR_TIM2EN
en _timer_pwm_config() en stm32f4_dashboard.c
• Definir valor de #define PORT_WHEEL_DASHBOARD_CHECK_TIME_MS 12000 en port_dashboard.h
• Completar la llamada stm32f4_system_gpio_config(..., STM32F4_GPIO_MODE_AF, ...); en la funci´on
port_dashboard_init() en stm32f4_dashboard.c
