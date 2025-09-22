/**
 * img_extract.h
 *
 * Functions to extract all files from an IMAGEWTY image into a dump folder.
 * After extraction, verifies integrity of extracted files using V*.fex checksums.
 */

#ifndef IMG_EXTRACT_H
#define IMG_EXTRACT_H

#include "img_header.h"

/**
 * @brief Extracts all files from an IMAGEWTY image into a dump folder.
 *
 * This function reads the image headers, extracts each file to the appropriate
 * location, and verifies the integrity of V*.fex files using their checksums.
 *
 * @param img_filename Path to the IMAGEWTY image file.
 * @return 0 on success, non-zero on error.
 */
int extract_image(const char* img_filename);

#endif /* IMG_EXTRACT_H */
