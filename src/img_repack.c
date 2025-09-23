/**
 * @file img_repack.c
 * @brief Repacking utilities for IMAGEWTY firmware images.
 *
 * This module implements functions to repack extracted IMAGEWTY directories
 * into a single IMAGEWTY file. The resulting image consists of:
 *  - Global header
 *  - File headers
 *  - File data with proper padding
 *
 * @note All operations assume the standard IMAGEWTY file format and header sizes.
 */

#include "img_repack.h"

#include <dirent.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "checksum.h"
#include "config_file.h"
#include "img_header.h"

/**
 * @brief Calculate aligned stored length and padding for a file.
 *
 * Pads the file to the next multiple of PADDING_ALIGNMENT (default 16 bytes).
 *
 * @param original_length Original file size in bytes.
 * @param stored_length Output parameter: padded file size.
 * @param padding Output parameter: number of padding bytes to add.
 */
void calculate_padding(uint64_t original_length, uint64_t* stored_length, uint64_t* padding)
{
    *stored_length =
        ((original_length + PADDING_ALIGNMENT - 1) / PADDING_ALIGNMENT) * PADDING_ALIGNMENT;
    *padding = *stored_length - original_length;
}

/**
 * @brief Repack a directory dump into a new IMAGEWTY file.
 *
 * The function follows the IMAGEWTY format standard:
 *  - Updates virtual files if needed.
 *  - Loads global header and file headers from image.cfg.
 *  - Writes global header.
 *  - Writes file headers (1024 bytes each by default).
 *  - Copies actual file data with proper padding.
 *
 * @param dump_folder Path to the extracted dump directory.
 * @param output_file Path to the resulting IMAGEWTY file.
 * @return 0 on success, non-zero on error.
 */
