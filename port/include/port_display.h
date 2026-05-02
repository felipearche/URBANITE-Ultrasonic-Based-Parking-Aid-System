/**
 * @file port_display.h
 * @brief Header for the portable functions to interact with the HW of the display system. The functions must be implemented in the platform-specific code.
 * @author Felipe Fernández-Arche
 * @author alumno2
 * @date 09/04/2025
 */
#ifndef PORT_DISPLAY_SYSTEM_H_
#define PORT_DISPLAY_SYSTEM_H_

/* Includes ------------------------------------------------------------------*/
/* Standard C includes */
#include <stdint.h>

/* Typedefs --------------------------------------------------------------------*/
/**
 * @brief Structure to define an RGB color.
 * 
 */
typedef struct
{
    uint8_t r; /*!< Red color value */
    uint8_t g; /*!< Green color value */
    uint8_t b; /*!< Blue color value */
} rgb_color_t;

/* Defines and enums ----------------------------------------------------------*/
/* Defines */
#define PORT_REAR_PARKING_DISPLAY_ID 0 /*!< Display system identifier for the rear parking sensor */
#define PORT_DISPLAY_RGB_MAX_VALUE 255 /*!< Maximum value for the RGB LED */
#define COLOR_RED /*!< Red color */
#define COLOR_GREEN /*!< Green color */
#define COLOR_BLUE /*!< Blue color */
#define COLOR_YELLOW /*!< Yellow color */
#define COLOR_TURQUOISE /*!< Turquoise color */
#define COLOR_OFF (rgb_color_t){0, 0, 0} /*!< Off color */

/* Function prototypes and explanation -------------------------------------------------*/
/**
 * @brief Configure the HW specifications of a given display.
 * 
 * @param display_id Display ID. This index is used to select the element of the displays_arr[] array.
 */
void port_display_init ( uint32_t display_id );

/**
 * @brief Set the Capture/Compare register values for each channel of the RGB LED given a color.
 * 
 * This function disables the timer associated to the RGB LEDs, sets the Capture/Compare register values for each channel of the RGB LED, and enables the timer.
 * 
 * @param display_id Display system identifier number.
 * @param color RGB color to set.
 */
void port_display_set_rgb ( uint32_t display_id, rgb_color_t color );

#endif /* PORT_DISPLAY_SYSTEM_H_ */