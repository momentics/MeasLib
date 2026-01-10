/**
 * @file io.c
 * @brief Communication Stream Interface Implementation.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 */

#include "measlib/core/io.h"

meas_status_t meas_io_send(meas_io_t *io, const void *data, size_t size) {
  if (io && io->base.api && ((meas_io_api_t *)io->base.api)->send) {
    return ((meas_io_api_t *)io->base.api)->send(io, data, size);
  }
  return MEAS_ERROR;
}

size_t meas_io_get_available(meas_io_t *io) {
  if (io && io->base.api && ((meas_io_api_t *)io->base.api)->get_available) {
    return ((meas_io_api_t *)io->base.api)->get_available(io);
  }
  return 0;
}

bool meas_io_is_connected(meas_io_t *io) {
  if (io && io->base.api && ((meas_io_api_t *)io->base.api)->is_connected) {
    return ((meas_io_api_t *)io->base.api)->is_connected(io);
  }
  return false;
}

meas_status_t meas_io_receive(meas_io_t *io, void *buffer, size_t size,
                              size_t *read_count) {
  if (io && io->base.api && ((meas_io_api_t *)io->base.api)->receive) {
    return ((meas_io_api_t *)io->base.api)
        ->receive(io, buffer, size, read_count);
  }
  return MEAS_ERROR;
}

meas_status_t meas_io_flush(meas_io_t *io) {
  if (io && io->base.api && ((meas_io_api_t *)io->base.api)->flush) {
    return ((meas_io_api_t *)io->base.api)->flush(io);
  }
  return MEAS_ERROR;
}
