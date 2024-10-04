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
    else 
    {
        printf("Packet Type: Read\n");
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
    int packet_index = 0;
    while (true) 
    {
        int packet_type = (packets[packet_index] >> 24) & 0xFF;
        if (packet_type != 0x40) break;  
        
        int length = packets[packet_index] & 0xFF;
        unsigned int address = packets[packet_index + 2] & 0xFFFFFFFC;  
        if (address >= (1 << 20)) break;  

        int first_be = (packets[packet_index + 1] >> 24) & 0xF;
        int last_be = (packets[packet_index + 1] >> 28) & 0xF;

        for (int i = 0; i < length; i++) 
        {
            unsigned int data = packets[packet_index + 3 + i];
            int mem_index = address + (i * 4);

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

        packet_index += (3 + length);
    }
}

unsigned int* create_completion(unsigned int packets[], const char *memory)
{
    (void)packets;
    (void)memory;
    return NULL;
}