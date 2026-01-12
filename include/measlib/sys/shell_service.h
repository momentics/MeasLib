/**
 * @file shell_service.h
 * @brief Shell Service API (USB CDC).
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 */

#ifndef MEAS_SYS_SHELL_SERVICE_H
#define MEAS_SYS_SHELL_SERVICE_H

/**
 * @brief Initialize the Shell Service.
 * @param link Pointer to the communication link API (e.g. USB CDC).
 * @param ctx Pointer to the link context.
 */
void meas_shell_service_init(const void *link_api, void *ctx);

/**
 * @brief Poll the Shell Service.
 * Reads input and processes commands (or echoes).
 */
void meas_shell_service_poll(void);

#endif // MEAS_SYS_SHELL_SERVICE_H
