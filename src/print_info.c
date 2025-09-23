#include "print_info.h"

#include <stddef.h> // For size_t
#include <stdio.h>
#include <string.h>

/**
 * @brief Print the main ImageWTY header in a compact, human-readable format.
 *
 * @param h Pointer to the ImageWTYHeader struct.
 */
void print_image_header(const ImageWTYHeader* h)
{
    printf("=== ImageWTY Header ===\n");
    printf("%-20s : %s\n", "Magic", h->magic);
    printf("%-20s : 0x%08X\n", "Header Version", h->header_version);
    printf("%-20s : %u bytes (0x%X)\n", "Header Size", h->header_size, h->header_size);
    printf("%-20s : 0x%08X\n", "Base RAM", h->base_ram);

    printf("\n--- Image Info ---\n");
    printf("%-20s : 0x%08X\n", "Format Version", h->format_version);
    printf("%-20s : %u bytes (%.2f MB)\n", "Total Size", h->total_image_size,
           h->total_image_size / 1024.0 / 1024.0);
    printf("%-20s : %u bytes\n", "Header + Align", h->header_size_aligned);
    printf("%-20s : %u bytes\n", "File Header Length", h->file_header_length);

    printf("\n--- USB & IDs ---\n");
    printf("%-20s : 0x%08X\n", "USB Product ID", h->usb_product_id);
    printf("%-20s : 0x%08X\n", "USB Vendor ID", h->usb_vendor_id);
    printf("%-20s : 0x%08X\n", "Hardware ID", h->hardware_id);
    printf("%-20s : 0x%08X\n", "Firmware ID", h->firmware_id);

    printf("\n--- Files ---\n");
    printf("%-20s : %u\n", "Number of Files", h->num_files);

    // Uncomment if needed for debugging unknown fields
    // printf("%-20s : %u\n", "Unknown Field #1", h->unknown1);
    // printf("%-20s : %u\n", "Unknown Field #2", h->unknown2);
    // printf("%-20s : %u\n", "Unknown Field #3", h->unknown3);
}

/**
 * @brief Print all file headers contained in the ImageWTY image.
 *
 * Each file entry is displayed in a compact block with key information
 * on fewer lines to avoid excessive vertical scrolling.
 *
 * @param files Pointer to array of ImageWTYFileHeader structs.
 * @param num_files Number of file headers in the array.
 */
void print_file_headers(ImageWTYFileHeader* files, uint32_t num_files)
{
    // Minimal column widths (can expand automatically)
    int w_no = 4;        // Column for file number
    int w_name = 30;     // Column for filename
    int w_maintype = 10; // Column for maintype
    int w_subtype = 10;  // Column for subtype
    int w_stored = 12;   // Column for stored length (with padding)
    int w_orig = 10;     // Column for original length
    int w_offset = 10;   // Column for offset

    // Adjust column widths dynamically based on content
    for (uint32_t i = 0; i < num_files; i++)
    {
        int len_name = strlen(files[i].filename);
        int len_maintype = strlen(files[i].maintype);
        int len_subtype = strlen(files[i].subtype);

        if (len_name > w_name)
            w_name = len_name;
        if (len_maintype > w_maintype)
            w_maintype = len_maintype;
        if (len_subtype > w_subtype)
            w_subtype = len_subtype;
    }

    // Calculate total width of the separator line
    int total_width = w_no + w_name + w_maintype + w_subtype + w_stored + w_orig + w_offset + 7;

    // Print header row
    printf("\n=== File Headers (%u files) ===\n", num_files);
    printf("%-*s %-*s %-*s %-*s %-*s %-*s %-*s\n", w_no, "No", w_name, "Filename", w_maintype,
           "Maintype", w_subtype, "Subtype", w_stored, "Size (pad)", w_orig, "Size", w_offset,
           "Offset");

    // Print dynamic separator line
    for (int i = 0; i < total_width; i++)
        putchar('-');
    putchar('\n');

    // Print each file row with optional description
    for (uint32_t i = 0; i < num_files; i++)
    {
        ImageWTYFileHeader* fh = &files[i];

        char offset_str[16];
        snprintf(offset_str, sizeof(offset_str), "0x%X", fh->offset);
        printf("%-*u %-*s %-*s %-*s %-*u %-*u %-*s\n", w_no, i + 1, w_name, fh->filename,
               w_maintype, fh->maintype, w_subtype, fh->subtype, w_stored, fh->stored_length,
               w_orig, fh->original_length, w_offset, offset_str);

        // Optional human-readable description
        const char* desc = describe_file(fh->filename);
        if (desc && strcmp(desc, "Unknown or unmapped file name") != 0)
        {
            printf("      -> %s\n", desc);
        }

        // Dynamic separator line after each file
        for (int j = 0; j < total_width; j++)
            putchar('-');
        putchar('\n');
    }
}

/**
 * @brief Structure representing a file name and its description.
 */
typedef struct
{
    const char* name;        ///< File name (e.g., boot.fex)
    const char* description; ///< Human-readable description of the file
} FileDesc;

/**
 * @brief Static lookup table containing known firmware file descriptions.
 *
 * Each entry maps a file name to its purpose or content, for reference
 * in tools or documentation.
 */
