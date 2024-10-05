#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include "hw2.h"

void print_packet(unsigned int *packet) 
{
    unsigned int packet_type = (*packet >> 10);
    unsigned int length = (*packet & 0x3FF);
    unsigned int address = packet[2];
    unsigned int header_1 = packet[1];
    
    unsigned int requester_id = (header_1 >> 16);
    unsigned int tag = (header_1 >> 8) & 0xFF;
    unsigned int last_be = (header_1 >> 4) & 0xF;
    unsigned int first_be = header_1 & 0xF;

    if ((packet_type & 0xFFFFF) != 0 || ((packet_type >> 21) & 0x1) != 0)
    {
        printf("Error: Invalid packet type\n");
        return;
    }

    unsigned int is_write = (packet_type >> 20);

    printf("Packet Type: %s\n", is_write ? "Write" : "Read");
    printf("Address: %u\n", address);
    printf("Length: %u\n", length);
    printf("Requester ID: %u\n", requester_id);
    printf("Tag: %u\n", tag);
    printf("Last BE: %u\n", last_be);
    printf("1st BE: %u\n", first_be);

    if (is_write) 
    {
        printf("Data: ");
        unsigned int *data_ptr = packet + 3;
        for (unsigned int i = 0; i < length; i++) 
        {
            printf("%d ", *(data_ptr + i));
        }
        printf("\n");
    } 
    else 
    {
        printf("Data: \n");
    }
}

void store_values(unsigned int packets[], char *memory)
{
    int packet_start = 0;

    while (1)
    {
        int packet_type = (packets[packet_start] >> 24) & 0xFF;
        unsigned int address = packets[packet_start + 2];

        if (packet_type != 0x40 || address > 1000000)
        {
            break;
        }

        int length = packets[packet_start] & 0xFF;
        int first_be = packets[packet_start + 1] & 0xF;
        int last_be = (packets[packet_start + 1] >> 4) & 0xF;

        for (int i = 0; i < length; i++)
        {
            int mem_index = address + (i * 4);
            int data = packets[packet_start + 3 + i];

            if (i == 0) 
            {
                if (first_be & 1) memory[mem_index] = (char)(data & 0xFF);
                if (first_be & 2) memory[mem_index + 1] = (char)((data >> 8) & 0xFF);
                if (first_be & 4) memory[mem_index + 2] = (char)((data >> 16) & 0xFF);
                if (first_be & 8) memory[mem_index + 3] = (char)((data >> 24) & 0xFF);
            }
            else if (i == length - 1) 
            {
                if (last_be & 1) memory[mem_index] = (char)(data & 0xFF);
                if (last_be & 2) memory[mem_index + 1] = (char)((data >> 8) & 0xFF);
                if (last_be & 4) memory[mem_index + 2] = (char)((data >> 16) & 0xFF);
                if (last_be & 8) memory[mem_index + 3] = (char)((data >> 24) & 0xFF);
            }
            else
            {
                memory[mem_index] = (char)(data & 0xFF);
                memory[mem_index + 1] = (char)((data >> 8) & 0xFF);
                memory[mem_index + 2] = (char)((data >> 16) & 0xFF);
                memory[mem_index + 3] = (char)((data >> 24) & 0xFF);
            }
        }

        packet_start += 3 + length;
    }
}
unsigned int* create_completion(unsigned int packets[], const char *memory) 
{
    unsigned int length = packets[0] & 0xFFF; 
    unsigned int address = packets[2] & 0xFFFFFFFC;  
    unsigned int requester_id = packets[1] >> 16;  
    unsigned int tag = (packets[1] >> 8) & 0xFF;  
    unsigned int remaining_bytes = length * 4;  

    unsigned int completions_count = (length + 3);  
    unsigned int *completions = (unsigned int*)malloc(completions_count * sizeof(unsigned int));
    unsigned int index = 0;

    while (remaining_bytes > 0) {
        unsigned int current_length = (remaining_bytes > 16 || ((address & 0x3FFF) + remaining_bytes) > 0x4000) 
                                        ? ((0x4000 - (address & 0x3FFF)) / 4) 
                                        : (remaining_bytes / 4);
        completions[index++] = (0xA << 24) | current_length;  
        completions[index++] = (220 << 16) | remaining_bytes;  
        completions[index++] = (requester_id << 16) | (tag << 8) | (address & 0x7F);  

        for (unsigned int i = 0; i < current_length; i++) {
            unsigned int data;
            memcpy(&data, &memory[address], sizeof(data));
            completions[index++] = data;
            address += 4;
        }

        remaining_bytes -= current_length * 4;

        if ((address & 0x3FFF) == 0) {
            address += 4;  
        }
    }

    return completions;
}