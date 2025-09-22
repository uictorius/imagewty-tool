/**
 * @file img_repack.c
 * @brief Implements repacking of extracted IMAGEWTY directories into a single
 *        IMAGEWTY file (global header + file headers + file data).
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
 * @brief Repack a directory dump into a new IMAGEWTY file
 *        following the IMAGEWTY format standard.
 *
 * Steps:
 *  - Update virtual files if needed.
 *  - Load global header and file headers from image.cfg.
 *  - Write global header.
 *  - Write each file header (1024 bytes each by default).
 *  - Copy actual file data into the final IMAGEWTY file.
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

    /* Ensure V-files are up-to-date before repacking */
    update_vfiles_if_needed(dump_folder);

    /* Load the original header and file list from image.cfg */
    char cfg_path[1024];
    snprintf(cfg_path, sizeof(cfg_path), "%s/image.cfg", dump_folder);

    ImageWTYHeader hdr;
    ImageWTYFileHeader* files = NULL;

    if (load_image_config(cfg_path, &hdr, &files) != 0 || !files)
    {
        fprintf(stderr, "Failed to load image.cfg from '%s'\n", cfg_path);
        return 1;
    }

    /* Open the output IMAGEWTY file */
    FILE* out = fopen(output_file, "wb");
    if (!out)
    {
        fprintf(stderr, "Cannot create output file '%s': %s\n", output_file, strerror(errno));
        free(files);
        return 1;
    }

    /* ------------------------------------------------------------------
     * Write Global Header
     * ------------------------------------------------------------------ */
    uint8_t gh_buf[IMG_HEADER_HEADER_SIZE];
    memset(gh_buf, 0, sizeof(gh_buf));

    /* Copy all known fields */
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

    /* Write the entire header buffer */
    if (fwrite(gh_buf, 1, sizeof(gh_buf), out) != sizeof(gh_buf))
    {
        perror("Error writing global header");
        free(files);
        fclose(out);
        return 1;
    }

    /* ------------------------------------------------------------------
     * Write File Headers (1024 bytes each by default)
     * ------------------------------------------------------------------ */
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

        /* Copy fields into the header buffer */
        memcpy(fh_buf + 0x00, &fh->filename_length, sizeof(fh->filename_length));
        memcpy(fh_buf + 0x04, &fh->header_size, sizeof(fh->header_size));
        memcpy(fh_buf + 0x08, fh->maintype, 8);
        memcpy(fh_buf + 0x10, fh->subtype, 16);
        memcpy(fh_buf + 0x20, &fh->unknown0, sizeof(fh->unknown0));
        memcpy(fh_buf + 0x24, fh->filename, 256);
        memcpy(fh_buf + 0x124, &fh->stored_length, sizeof(fh->stored_length));
        memcpy(fh_buf + 0x128, &fh->pad1, sizeof(fh->pad1));
        memcpy(fh_buf + 0x12C, &fh->original_length, sizeof(fh->original_length));
        memcpy(fh_buf + 0x130, &fh->pad2, sizeof(fh->pad2));
        memcpy(fh_buf + 0x134, &fh->offset, sizeof(fh->offset));

        /* Seek to correct offset for this file header */
        if (fseek(out, IMG_HEADER_HEADER_SIZE + i * hdr.file_header_length, SEEK_SET) != 0)
        {
            perror("Error seeking to file header position");
            free(fh_buf);
            free(files);
            fclose(out);
            return 1;
        }

        /* Write buffer to file */
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

    /* ------------------------------------------------------------------
     * Write Actual File Data
     * ------------------------------------------------------------------ */
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

        /* Move output file pointer to the correct offset */
        if (fseek(out, fh->offset, SEEK_SET) != 0)
        {
            perror("Error seeking in output file");
            fclose(in);
            free(files);
            fclose(out);
            return 1;
        }

        uint8_t* buf = malloc(fh->original_length);
        if (!buf)
        {
            perror("Memory allocation failed");
            fclose(in);
            free(files);
            fclose(out);
            return 1;
        }

        if (fread(buf, 1, fh->original_length, in) != fh->original_length)
        {
            fprintf(stderr, "Error reading file data: %s\n", filepath);
            free(buf);
            fclose(in);
            free(files);
            fclose(out);
            return 1;
        }

        if (fwrite(buf, 1, fh->original_length, out) != fh->original_length)
        {
            fprintf(stderr, "Error writing file data: %s\n", filepath);
            free(buf);
            fclose(in);
            free(files);
            fclose(out);
            return 1;
        }

        free(buf);
        fclose(in);
        printf("Packed: %s (%u bytes)\n", fh->filename, fh->original_length);
    }

    free(files);
    fclose(out);

    printf("Repack completed successfully: %s\n", output_file);
    return 0;
}