static const FileDesc file_table[] = {
    {"env.fex", "Environment variables stored in the env partition."},
    {"boot.fex", "Kernel image and ramdisk,\nwritten to the boot partition."},
    {"recovery.fex", "Recovery OS image,\nwritten to the recovery partition."},
    {"super.fex",
     "Sparse image containing system/vendor/product logical partitions,\nwritten to super."},
    {"boot-resource.fex", "Static data for the bootloader (BMP splash, fonts, magic.bin)."},
    {"vbmeta.fex", "Android Verified Boot (AVB) metadata."},
    {"vbmeta_system.fex", "AVB metadata for /system."},
    {"vbmeta_vendor.fex", "AVB metadata for /vendor."},
    {"sunxi_gpt.fex", "Modified GPT partition table written at the beginning of the card."},
    {"boot0_sdcard.fex", "Bootloader for SD card,\nwritten outside GPT partitions."},
    {"boot_package.fex", "Binary package for the bootloader\nwritten at offset 32800 blocks."},
    {"boot0_nand.fex", "Bootloader for NAND flash devices."},
    {"sunxi_mbr.fex", "Binary file for the flasher,\ncontains partition info."},
    {"dlinfo.fex", "Binary file for the flasher,\ncontains file-to-partition mapping."},
    {"usbtool.fex", "Used by PhoenixSuite USB programming tools."},
    {"aultools.fex", "Compiled Lua script for USB tools."},
    {"aultls32.fex", "Compiled Lua script for USB tools (32-bit)."},
    {"cardscript.fex", "Becomes script.cfg for PhoenixCard."},
    {"cardtool.fex", "Becomes card.scj for PhoenixCard."},
    {"fes1.fex", "Auxiliary loader for FES mode."},
    {"sunxi.fex", "Device tree blob (DTB), not flashed directly."},
    {"u-boot.fex", "U-Boot loader inside boot_package.fex."},
    {"u-boot-crash.fex", "Residual text file (likely legacy)."},
    {"sys_partition.fex", "Text partition layout (not flashed directly)."},
    {"sys_config.fex", "Text configuration from which DTB is generated."},
    {"config.fex", "Binary version of sys_config.fex."},
    {"split_xxxx.fex", "Signature block (magic.bin) used by the bootloader."},
    {"Vboot-resource.fex", "Checksum for boot-resource.fex."},
    {"Venv.fex", "Checksum for env.fex."},
    {"Vboot.fex", "Checksum for boot.fex."},
    {"Vsuper.fex", "Checksum for super.fex."},
    {"Vrecovery.fex", "Checksum for recovery.fex."},
    {"Vvbmeta.fex", "Checksum for vbmeta.fex."},
    {"Vvbmeta_system.fex", "Checksum for vbmeta_system.fex."},
    {"Vvbmeta_vendor.fex", "Checksum for vbmeta_vendor.fex."},
    {"toc0.fex", "Empty stub file for TOC0 format secondary bootloader."},
    {"toc1.fex", "Empty stub file for TOC1 (sunxi-secure package)."},
    {"arisc.fex", "Empty stub file (ARISC)."},
    {"usbtool_crash.fex", "Empty stub file (USB tool crash)."},
    {"board.fex", "Empty stub file (Board)."},
    {"sunxi_version.fex", "Build timestamp / firmware version information."},
    {"vmlinux.fex", "Bzip2-compressed archive containing the Linux kernel (vmlinux)."},
    {"Vvendor_boot.fex", "Checksum for vendor_boot.fex."},
    {"Vmisc.fex", "Checksum for misc.fex."},
    {"Vdtbo.fex", "Checksum for dtbo.fex."},
    {"VReserve0.fex", "Checksum for Reserve0.fex."},
    {"Reserve0.fex", "FAT16 partition containing panel configuration files\n(resolution, timing, "
                     "backlight, PWM, gamut) used by the bootloader or kernel at early boot."},
    {"vendor_boot.fex", "Android vendor boot image:\ncontains DTB, hash constants, ramdisk, init "
                        "scripts,\nand vendor-specific configuration loaded by the bootloader."},
    {"dtbo.fex", "Android DTBO (Device Tree Blob Overlay) file:\ncontains multiple DTBs for "
                 "hardware-specific configurations\napplied over the main device tree."},
    {"misc.fex", "Generic binary file used by the bootloader/kernel\nfor miscellaneous "
                 "configuration or control data."},
};

/**
 * @brief Return a human-readable description of a firmware file.
 *
 * @param name Full path or base name of the file.
 * @return Description string if known, otherwise "Unknown or unmapped file name".
 */
const char* describe_file(const char* name)
{
    // Extract the base name in case a full path is provided
    const char* base = strrchr(name, '/');
    if (!base)
        base = strrchr(name, '\\');
    base = (base) ? base + 1 : name;

    // Lookup the file in the table
    for (size_t i = 0; i < sizeof(file_table) / sizeof(file_table[0]); i++)
    {
        if (strcmp(file_table[i].name, base) == 0)
        {
            return file_table[i].description;
        }
    }

    // Return default string if file not found
    return "Unknown or unmapped file name";
}