int repack_image(const char* dump_folder, const char* output_file)
{
    if (!dump_folder || !output_file)
    {
        fprintf(stderr, "repack_image: invalid parameters\n");
        return 1;
    }

    // Update virtual files if necessary
    update_vfiles_if_needed(dump_folder);

    // Load global header and file headers from image.cfg
    char cfg_path[1024];
    snprintf(cfg_path, sizeof(cfg_path), "%s/image.cfg", dump_folder);

    ImageWTYHeader hdr;
    ImageWTYFileHeader* files = NULL;

    if (load_image_config(cfg_path, &hdr, &files) != 0 || !files)
    {
        fprintf(stderr, "Failed to load image.cfg from '%s'\n", cfg_path);
        return 1;
    }

    // Open output file
    FILE* out = fopen(output_file, "wb");
    if (!out)
    {
        fprintf(stderr, "Cannot create output file '%s': %s\n", output_file, strerror(errno));
        free(files);
        return 1;
    }

    // ------------------------------------------------------------------
    // Write Global Header
    // ------------------------------------------------------------------
    uint8_t gh_buf[IMG_HEADER_HEADER_SIZE];
    memset(gh_buf, 0, sizeof(gh_buf));

    memcpy(gh_buf + 0x00, hdr.magic, 8);
    memcpy(gh_buf + 0x08, &hdr.header_version, 4);
    memcpy(gh_buf + 0x0C, &hdr.header_size, 4);
    memcpy(gh_buf + 0x10, &hdr.base_ram, 4);
    memcpy(gh_buf + 0x14, &hdr.format_version, 4);
    memcpy(gh_buf + 0x18, &hdr.total_image_size, 4);
    memcpy(gh_buf + 0x1C, &hdr.header_size_aligned, 4);
    memcpy(gh_buf + 0x20, &hdr.file_header_length, 4);
    memcpy(gh_buf + 0x24, &hdr.usb_product_id, 4);
    memcpy(gh_buf + 0x28, &hdr.usb_vendor_id, 4);
    memcpy(gh_buf + 0x2C, &hdr.hardware_id, 4);
    memcpy(gh_buf + 0x30, &hdr.firmware_id, 4);
    memcpy(gh_buf + 0x34, &hdr.unknown1, 4);
    memcpy(gh_buf + 0x38, &hdr.unknown2, 4);
    memcpy(gh_buf + 0x3C, &hdr.num_files, 4);
    memcpy(gh_buf + 0x40, &hdr.unknown3, 4);

    if (fwrite(gh_buf, 1, sizeof(gh_buf), out) != sizeof(gh_buf))
    {
        perror("Error writing global header");
        free(files);
        fclose(out);
        return 1;
    }

    // ------------------------------------------------------------------
    // Update stored_length and offset for each file
    // ------------------------------------------------------------------
    uint64_t offset = IMG_HEADER_HEADER_SIZE + (hdr.num_files * hdr.file_header_length);

    for (uint32_t i = 0; i < hdr.num_files; i++)
    {
        ImageWTYFileHeader* fh = &files[i];

        char filepath[1024];
        snprintf(filepath, sizeof(filepath), "%s/%s", dump_folder, fh->filename);

        FILE* in = fopen(filepath, "rb");
        if (!in)
        {
            fprintf(stderr, "Cannot open file '%s': %s\n", filepath, strerror(errno));
            free(files);
            fclose(out);
            return 1;
        }

        fseek(in, 0, SEEK_END);
        uint64_t original_length = ftell(in);
        rewind(in);
        fclose(in);

        uint64_t stored_length, padding;
        calculate_padding(original_length, &stored_length, &padding);

        fh->original_length = original_length;
        fh->stored_length = stored_length;
        fh->offset = offset;

        offset += stored_length;
    }

    // ------------------------------------------------------------------
    // Write File Headers
    // ------------------------------------------------------------------
    for (uint32_t i = 0; i < hdr.num_files; i++)
    {
        ImageWTYFileHeader* fh = &files[i];
        uint8_t* fh_buf = malloc(hdr.file_header_length);
        if (!fh_buf)
        {
            perror("Memory allocation failed for file header");
            free(files);
            fclose(out);
            return 1;
        }
        memset(fh_buf, 0, hdr.file_header_length);

        memcpy(fh_buf + 0x00, &fh->filename_length, sizeof(fh->filename_length));
        memcpy(fh_buf + 0x04, &fh->header_size, sizeof(fh->header_size));
        memcpy(fh_buf + 0x08, fh->maintype, 8);
        memcpy(fh_buf + 0x10, fh->subtype, 16);
        memcpy(fh_buf + 0x20, &fh->unknown0, sizeof(fh->unknown0));
        memcpy(fh_buf + 0x24, fh->filename, 256);
        memcpy(fh_buf + 0x124, &fh->stored_length, sizeof(fh->stored_length));
        memset(fh_buf + 0x128, 0, 4); // pad1
        memcpy(fh_buf + 0x12C, &fh->original_length, sizeof(fh->original_length));
        memset(fh_buf + 0x130, 0, 4); // pad2
        memcpy(fh_buf + 0x134, &fh->offset, sizeof(fh->offset));

        if (fseek(out, IMG_HEADER_HEADER_SIZE + i * hdr.file_header_length, SEEK_SET) != 0)
        {
            perror("Error seeking to file header position");
            free(fh_buf);
            free(files);
            fclose(out);
            return 1;
        }

        if (fwrite(fh_buf, 1, hdr.file_header_length, out) != hdr.file_header_length)
        {
            perror("Error writing file header");
            free(fh_buf);
            free(files);
            fclose(out);
            return 1;
        }

        free(fh_buf);
    }

    // ------------------------------------------------------------------
    // Write file data with padding
    // ------------------------------------------------------------------
    for (uint32_t i = 0; i < hdr.num_files; i++)
    {
        ImageWTYFileHeader* fh = &files[i];
        char filepath[1024];
        snprintf(filepath, sizeof(filepath), "%s/%s", dump_folder, fh->filename);

        FILE* in = fopen(filepath, "rb");
        if (!in)
        {
            fprintf(stderr, "Cannot open file '%s'\n", filepath);
            free(files);
            fclose(out);
            return 1;
        }

        if (fseek(out, fh->offset, SEEK_SET) != 0)
        {
            perror("Error seeking in output file");
            fclose(in);
            free(files);
            fclose(out);
            return 1;
        }

        uint8_t* buf = malloc(fh->original_length);
        size_t read_bytes = fread(buf, 1, fh->original_length, in);
        if (read_bytes != fh->original_length)
        {
            fprintf(stderr, "Error reading file %s: read %zu bytes instead of %u\n", filepath,
                    read_bytes, fh->original_length);
            free(buf);
            fclose(in);
            free(files);
            fclose(out);
            return 1;
        }

        fwrite(buf, 1, fh->original_length, out);
        free(buf);
        fclose(in);

        // Write padding
        if (fh->stored_length > fh->original_length)
        {
            uint8_t zero_buf[PADDING_ALIGNMENT] = {0};
            uint64_t pad_remaining = fh->stored_length - fh->original_length;
            while (pad_remaining > 0)
            {
                uint64_t write_now =
                    pad_remaining > PADDING_ALIGNMENT ? PADDING_ALIGNMENT : pad_remaining;
                fwrite(zero_buf, 1, write_now, out);
                pad_remaining -= write_now;
            }
        }

        printf("Packed: %s (original: %u, stored: %u)\n", fh->filename, fh->original_length,
               fh->stored_length);
    }

    free(files);
    fclose(out);

    printf("Repack completed successfully: %s\n", output_file);
    return 0;
}
