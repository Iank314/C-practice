#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#include "hw2.h"

void print_packet(unsigned int *packet)
{
    int packet_type = (*(packet) >> 24) & 0xFF;
    int length = *(packet) & 0xFF;
    unsigned int address = *(packet + 2);
   
    int requester_id = (*(packet + 1) >> 16);
    int tag = (*(packet + 1) >> 8) & 0xFF;
    int last_be = (*(packet + 1) >> 4) & 0xF;
    int first_be = (*(packet + 1) & 0xF);


    if (packet_type == 0x40)
    {
        printf("Packet Type: Write\n");
    }
    else if (packet_type == 0x00)
    {
        printf("Packet Type: Read\n");
    }
    else
    {
        printf("Error: Invalid packet type 0x%X\n", packet_type);
        return;
    }


    printf("Address: %d\n", address);
    printf("Length: %d\n", length);
    printf("Requester ID: %d\n", requester_id);
    printf("Tag: %d\n", tag);
    printf("Last BE: %d\n", last_be);
    printf("1st BE: %d\n", first_be);


    if (packet_type == 0x40)
    {
        printf("Data: ");
        for (int i = 0; i < length; i++)
        {
            printf("%d ", (int)*(packet + 3 + i));
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
    int packet_type = (packets[0] >> 24) & 0xFF;
    if (packet_type != 0x00) 
    {
        return NULL;
    }

    unsigned int address = packets[2];
    int length = packets[0] & 0xFF;
    int requester_id = (packets[1] >> 16) & 0xFFFF;
    int tag = (packets[1] >> 8) & 0xFF;

    unsigned int *completion_packets = (unsigned int*)malloc((3 + length) * sizeof(unsigned int));
    if (completion_packets == NULL) return NULL;

    int completion_packet_index = 0;
    int byte_count = length * 4;

    while (length > 0)
    {
        int current_length = length;

        if ((address & 0x3FFF) + (current_length * 4) > 0x4000) 
        {
            current_length = (0x4000 - (address & 0x3FFF)) / 4;
        }

        completion_packets[completion_packet_index] = (0xDC << 24) | current_length;
        completion_packets[completion_packet_index + 1] = (220 << 24) | (requester_id << 16) | (tag << 8) | ((byte_count > 0xFFF) ? 0xFFF : byte_count); 
        completion_packets[completion_packet_index + 2] = address & 0x7FFFFFFF; 

        for (int i = 0; i < current_length; i++)
        {
            int mem_index = address + (i * 4);
            
            unsigned int data = 0;
            if (mem_index + 3 < 1000000) 
            {  
                data = (unsigned int)((memory[mem_index] & 0xFF) |
                                      (memory[mem_index + 1] & 0xFF) << 8 |
                                      (memory[mem_index + 2] & 0xFF) << 16 |
                                      (memory[mem_index + 3] & 0xFF) << 24);
            }
            completion_packets[completion_packet_index + 3 + i] = data;
        }

        address += current_length * 4;
        byte_count -= current_length * 4;
        length -= current_length;
        completion_packet_index += 3 + current_length;
    }

    return completion_packets;
}