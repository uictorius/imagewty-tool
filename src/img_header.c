/**
 * @file img_header.c
 * @brief Implements reading of IMAGEWTY image headers and file headers.
 */

#include "img_header.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Read a 32-bit unsigned integer from a file in little-endian format.
 *
 * Exits the program if reading fails.
 *
 * @param f File pointer.
 * @return uint32_t value read.
 */
uint32_t read_uint32_le(FILE* f)
{
    uint8_t buf[4];
    if (fread(buf, 1, 4, f) != 4)
    {
        perror("Failed to read uint32");
        exit(EXIT_FAILURE);
    }
    return ((uint32_t)buf[0]) | ((uint32_t)buf[1] << 8) | ((uint32_t)buf[2] << 16) |
           ((uint32_t)buf[3] << 24);
}

/**
 * @brief Read the global IMAGEWTY header from the image file.
 *
 * @param f File pointer to IMAGEWTY file.
 * @param hdr Pointer to ImageWTYHeader structure to populate.
 */
void read_image_header(FILE* f, ImageWTYHeader* hdr)
{
    if (fread(hdr->magic, 1, 8, f) != 8)
    {
        perror("Failed to read IMAGEWTY magic");
        exit(EXIT_FAILURE);
    }
    hdr->magic[8] = '\0'; /* Null-terminate magic string */

    hdr->header_version = read_uint32_le(f);
    hdr->header_size = read_uint32_le(f);
    hdr->base_ram = read_uint32_le(f);
    hdr->format_version = read_uint32_le(f);
    hdr->total_image_size = read_uint32_le(f);
    hdr->header_size_aligned = read_uint32_le(f);
    hdr->file_header_length = read_uint32_le(f);
    hdr->usb_product_id = read_uint32_le(f);
    hdr->usb_vendor_id = read_uint32_le(f);
    hdr->hardware_id = read_uint32_le(f);
    hdr->firmware_id = read_uint32_le(f);
    hdr->unknown1 = read_uint32_le(f);
    hdr->unknown2 = read_uint32_le(f);
    hdr->num_files = read_uint32_le(f);
    hdr->unknown3 = read_uint32_le(f);
}

/**
 * @brief Read a single file header from the image file.
 *
 * @param f File pointer to IMAGEWTY file.
 * @param fh Pointer to ImageWTYFileHeader structure to populate.
 * @param file_header_length Length of each file header in bytes.
 */
void read_file_header(FILE* f, ImageWTYFileHeader* fh, uint32_t file_header_length)
{
    long start_offset = ftell(f);

    /* Basic header fields */
    fh->filename_length = read_uint32_le(f);
    fh->header_size = read_uint32_le(f);

    /* Read maintype (8 bytes) */
    if (fread(fh->maintype, 1, 8, f) != 8)
    {
        perror("Failed to read maintype");
        exit(EXIT_FAILURE);
    }
    fh->maintype[8] = '\0';

    /* Read subtype (16 bytes) */
    if (fread(fh->subtype, 1, 16, f) != 16)
    {
        perror("Failed to read subtype");
        exit(EXIT_FAILURE);
    }
    fh->subtype[16] = '\0';

    fh->unknown0 = read_uint32_le(f);

    /* Read filename (up to 256 bytes) */
    if (fh->filename_length > 256)
        fh->filename_length = 256;
    if (fread(fh->filename, 1, fh->filename_length, f) != fh->filename_length)
    {
        perror("Failed to read filename");
        exit(EXIT_FAILURE);
    }
    fh->filename[fh->filename_length] = '\0';

    /* Skip remaining filename padding */
    fseek(f, 256 - fh->filename_length, SEEK_CUR);

    /* File size fields */
    fh->stored_length = read_uint32_le(f);
    fh->pad1 = read_uint32_le(f);
    fh->original_length = read_uint32_le(f);
    fh->pad2 = read_uint32_le(f);
    fh->offset = read_uint32_le(f); /* Absolute offset of file data */

    /* Skip any remaining padding in the file header */
    long consumed = ftell(f) - start_offset;
    long skip = (long)file_header_length - consumed;
    if (skip > 0)
        fseek(f, skip, SEEK_CUR);
}

/**
 * @brief Read all file headers from the image.
 *
 * @param f File pointer to IMAGEWTY file.
 * @param num_files Number of files in the image.
 * @param file_header_length Length of each file header in bytes.
 * @return Pointer to allocated array of ImageWTYFileHeader structures.
 */
ImageWTYFileHeader* read_all_file_headers(FILE* f, uint32_t num_files, uint32_t file_header_length)
{
    ImageWTYFileHeader* files = malloc(sizeof(ImageWTYFileHeader) * num_files);
    if (!files)
    {
        perror("Failed to allocate memory for file headers");
        exit(EXIT_FAILURE);
    }

    for (uint32_t i = 0; i < num_files; i++)
    {
        if (fseek(f, FILE_HEADERS_START + i * file_header_length, SEEK_SET) != 0)
        {
            perror("Failed to seek to file header");
            free(files);
            exit(EXIT_FAILURE);
        }
        read_file_header(f, &files[i], file_header_length);
    }

    return files;
}
