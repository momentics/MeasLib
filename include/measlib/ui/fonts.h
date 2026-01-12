/**
 * @file fonts.h
 * @brief Font Definitions and Types.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 *
 * Defines the common interface for bitmapped fonts used by the renderer.
 */

#ifndef MEASLIB_UI_FONTS_H
#define MEASLIB_UI_FONTS_H

#include <stdint.h>

/**
 * @brief Font Descriptor
 */
typedef struct {
  const uint8_t *bitmap; /**< Pointer to raw bitmap data (column-major or
                            row-major depending on impl) */
  uint8_t width_max;     /**< Maximum character width */
  uint8_t height;        /**< Character height */
  uint8_t start_char;    /**< ASCII code of the first character */
  uint8_t end_char;      /**< ASCII code of the last character */

  /**
   * @brief Glyph Extractor
   *
   * @param font_data Pointer to the font's bitmap array
   * @param c Character to retrieve
   * @param[out] w_out Actual width of the glyph
   * @param[out] h_out Actual height of the glyph
   * @return const uint8_t* Pointer to the specific glyph data
   */
  const uint8_t *(*get_glyph)(const uint8_t *font_data, char c, uint8_t *w_out,
                              uint8_t *h_out);
} meas_font_t;

// --- Available Fonts ---

/**
 * @brief Standard 5x7 System Font
 * Good for dense information and status bars.
 */
extern const meas_font_t font_5x7;

/**
 * @brief Large 11x14 Readable Font
 * Good for main measurement readings.
 */
extern const meas_font_t font_11x14;

#endif // MEASLIB_UI_FONTS_H
