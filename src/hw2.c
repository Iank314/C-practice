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
        unsigned int byte_count = length - 8;

        for (int i = 0; i < 4; i++) 
        {
           if (first_be & (1 << i)) 
           {
               byte_count++;
           }
           if (last_be & (1 << i)) 
           {
               byte_count++;
           }
}

        if ((address & 0xFFFFC000) != ((address + (length * 4) - 1) & 0xFFFFC000)) 
        {
            unsigned int traverse = length;
            unsigned int holder = address;

            while (traverse > 0) 
            {
                unsigned int boundary = (holder & 0xFFFFC000) + 0x4000;
                unsigned int max_length = (boundary - holder) / 4;
                unsigned int current_length = (traverse < max_length) ? traverse : max_length;

                unsigned int lower_address = (address == holder) ? (holder & 0x7F) : 0x00;
                completionpackets[indexforcompletion++] = (0x4A << 24) | current_length;
                completionpackets[indexforcompletion++] = (220 << 16) | byte_count;
                completionpackets[indexforcompletion++] = (requester_id << 16) | (tag << 8) | lower_address;

                for (unsigned int i = 0; i < current_length; i++) 
                {
                    completionpackets[indexforcompletion++] = ((unsigned char)memory[holder + (i * 4) + 3] << 24) |
                                                              ((unsigned char)memory[holder + (i * 4) + 2] << 16) |
                                                              ((unsigned char)memory[holder + (i * 4) + 1] << 8) |
                                                              ((unsigned char)memory[holder + (i * 4)]);
                }

                traverse -= current_length;
                holder += (current_length * 4);
                byte_count -= current_length * 4;
            }
        } 
        else 
        {
            completionpackets[indexforcompletion++] = (0x4A << 24) | length;
            completionpackets[indexforcompletion++] = (220 << 16) | byte_count;
            completionpackets[indexforcompletion++] = (requester_id << 16) | (tag << 8) | (address & 0x7F);

            for (unsigned int counter = 0; counter < length; counter++) 
            {
                completionpackets[indexforcompletion++] = ((unsigned char)memory[address + (counter * 4) + 3] << 24) |
                                                          ((unsigned char)memory[address + (counter * 4) + 2] << 16) |
                                                          ((unsigned char)memory[address + (counter * 4) + 1] << 8) |
                                                          ((unsigned char)memory[address + (counter * 4)]);
            }
        }

        index += 3;
    }

    return completionpackets;
}