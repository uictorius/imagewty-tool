/**
 * img_header.h
 *
 * Definitions for IMAGEWTY file headers and utility functions.
 * Provides structures and functions to read IMAGEWTY main and file headers.
 */

#ifndef IMG_HEADER_H
#define IMG_HEADER_H

#include <stdint.h>
#include <stdio.h>

/** Magic string identifying an ImageWTY file */
#define IMAGEWTY_MAGIC "IMAGEWTY"

/** Standard main header size in bytes */
#define IMG_HEADER_HEADER_SIZE 1024

/** Offset where the file headers section begins */
#define FILE_HEADERS_START 0x400

/**
 * @brief Main IMAGEWTY image header structure.
 *
 * Contains metadata about the firmware image, including number of files.
 * Some fields are unknown/reserved and kept for future use.
 */
typedef struct
{
    char magic[9];                /**< Magic string (null-terminated) */
    uint32_t header_version;      /**< Header version number */
    uint32_t header_size;         /**< Header size in bytes */
    uint32_t base_ram;            /**< Base RAM address */
    uint32_t format_version;      /**< Firmware image format version */
    uint32_t total_image_size;    /**< Total size of the firmware image */
    uint32_t header_size_aligned; /**< Header size including alignment */
    uint32_t file_header_length;  /**< Size of each file header */
    uint32_t usb_product_id;      /**< USB product ID */
    uint32_t usb_vendor_id;       /**< USB vendor ID */
    uint32_t hardware_id;         /**< Hardware ID */
    uint32_t firmware_id;         /**< Firmware ID */
    uint32_t unknown1;            /**< Unknown/reserved */
    uint32_t unknown2;            /**< Unknown/reserved */
    uint32_t num_files;           /**< Number of files in the image */
    uint32_t unknown3;            /**< Unknown/reserved */
} ImageWTYHeader;

/**
 * @brief Single file header inside the IMAGEWTY image.
 *
 * Describes metadata such as file name, type, lengths, and data offset.
 */
typedef struct
{
    uint32_t filename_length; /**< Length of the file name */
    uint32_t header_size;     /**< Size of this file header */
    char maintype[9];         /**< Main type (8 bytes + null) */
    char subtype[17];         /**< Sub type (16 bytes + null) */
    uint32_t unknown0;        /**< Unknown/reserved */
    char filename[257];       /**< File name (256 bytes + null) */
    uint32_t stored_length;   /**< Length of stored/compressed data */
    uint32_t pad1;            /**< Padding/reserved */
    uint32_t original_length; /**< Original uncompressed file size */
    uint32_t pad2;            /**< Padding/reserved */
    uint32_t offset;          /**< Absolute offset of file data in the image */
} ImageWTYFileHeader;

/* Function prototypes */

/**
 * @brief Reads a 32-bit unsigned integer from a file in little-endian order.
 *
 * @param f File pointer
 * @return 32-bit unsigned integer
 */
uint32_t read_uint32_le(FILE* f);

/**
 * @brief Reads the main IMAGEWTY header from a file.
 *
 * @param f   File pointer
 * @param hdr Pointer to an ImageWTYHeader struct to populate
 */
void read_image_header(FILE* f, ImageWTYHeader* hdr);

/**
 * @brief Reads a single file header from the file.
 *
 * @param f                  File pointer
 * @param fh                 Pointer to ImageWTYFileHeader struct to populate
 * @param file_header_length Size of the file header (from main header)
 */
void read_file_header(FILE* f, ImageWTYFileHeader* fh, uint32_t file_header_length);

/**
 * @brief Reads all file headers sequentially from the IMAGEWTY image.
 *
 * Allocates an array of ImageWTYFileHeader; caller must free the memory.
 *
 * @param f               File pointer
 * @param num_files       Number of file headers to read
 * @param file_header_len Size of each file header (from main header)
 * @return Pointer to allocated array of ImageWTYFileHeader
 */
ImageWTYFileHeader* read_all_file_headers(FILE* f, uint32_t num_files, uint32_t file_header_len);

#endif /* IMG_HEADER_H */
