/**
 * img_repack.h
 *
 * Functions to repack an IMAGEWTY dump folder into a firmware image.
 */

#ifndef IMG_REPACK_H
#define IMG_REPACK_H

/**
 * @brief Repack all files from a dump folder into a single IMAGEWTY image.
 *
 * @param dump_folder Path to the folder containing extracted files and image.cfg.
 * @param output_file Path where the repacked IMAGEWTY image will be written.
 * @return 0 on success, non-zero on error.
 */
int repack_image(const char* dump_folder, const char* output_file);

#endif /* IMG_REPACK_H */
