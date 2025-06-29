#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

// Raw 512-byte buffer for license data
typedef struct {
    uint8_t raw_data[512];      // Raw 512-byte block
} license_block_t;

// Calculate checksum for a data block (matches the original getCheckSum function)
// Original function only sums 2 bytes (i <= 1 means i = 0, 1)
uint16_t getCheckSum(uint8_t* data) {
    uint16_t sum = 0;
    for (int i = 0; i <= 1; i++) {
        sum += data[i];
    }
    return sum;
}

// Check existing licenses on USB device
int checkLicenses(const char* usb_path) {
    FILE* stream = fopen(usb_path, "r+b");
    if (stream == NULL) {
        printf("Error: Could not open USB device at %s\n", usb_path);
        return -1;
    }
    
    // Seek to the license block (512 bytes from end) - matches 0xfffffe00 offset
    if (fseek(stream, -512, SEEK_END) != 0) {
        printf("Error: Could not seek to license block\n");
        fclose(stream);
        return -1;
    }
    
    license_block_t license_data;
    
    // Read the license block
    if (fread(license_data.raw_data, 1, 512, stream) != 512) {
        printf("Error: Could not read license block from USB\n");
        fclose(stream);
        return -1;
    }
    
    fclose(stream);
    
    // Extract license count from offset 0x34 (52 decimal) - little-endian
    uint16_t license_count = license_data.raw_data[0x34] | 
                            (license_data.raw_data[0x35] << 8);
    
    printf("Debug: Raw license count bytes: 0x%02X 0x%02X\n", 
           license_data.raw_data[0x34], license_data.raw_data[0x35]);
    printf("Debug: License count: %d\n", license_count);
    
    // If no licenses, return 0
    if (license_count == 0) {
        printf("No licenses found\n");
        return 0;
    }
    
    // Based on assembly analysis, getCheckSum only processes 2 bytes each time
    // The three calls are at offsets: 0x34 (var_1e4), 0x65 (var_1b3), 0x69 (var_1af)
    uint16_t checksum1 = getCheckSum(&license_data.raw_data[0x34]);  // 2 bytes
    uint16_t checksum2 = getCheckSum(&license_data.raw_data[0x65]);  // 2 bytes
    uint16_t checksum3 = getCheckSum(&license_data.raw_data[0x69]);  // 2 bytes
    uint16_t total_checksum = checksum1 + checksum2 + checksum3;
    
    // Extract stored checksum from offset 0x1FC - little-endian
    uint16_t stored_checksum = license_data.raw_data[0x1FC] | 
                              (license_data.raw_data[0x1FD] << 8);
    
    printf("Debug: Checksum1 (0x34): %d\n", checksum1);
    printf("Debug: Checksum2 (0x65): %d\n", checksum2);  
    printf("Debug: Checksum3 (0x69): %d\n", checksum3);
    printf("Debug: Total calculated: %d\n", total_checksum);
    printf("Debug: Stored checksum: %d\n", stored_checksum);
    
    // Validate checksum
    if (total_checksum != stored_checksum) {
        printf("Error: License checksum validation failed (calculated: %d, stored: %d)\n", 
               total_checksum, stored_checksum);
        return -1;
    }
    
    printf("Found %d valid licenses\n", license_count);
    return license_count;
}

// Create a new license block on the USB device
int createLicense(const char* usb_path, uint16_t license_count) {
    FILE* stream = fopen(usb_path, "r+b");
    if (stream == NULL) {
        printf("Error: Could not open USB device at %s\n", usb_path);
        return -1;
    }
    
    // Seek to the license block location (512 bytes from end)
    if (fseek(stream, -512, SEEK_END) != 0) {
        printf("Error: Could not seek to license block\n");
        fclose(stream);
        return -1;
    }
    
    license_block_t license_data;
    
    // Initialize with random data (matching the decrement function behavior)
    srand(time(NULL));
    for (int i = 0; i < 512; i++) {
        license_data.raw_data[i] = (uint8_t)(rand() & 0xFF);
    }
    
    // Set the license count at offset 0x34 (little-endian)
    license_data.raw_data[0x34] = license_count & 0xFF;
    license_data.raw_data[0x35] = (license_count >> 8) & 0xFF;
    
    // Set some data at the checksum locations
    // The checksum function only reads 2 bytes from each location
    license_data.raw_data[0x65] = 0x01;  // Some data for checksum2
    license_data.raw_data[0x66] = 0x02;
    
    license_data.raw_data[0x69] = 0x03;  // Some data for checksum3  
    license_data.raw_data[0x6A] = 0x04;
    
    // Calculate checksum (only 2 bytes each)
    uint16_t checksum1 = getCheckSum(&license_data.raw_data[0x34]);  // license_count bytes
    uint16_t checksum2 = getCheckSum(&license_data.raw_data[0x65]);  // 0x01 + 0x02 = 3
    uint16_t checksum3 = getCheckSum(&license_data.raw_data[0x69]);  // 0x03 + 0x04 = 7
    uint16_t total_checksum = checksum1 + checksum2 + checksum3;
    
    // Store checksum at offset 0x1FC (little-endian)
    license_data.raw_data[0x1FC] = total_checksum & 0xFF;
    license_data.raw_data[0x1FD] = (total_checksum >> 8) & 0xFF;
    
    printf("Debug: Creating license with count %d\n", license_count);
    printf("Debug: Checksum1: %d, Checksum2: %d, Checksum3: %d\n", 
           checksum1, checksum2, checksum3);
    printf("Debug: Total checksum: %d\n", total_checksum);
    
    // Write the license block
    if (fwrite(license_data.raw_data, 1, 512, stream) != 512) {
        printf("Error: Could not write license block to USB\n");
        fclose(stream);
        return -1;
    }
    
    // Flush to ensure data is written
    fflush(stream);
    fclose(stream);
    
    printf("Successfully created license block with %d licenses\n", license_count);
    return 0;
}

