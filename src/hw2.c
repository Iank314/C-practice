#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

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
unsigned int* create_completion(unsigned int packets[], const char *memory) {
    unsigned int address = packets[2] & 0xFFFFFFFC;
    unsigned int length = packets[0] & 0x3FF;
    unsigned int requester_id = packets[1] >> 16;
    unsigned int tag = (packets[1] >> 8) & 0xFF;
    unsigned int lower_address = address & 0x7F;

    unsigned int byte_count = length * 4;
    unsigned int* completion = (unsigned int*)malloc((3 + length) * sizeof(unsigned int));

    completion[0] = (0x50 << 24) | length;
    completion[1] = (220 << 16) | byte_count;
    completion[2] = (requester_id << 16) | (tag << 8) | lower_address;

    unsigned int boundary = 0x4000;
    unsigned int bytes_to_boundary = boundary - (address % boundary);

    for (unsigned int i = 0; i < length; i++) 
    {
        unsigned int mem_index = address + i * 4;

        if ((mem_index + 4) > boundary) {
            unsigned int split_length = (bytes_to_boundary / 4);
            
            unsigned int* new_completion = (unsigned int*)malloc((3 + split_length) * sizeof(unsigned int));
            new_completion[0] = (0x50 << 24) | split_length;
            new_completion[1] = (220 << 16) | (split_length * 4);
            new_completion[2] = (requester_id << 16) | (tag << 8) | (mem_index & 0x7F);

            for (unsigned int j = 0; j < split_length; j++) 
            {
                new_completion[3 + j] = *(unsigned int *)(memory + mem_index + j * 4);
            }

            return new_completion;
        }
        completion[3 + i] = *(unsigned int *)(memory + mem_index);
    }

    return completion;
}