/**
 * print_info.h
 *
 * Functions to display human-readable information about IMAGEWTY headers
 * and firmware files.
 */

#ifndef PRINT_INFO_H
#define PRINT_INFO_H

#include "img_header.h"
#include <stdint.h>

/**
 * @brief Print the main IMAGEWTY image header in a human-readable format.
 *
 * Displays all key fields of the main image header,
 * including magic, header size, USB IDs, number of files, etc.
 *
 * @param h Pointer to the ImageWTYHeader structure
 */
void print_image_header(const ImageWTYHeader* h);

/**
 * @brief Print all file headers contained in the IMAGEWTY image.
 *
 * Iterates through an array of ImageWTYFileHeader structures
 * and prints detailed information for each file.
 *
 * @param files     Pointer to an array of ImageWTYFileHeader structures
 * @param num_files Number of file headers in the array
 */
void print_file_headers(ImageWTYFileHeader* files, uint32_t num_files);

/**
 * @brief Get a human-readable description of a firmware file.
 *
 * Returns a string describing the purpose or content of a firmware file
 * based on its name. If the file is not recognized, the function returns
 * "Unknown or unmapped file name".
 *
 * @param name Full path or base name of the file. Both '/' and '\\' path
 * separators are handled.
 * @return Description string (constant, must not be freed)
 */
const char* describe_file(const char* name);

#endif /* PRINT_INFO_H */
