/**
 * @file storage.h
 * @brief File System Facade.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 *
 * Provides a unified abstract interface for file operations, wrapping
 * underlying libraries like FatFS (SD Card) or LittleFS (Flash).
 */

#ifndef MEASLIB_CORE_STORAGE_H
#define MEASLIB_CORE_STORAGE_H

#include "measlib/core/object.h"

/**
 * @brief File Handle
 */
typedef struct {
  meas_object_t base;
} meas_file_t;

/**
 * @brief Filesystem API (VTable)
 */
typedef struct {
  meas_object_api_t base;

  /**
   * @brief Mount the filesystem.
   * @param fs_drv Filesystem driver instance.
   * @return meas_status_t
   */
  meas_status_t (*mount)(meas_object_t *fs_drv);

  /**
   * @brief Unmount the filesystem.
   * @param fs_drv Filesystem driver instance.
   * @return meas_status_t
   */
  meas_status_t (*unmount)(meas_object_t *fs_drv);

  /**
   * @brief Open a file.
   * @param fs_drv Filesystem driver instance.
   * @param path Full path to file.
   * @param out_file Pointer to store the created file handle.
   * @return meas_status_t
   */
  meas_status_t (*open)(meas_object_t *fs_drv, const char *path,
                        meas_file_t **out_file);

  /**
   * @brief Write to an open file.
   * @param file File handle.
   * @param data Data buffer.
   * @param size Bytes to write.
   * @return meas_status_t
   */
  meas_status_t (*write)(meas_file_t *file, const void *data, size_t size);

  /**
   * @brief Read from an open file.
   * @param file File handle.
   * @param buffer Destination buffer.
   * @param size Max bytes to read.
   * @param read_count Actual bytes read.
   * @return meas_status_t
   */
  meas_status_t (*read)(meas_file_t *file, void *buffer, size_t size,
                        size_t *read_count);

  /**
   * @brief Seek to position.
   * @param file File handle.
   * @param offset Offset from start.
   * @return meas_status_t
   */
  meas_status_t (*seek)(meas_file_t *file, size_t offset);

  /**
   * @brief Get current position.
   * @param file File handle.
   * @param offset Output offset.
   * @return meas_status_t
   */
  meas_status_t (*tell)(meas_file_t *file, size_t *offset);

  /**
   * @brief Close the file.
   * @param file File handle.
   * @return meas_status_t
   */
  meas_status_t (*close)(meas_file_t *file);

  /**
   * @brief Create a directory.
   * @param fs_drv Filesystem driver instance.
   * @param path Directory path.
   * @return meas_status_t
   */
  meas_status_t (*mkdir)(meas_object_t *fs_drv, const char *path);

  /**
   * @brief Remove a file or directory.
   * @param fs_drv Filesystem driver instance.
   * @param path Path to remove.
   * @return meas_status_t
   */
  meas_status_t (*remove)(meas_object_t *fs_drv, const char *path);

  /**
   * @brief Get file statistics (Size).
   * @param fs_drv Filesystem driver instance.
   * @param path File path.
   * @param size Output size.
   * @return meas_status_t
   */
  meas_status_t (*stat)(meas_object_t *fs_drv, const char *path, size_t *size);

} meas_fs_api_t;

#endif // MEASLIB_CORE_STORAGE_H
