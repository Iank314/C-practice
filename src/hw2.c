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
    int complete = 0;
    unsigned int *completionpackets = (unsigned int*)malloc(10000000);

    while(((packets[index] >> 30) & 0x3) == 0x0) 
    {
        unsigned int length = packets[index] & ((1 << 10) - 1);
        unsigned int requester_id = (packets[index + 1] >> 16) & ((1 << 16) - 1);
        unsigned int tag = (packets[index + 1] >> 8) & ((1 << 8) - 1);
        unsigned int last_be = (packets[index + 1] >> 4) & ((1 << 4) - 1);
        unsigned int first_be = packets[index + 1] & ((1 << 4) - 1);
        unsigned int address = *(packets + index + 2);
        unsigned int byte_count = (length - 2) << 2;

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
            unsigned int loop = length;
            unsigned int holder = address;
            unsigned int byte = byte_count;
            while(loop > 0) 
            {
               unsigned int boundary = (holder & 0xFFFFC000) + 0x4000;
               unsigned int max_length = (boundary - holder) / 4;
               unsigned int packet_length;
               if (loop < max_length) 
               {
                     packet_length = loop;
               } 
               else  
               {
                  packet_length = max_length;
               }

                unsigned int loweraddress;
                if (address == holder) 
                {
                    loweraddress = holder & 0x7F;
                } 
                else 
                {
                   loweraddress = 0x00;
                }
                completionpackets[complete] = (0x4A << 24) | packet_length;
                completionpackets[complete + 1] = (220 << 16) | byte;
                completionpackets[complete + 2] = (requester_id << 16) | (tag << 8) | loweraddress;
                unsigned char *ptr = (unsigned char *)(memory + holder);
                for (unsigned int i = 0; i < packet_length; i++) 
                {
                   completionpackets[complete + 3 + i] = (ptr[3] << 24) | 
                                          (ptr[2] << 16) | 
                                          (ptr[1] << 8)  | 
                                           ptr[0];
                    ptr += 4;
                }
                complete += (3 + packet_length);
                loop -= packet_length;
                holder += (packet_length * 4);

                unsigned int bit_count = 0;
                for(int j = 0; j < 4; j++) 
                {
                    if (first_be & (1 << j)) 
                    {
                        bit_count++;
                    }
                }
                byte = byte_count - ((packet_length - 1) * 4) - bit_count;
            }
        }
 else
 {
     completionpackets[complete] = ((0x4A & 0xFF) << 24) | (length & 0x3FF); 
     completionpackets[complete + 1] = ((220 & 0xFFFF) << 16) | (byte_count & 0xFFFF);
     completionpackets[complete + 2] = ((requester_id & 0xFFFF) << 16) | ((tag & 0xFF) << 8) | (address & 0x7F);

       for (unsigned int data = 0; data < length; ++data)
         {
            unsigned int mem_address = address + (data * 4);
             completionpackets[complete + 3 + data] = 
            ((unsigned int)(memory[mem_address + 3] & 0xFF) << 24) | 
            ((unsigned int)(memory[mem_address + 2] & 0xFF) << 16) | 
            ((unsigned int)(memory[mem_address + 1] & 0xFF) << 8) | 
            ((unsigned int)(memory[mem_address] & 0xFF));
         }

         complete += (3 + length);
 }
         index += 3;
    }

    return completionpackets;
}