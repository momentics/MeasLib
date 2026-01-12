/**
 * @file drv_usb_vcp.c
 * @brief STM32F303 Bare Metal USB CDC (VCP) Driver.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 *
 * Implements the Link Interface (`meas_hal_link_api_t`) for USB Communication.
 *
 * @note This file contains the Device, Configuration, and String Descriptors
 * ported from the NanoVNA-X reference.
 *
 * @warning A full Bare Metal USB Device Stack is required to drive the
 * USB peripheral (handle interrupts, enumeration, SETUP packets).
 * This driver currently provides the API structure and Descriptors but
 * relies on an external or future-implemented stack to actually move data.
 * For now, the API returns unconnected status.
 */

#include "measlib/drivers/hal.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// --- USB Descriptors (Ported from NanoVNA-X) ---

#define USB_DESC_DEVICE(bcdUSB, bDeviceClass, bDeviceSubClass,                 \
                        bDeviceProtocol, wMaxPacketSize, idVendor, idProduct,  \
                        bcdDevice, iManufacturer, iProduct, iSerialNumber,     \
                        bNumConfigurations)                                    \
  (uint8_t)(bcdUSB & 0xFF), (uint8_t)((bcdUSB >> 8) & 0xFF), bDeviceClass,     \
      bDeviceSubClass, bDeviceProtocol, wMaxPacketSize,                        \
      (uint8_t)(idVendor & 0xFF), (uint8_t)((idVendor >> 8) & 0xFF),           \
      (uint8_t)(idProduct & 0xFF), (uint8_t)((idProduct >> 8) & 0xFF),         \
      (uint8_t)(bcdDevice & 0xFF), (uint8_t)((bcdDevice >> 8) & 0xFF),         \
      iManufacturer, iProduct, iSerialNumber, bNumConfigurations

#define USB_DESC_CONFIGURATION(wTotalLength, bNumInterfaces,                   \
                               bConfigurationValue, iConfiguration,            \
                               bmAttributes, bMaxPower)                        \
  0x09, 0x02, (uint8_t)(wTotalLength & 0xFF),                                  \
      (uint8_t)((wTotalLength >> 8) & 0xFF), bNumInterfaces,                   \
      bConfigurationValue, iConfiguration, bmAttributes, bMaxPower

#define USB_DESC_INTERFACE(bInterfaceNumber, bAlternateSetting, bNumEndpoints, \
                           bInterfaceClass, bInterfaceSubClass,                \
                           bInterfaceProtocol, iInterface)                     \
  0x09, 0x04, bInterfaceNumber, bAlternateSetting, bNumEndpoints,              \
      bInterfaceClass, bInterfaceSubClass, bInterfaceProtocol, iInterface

#define USB_DESC_ENDPOINT(bEndpointAddress, bmAttributes, wMaxPacketSize,      \
                          bInterval)                                           \
  0x07, 0x05, bEndpointAddress, bmAttributes,                                  \
      (uint8_t)(wMaxPacketSize & 0xFF),                                        \
      (uint8_t)((wMaxPacketSize >> 8) & 0xFF), bInterval

#define USB_DESC_BYTE(b) b
#define USB_DESC_WORD(w) (uint8_t)(w & 0xFF), (uint8_t)((w >> 8) & 0xFF)
#define USB_DESCRIPTOR_STRING 0x03

enum { STR_LANG_ID = 0, STR_MANUFACTURER, STR_PRODUCT, STR_SERIAL };

/* Device Descriptor */
static const uint8_t vcom_device_descriptor_data[] = {
    0x12,
    0x01, // bLength, bDescriptorType
    0x10,
    0x01, // bcdUSB (1.1)
    0x02, // bDeviceClass (CDC)
    0x00, // bDeviceSubClass
    0x00, // bDeviceProtocol
    0x40, // bMaxPacketSize (64)
    0x83,
    0x04, // idVendor (ST 0x0483)
    0x40,
    0x57, // idProduct (0x5740)
    0x00,
    0x02,             // bcdDevice (2.00)
    STR_MANUFACTURER, // iManufacturer
    STR_PRODUCT,      // iProduct
    STR_SERIAL,       // iSerialNumber
    0x01              // bNumConfigurations
};

/* String Descriptors */
static const uint8_t vcom_string0[] = {
    4, 0x03, 0x09, 0x04 // LangID: US English (0x0409)
};

// "NanoVNA"
static const uint8_t vcom_string1[] = {16,  0x03, 'N', 0, 'a', 0, 'n', 0,
                                       'o', 0,    'V', 0, 'N', 0, 'A', 0};

// "NanoVNA-H4"
static const uint8_t vcom_string2[] = {22,  0x03, 'N', 0, 'a', 0, 'n', 0,
                                       'o', 0,    'V', 0, 'N', 0, 'A', 0,
                                       '-', 0,    'H', 0, '4', 0};

// Serial "001"
static const uint8_t vcom_string3[] = {8, 0x03, '0', 0, '0', 0, '1', 0};

// ... Full configuration handling would go here ...

// --- API Implementation ---

static bool connected = false;

static meas_status_t drv_usb_send(void *ctx, const void *data, size_t len) {
  (void)ctx;
  (void)data;
  (void)len;
  // TODO: Add USB Packet to IN Endpoint FIFO
  return MEAS_OK;
}

static meas_status_t drv_usb_recv(void *ctx, void *data, size_t len,
                                  size_t *read) {
  (void)ctx;
  (void)data;
  (void)len;
  if (read)
    *read = 0;
  // TODO: Read from OUT Endpoint FIFO ring buffer
  return MEAS_OK;
}

static bool drv_usb_is_connected(void *ctx) {
  (void)ctx;
  // TODO: Return true if DTR is set by Host
  return connected;
}

static const meas_hal_link_api_t usb_api = {.send = drv_usb_send,
                                            .recv = drv_usb_recv,
                                            .is_connected =
                                                drv_usb_is_connected};

meas_hal_link_api_t *meas_drv_usb_init(void) {
  // 1. Enable USB Clock (RCC_APB1ENR_USBEN)
  // 2. Configure USB Prescaler
  // 3. Initialize USB Peripheral (CNTR, BTABLE, etc.)
  // 4. Enable USB Interrupts (NVIC)

  // Placeholder:
  connected = false;

  return (meas_hal_link_api_t *)&usb_api;
}
