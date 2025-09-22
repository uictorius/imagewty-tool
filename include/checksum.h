/**
 * checksum.h
 *
 * Declarations for checksum computation and verification/updating of V*.fex
 * files. Provides functions to ensure file integrity in extracted firmware
 * dumps.
 */

#ifndef CHECKSUM_H
#define CHECKSUM_H

#include <stdint.h>

/**
 * @brief Compute a simple 32-bit checksum for a given file.
 *
 * The checksum sums all 4-byte words in little-endian order,
 * padding any remaining bytes with zeros.
 *
 * @param filename Path to the file.
 * @return 32-bit checksum, or 0 if the file cannot be opened.
 */
uint32_t compute_checksum(const char* filename);

/**
 * @brief Verify and update "V*.fex" checksums in the specified folder.
 *
 * If a mismatch is found, the checksum in the V*.fex file is updated.
 *
 * @param dump_folder Path to folder containing extracted files.
 */
void update_vfiles_if_needed(const char* dump_folder);

/**
 * @brief Verify "V*.fex" checksums without updating them.
 *
 * Useful for integrity check after extraction.
 *
 * @param dump_folder Path to folder containing extracted files.
 */
void verify_vfiles_checksums(const char* dump_folder);

#endif /* CHECKSUM_H */
