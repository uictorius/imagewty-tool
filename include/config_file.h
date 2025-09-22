/**
 * config_file.h
 *
 * Functions to load, write, and print IMAGEWTY configuration files.
 *
 * Provides utilities to parse the `image.cfg` file from a dump folder,
 * manage memory for file headers, and output configuration in a
 * human-readable format.
 */

#ifndef CONFIG_FILE_H
#define CONFIG_FILE_H

#include "img_header.h"
#include <stdint.h>

/**
 * @brief Loads the IMAGEWTY configuration from an image.cfg file.
 *
 * Parses the configuration file, fills the provided ImageWTYHeader structure,
 * and allocates an array of ImageWTYFileHeader structures for all files listed.
 *
 * @param cfg_path Path to the `image.cfg` file.
 * @param hdr Pointer to an ImageWTYHeader structure to populate.
 * @param files Pointer to a pointer where the allocated array of
 *              ImageWTYFileHeader will be stored.
 *              The caller is responsible for freeing it using `free_file_list`.
 * @return 0 on success, non-zero on failure (file not found, memory allocation error, etc.)
 */
int load_image_config(const char* cfg_path, ImageWTYHeader* hdr, ImageWTYFileHeader** files);

/**
 * @brief Writes the IMAGEWTY configuration to an image.cfg file.
 *
 * Outputs the header and all file information to the specified file in the
 * standard IMAGEWTY configuration format.
 *
 * @param cfg_path Path to the `image.cfg` file to write.
 * @param hdr Pointer to the ImageWTYHeader structure to write.
 * @param files Array of ImageWTYFileHeader structures representing each file.
 * @param num_files Number of files in the array.
 * @return 0 on success, non-zero on failure (file write error, etc.)
 */
int write_image_config(const char* cfg_path, const ImageWTYHeader* hdr,
                       const ImageWTYFileHeader* files, uint32_t num_files);

/**
 * @brief Frees memory allocated for the file headers array.
 *
 * @param files Pointer to the array of ImageWTYFileHeader structures
 *              previously allocated by `load_image_config`.
 */
void free_file_list(ImageWTYFileHeader* files);

#endif /* CONFIG_FILE_H */
