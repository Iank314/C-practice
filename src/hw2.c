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

unsigned int* create_completion(unsigned int packets[], const char *memory) 
{
    int index = 0;
    int indexforcompletion = 0;
    unsigned int *completionpackets = (unsigned int*)malloc(1000000);

    while (((packets[index] >> 30) & 0x3) == 0x0) 
    {
        unsigned int length = packets[index] & 0x3FF;
        unsigned int address = packets[index + 2];
        unsigned int header_1 = packets[index + 1];
        unsigned int requester_id = (header_1 >> 16);
        unsigned int tag = (header_1 >> 8) & 0xFF;
        unsigned int first_be = header_1 & 0xF;
        unsigned int last_be = (header_1 >> 4) & 0xF;
        unsigned int byte_count = length * 4;
        unsigned int remaining_bytes = byte_count;

        unsigned int initial_byte_count = __builtin_popcount(first_be) * 4;
        unsigned int final_byte_count = __builtin_popcount(last_be) * 4;

        while (remaining_bytes > 0) 
        {
            unsigned int current_length;
            unsigned int boundary = (address & ~0x3FFF) + 0x4000;

            if (address + remaining_bytes > boundary) 
            {
                current_length = (boundary - address) / 4;
            } 
            else 
            {
                current_length = remaining_bytes / 4;
            }

            unsigned int lower_address = address & 0x7F;

            completionpackets[indexforcompletion++] = (0x25 << 25) | (current_length);

            if (remaining_bytes == byte_count) 
            {
                completionpackets[indexforcompletion++] = (220 << 16) | initial_byte_count;
            } 
            else if (remaining_bytes <= final_byte_count) 
            {
                completionpackets[indexforcompletion++] = (220 << 16) | remaining_bytes;
                remaining_bytes = remaining_bytes - 4;
            } 
            else 
            {
                completionpackets[indexforcompletion++] = (220 << 16) | final_byte_count;
            }

            completionpackets[indexforcompletion++] = (requester_id << 16) | (tag << 8) | lower_address;

            for (unsigned int i = 0; i < current_length; i++) 
            {
                unsigned int data = ((unsigned char)memory[address] << 0) |
                                    ((unsigned char)memory[address + 1] << 8) |
                                    ((unsigned char)memory[address + 2] << 16) |
                                    ((unsigned char)memory[address + 3] << 24);

                if (remaining_bytes < 4) 
                {
                    unsigned int valid_bytes = remaining_bytes;
                    unsigned int mask = (1 << (valid_bytes * 8)) - 1;
                    data &= mask;
                }

                completionpackets[indexforcompletion++] = data;
                address += 4;
                remaining_bytes -= 4;
            }

            if ((address & 0x3FFF) == 0) 
            {
                address += 4;
            }
        }

        index += 3;
    }

    return completionpackets;
}