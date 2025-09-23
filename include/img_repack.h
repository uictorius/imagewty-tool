/**
 * img_repack.h
 *
 * Functions to repack an IMAGEWTY dump folder into a firmware image.
 */

#ifndef IMG_REPACK_H
#define IMG_REPACK_H

/**
 * @def PADDING_ALIGNMENT
 * @brief Alignment boundary used when calculating padded file sizes.
 *
 * All stored file lengths are rounded up to a multiple of this alignment.
 */
#define PADDING_ALIGNMENT 16

/**
 * @brief Repack all files from a dump folder into a single IMAGEWTY image.
 *
 * @param dump_folder Path to the folder containing extracted files and image.cfg.
 * @param output_file Path where the repacked IMAGEWTY image will be written.
 * @return 0 on success, non-zero on error.
 */
int repack_image(const char* dump_folder, const char* output_file);

#endif /* IMG_REPACK_H */
