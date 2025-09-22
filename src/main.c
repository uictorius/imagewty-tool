/**
 * @file main.c
 * @brief Command-line interface for IMAGEWTY-Tool.
 *
 * This program provides commands to inspect, extract, repack,
 * and configure Allwinner firmware images handled by IMAGEWTY-Tool.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "checksum.h"
#include "config_file.h"
#include "img_extract.h"
#include "img_header.h"
#include "img_repack.h"
#include "print_info.h"

#define VERSION "1.0.0"
#define MIN_ARGS 3 /**< Minimum number of arguments for valid execution. */

/**
 * @enum Command
 * @brief Available tool commands.
 */
typedef enum
{
    CMD_INVALID = 0, /**< Invalid/unknown command. */
    CMD_INFO,        /**< Show information about an image. */
    CMD_EXTRACT,     /**< Extract files from an image. */
    CMD_REPACK,      /**< Repack files into a new image. */
    CMD_CONFIG       /**< Load and display a config file. */
} Command;

/**
 * @brief Display program usage instructions in a clean, user-friendly format.
 * @param prog Name of the executable.
 */
static void usage(const char* prog)
{
    printf("=== IMAGEWTY Tool ===\n");
    printf("Version: %s\n\n", VERSION);

    printf("Usage:\n");
    printf(
        "  %s info <image.img>                  Display main image header and file information\n",
        prog);
    printf("  %s extract <image.img>               Extract all files from the firmware image\n",
           prog);
    printf("  %s repack <folder.dump> <new_image.img>  Repack extracted files into a new firmware "
           "image\n",
           prog);
    printf("  %s config <image.cfg>                Inspect or display IMAGEWTY configuration "
           "files\n\n",
           prog);

    printf("Notes:\n");
    printf("  - Extraction creates a folder named <image>.dump with all extracted files and a "
           "generated image.cfg.\n");
    printf("  - Repacking recalculates all V-file checksums automatically.\n\n");

    printf("For more information, visit: https://github.com/uictorius/imagewty-tool\n");
}

/**
 * @brief Parse a command string into Command enum.
 * @param cmd_str Command string.
 * @return Corresponding Command enum value.
 */
static Command parse_command(const char* cmd_str)
{
    if (strcmp(cmd_str, "info") == 0)
        return CMD_INFO;
    if (strcmp(cmd_str, "extract") == 0)
        return CMD_EXTRACT;
    if (strcmp(cmd_str, "repack") == 0)
        return CMD_REPACK;
    if (strcmp(cmd_str, "config") == 0)
        return CMD_CONFIG;
    return CMD_INVALID;
}

/**
 * @brief Open an IMAGEWTY image and validate its header.
 * @param path Path to the image file.
 * @param hdr Pointer to ImageWTYHeader to populate.
 * @return FILE* on success, NULL on failure.
 */
static FILE* open_image_file(const char* path, ImageWTYHeader* hdr)
{
    FILE* f = fopen(path, "rb");
    if (!f)
    {
        perror("Error opening image");
        return NULL;
    }

    /* Read the image header (function exits on failure) */
    read_image_header(f, hdr);

    if (strncmp(hdr->magic, IMAGEWTY_MAGIC, 8) != 0)
    {
        fprintf(stderr, "Error: '%s' is not a valid IMAGEWTY image or may be encrypted.\n", path);
        fclose(f);
        return NULL;
    }

    return f;
}

/**
 * @brief Handle the 'info' command.
 * @param path Path to the image file.
 * @return 0 on success, non-zero on failure.
 */
static int handle_info(const char* path)
{
    ImageWTYHeader hdr;
    FILE* f = open_image_file(path, &hdr);
    if (!f)
        return 1;

    print_image_header(&hdr);

    ImageWTYFileHeader* files = read_all_file_headers(f, hdr.num_files, hdr.file_header_length);
    if (files)
    {
        print_file_headers(files, hdr.num_files);
        free(files);
    }

    fclose(f);
    return 0;
}

/**
 * @brief Handle the 'config' command.
 * @param path Path to the config file.
 * @return 0 on success, non-zero on failure.
 */
static int handle_config(const char* path)
{
    ImageWTYHeader hdr;
    ImageWTYFileHeader* files = NULL;

    int res = load_image_config(path, &hdr, &files);
    if (res == 0)
    {
        print_image_header(&hdr);
        print_file_headers(files, hdr.num_files);
    }
    else if (res == 1)
    {
        fprintf(stderr, "Failed to open config file '%s'\n", path);
    }
    else if (res == 2)
    {
        fprintf(stderr, "Config file '%s' contains invalid fields\n", path);
    }

    free_file_list(files);
    return (res == 0) ? 0 : 1;
}

/**
 * @brief Program entry point.
 */
int main(int argc, char* argv[])
{
    if (argc < MIN_ARGS)
    {
        usage(argv[0]);
        return 1;
    }

    Command cmd = parse_command(argv[1]);

    switch (cmd)
    {
    case CMD_INFO:
        return handle_info(argv[2]);

    case CMD_EXTRACT:
        return extract_image(argv[2]);

    case CMD_REPACK:
        if (argc < 4)
        {
            usage(argv[0]);
            return 1;
        }
        return repack_image(argv[2], argv[3]);

    case CMD_CONFIG:
        return handle_config(argv[2]);

    default:
        usage(argv[0]);
        return 1;
    }
}