// Decrement licenses (compatible with the original decrement function)
int decrementLicenses(const char* usb_path) {
    FILE* stream = fopen(usb_path, "r+b");
    if (stream == NULL) {
        printf("Error: Could not open USB device at %s\n", usb_path);
        return -1;
    }
    
    // Seek to the license block (512 bytes from end)
    if (fseek(stream, -512, SEEK_END) != 0) {
        printf("Error: Could not seek to license block\n");
        fclose(stream);
        return -1;
    }
    
    license_block_t license_data;
    
    // Read the license block
    if (fread(license_data.raw_data, 1, 512, stream) != 512) {
        printf("Error: Could not read license block from USB\n");
        fclose(stream);
        return -1;
    }
    
    // Extract current license count - little-endian
    uint16_t license_count = license_data.raw_data[0x34] | 
                            (license_data.raw_data[0x35] << 8);
    
    // Check if we have licenses to decrement
    if (license_count == 0) {
        printf("Error: No licenses available to decrement\n");
        fclose(stream);
        return -1;
    }
    
    // Decrement the license count
    license_count--;
    printf("New license count: %d\n", license_count);
    
    // Fill buffer with random data (matching original behavior)
    srand(time(NULL));
    for (int i = 0; i < 512; i++) {
        license_data.raw_data[i] = (uint8_t)(rand() & 0xFF);
    }
    
    // Write back the decremented license count
    license_data.raw_data[0x34] = license_count & 0xFF;
    license_data.raw_data[0x35] = (license_count >> 8) & 0xFF;
    
    // Restore the checksum data locations
    license_data.raw_data[0x65] = 0x01;
    license_data.raw_data[0x66] = 0x02;
    license_data.raw_data[0x69] = 0x03;
    license_data.raw_data[0x6A] = 0x04;
    
    // Recalculate checksum
    uint16_t checksum1 = getCheckSum(&license_data.raw_data[0x34]);
    uint16_t checksum2 = getCheckSum(&license_data.raw_data[0x65]);
    uint16_t checksum3 = getCheckSum(&license_data.raw_data[0x69]);
    uint16_t total_checksum = checksum1 + checksum2 + checksum3;
    
    // Store new checksum
    license_data.raw_data[0x1FC] = total_checksum & 0xFF;
    license_data.raw_data[0x1FD] = (total_checksum >> 8) & 0xFF;
    
    // Seek back to write position
    if (fseek(stream, -512, SEEK_END) != 0) {
        printf("Error: Could not seek to write position\n");
        fclose(stream);
        return -1;
    }
    
    // Write the updated license block
    if (fwrite(license_data.raw_data, 1, 512, stream) != 512) {
        printf("Error: Could not write updated license block to USB\n");
        fclose(stream);
        return -1;
    }
    
    // Flush and close
    fflush(stream);
    fclose(stream);
    
    printf("Successfully decremented licenses. Remaining: %d\n", license_count);
    return 0;
}

// Main function with command line interface
int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <usb_path> [command] [license_count]\n", argv[0]);
        printf("  usb_path: Path to USB device (e.g., /dev/sdb)\n");
        printf("  command: 'check', 'create', or 'decrement'\n");
        printf("  license_count: Number of licenses to create (only for 'create')\n");
        printf("\nExamples:\n");
        printf("  %s /dev/sdb check           # Check existing licenses\n", argv[0]);
        printf("  %s /dev/sdb create 5        # Create 5 licenses\n", argv[0]);
        printf("  %s /dev/sdb decrement       # Decrement license count\n", argv[0]);
        return 1;
    }
    
    const char* usb_path = argv[1];
    
    if (argc == 2) {
        // Default to check if no command specified
        int license_count = checkLicenses(usb_path);
        if (license_count >= 0) {
            printf("Licenses remaining: %d\n", license_count);
        }
        return (license_count >= 0) ? 0 : 1;
    }
    
    const char* command = argv[2];
    
    if (strcmp(command, "check") == 0) {
        int license_count = checkLicenses(usb_path);
        if (license_count >= 0) {
            printf("Licenses remaining: %d\n", license_count);
        }
        return (license_count >= 0) ? 0 : 1;
    }
    else if (strcmp(command, "create") == 0) {
        if (argc < 4) {
            printf("Error: License count required for create command\n");
            return 1;
        }
        
        int license_count = atoi(argv[3]);
        if (license_count <= 0) {
            printf("Error: License count must be positive\n");
            return 1;
        }
        
        if (createLicense(usb_path, license_count) == 0) {
            printf("License creation successful!\n");
            return 0;
        } else {
            printf("License creation failed!\n");
            return 1;
        }
    }
    else if (strcmp(command, "decrement") == 0) {
        if (decrementLicenses(usb_path) == 0) {
            printf("License decrement successful!\n");
            return 0;
        } else {
            printf("License decrement failed!\n");
            return 1;
        }
    }
    else {
        printf("Error: Unknown command '%s'\n", command);
        printf("Valid commands: check, create, decrement\n");
        return 1;
    }
}
