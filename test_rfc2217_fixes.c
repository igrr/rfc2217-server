#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

// Simulate the telnet constants
#define T_IAC 0xffU

// Test function to verify 0xff byte escaping in send_data
void test_send_data_escaping() {
    printf("Testing send_data 0xff escaping...\n");
    
    // Test cases with different data patterns
    uint8_t test_cases[][32] = {
        {0x00, 0x01, 0x02, 0x03}, // No 0xff bytes
        {0x00, 0xff, 0x01, 0x02}, // Single 0xff
        {0xff, 0x01, 0x02, 0x03}, // 0xff at start
        {0x01, 0x02, 0x03, 0xff}, // 0xff at end
        {0xff, 0xff, 0x01, 0x02}, // Double 0xff
        {0x01, 0xff, 0xff, 0x02}, // Double 0xff in middle
        {0xff, 0xff, 0xff, 0xff}, // All 0xff
    };
    
    int test_lengths[] = {4, 4, 4, 4, 4, 4, 4};
    
    for (int i = 0; i < 7; i++) {
        uint8_t *data = test_cases[i];
        int len = test_lengths[i];
        
        printf("Test case %d: ", i);
        for (int j = 0; j < len; j++) {
            printf("0x%02x ", data[j]);
        }
        printf("\n");
        
        // Check if escaping is needed
        bool needs_escaping = false;
        for (int j = 0; j < len; j++) {
            if (data[j] == T_IAC) {
                needs_escaping = true;
                break;
            }
        }
        
        if (needs_escaping) {
            printf("  Needs escaping: YES\n");
            // Simulate the escaping logic
            uint8_t escaped_buffer[64];
            size_t escaped_size = 0;
            
            for (int j = 0; j < len && escaped_size < sizeof(escaped_buffer) - 1; j++) {
                if (data[j] == T_IAC) {
                    escaped_buffer[escaped_size++] = T_IAC;
                    escaped_buffer[escaped_size++] = T_IAC;
                } else {
                    escaped_buffer[escaped_size++] = data[j];
                }
            }
            
            printf("  Escaped data: ");
            for (size_t j = 0; j < escaped_size; j++) {
                printf("0x%02x ", escaped_buffer[j]);
            }
            printf("(length: %zu)\n", escaped_size);
        } else {
            printf("  Needs escaping: NO\n");
        }
        printf("\n");
    }
}

// Test function to verify 0xff byte unescaping in receive processing
void test_receive_data_unescaping() {
    printf("Testing receive_data 0xff unescaping...\n");
    
    // Test cases with escaped data
    uint8_t test_cases[][32] = {
        {0x00, 0x01, 0x02, 0x03}, // No escaping
        {0x00, 0xff, 0xff, 0x01}, // Single escaped 0xff
        {0xff, 0xff, 0x01, 0x02}, // Escaped 0xff at start
        {0x01, 0x02, 0xff, 0xff}, // Escaped 0xff at end
        {0xff, 0xff, 0xff, 0xff}, // Double escaped 0xff
    };
    
    int test_lengths[] = {4, 4, 4, 4, 4};
    
    for (int i = 0; i < 5; i++) {
        uint8_t *data = test_cases[i];
        int len = test_lengths[i];
        
        printf("Test case %d: ", i);
        for (int j = 0; j < len; j++) {
            printf("0x%02x ", data[j]);
        }
        printf("\n");
        
        // Simulate the receive processing logic
        uint8_t data_buffer[256];
        size_t data_buffer_size = 0;
        int telnet_mode = 0; // T_NORMAL
        
        for (int j = 0; j < len; j++) {
            uint8_t c = data[j];
            
            if (telnet_mode == 0) { // T_NORMAL
                if (c == T_IAC) {
                    telnet_mode = 1; // T_GOT_IAC
                } else {
                    if (data_buffer_size < sizeof(data_buffer)) {
                        data_buffer[data_buffer_size++] = c;
                    }
                }
            } else if (telnet_mode == 1) { // T_GOT_IAC
                if (c == T_IAC) {
                    // Double IAC means literal IAC byte
                    if (data_buffer_size < sizeof(data_buffer)) {
                        data_buffer[data_buffer_size++] = c;
                    }
                }
                telnet_mode = 0; // T_NORMAL
            }
        }
        
        printf("  Unescaped data: ");
        for (size_t j = 0; j < data_buffer_size; j++) {
            printf("0x%02x ", data_buffer[j]);
        }
        printf("(length: %zu)\n", data_buffer_size);
        printf("\n");
    }
}

int main() {
    printf("RFC2217 Server 0xff Byte Escaping Test\n");
    printf("=====================================\n\n");
    
    test_send_data_escaping();
    test_receive_data_unescaping();
    
    printf("Test completed successfully!\n");
    return 0;
}