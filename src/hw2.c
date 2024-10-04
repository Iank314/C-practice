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
    int length = packets[0] & 0xFF; 
    unsigned int address = packets[2]; 
    int first_be = packets[1] & 0xF; 
    int last_be = (packets[1] >> 4) & 0xF; 


    for (int i = 0; i < length; i++) 
    {
        int mem_index = address + (i * 4); 


        int data = packets[3 + i]; 

        if (i == 0) 
        {
            if (first_be & 1) memory[mem_index] = (char)(data & 0xFF);          // Byte 0
            if (first_be & 2) memory[mem_index + 1] = (char)((data >> 8) & 0xFF);  // Byte 1
            if (first_be & 4) memory[mem_index + 2] = (char)((data >> 16) & 0xFF); // Byte 2
            if (first_be & 8) memory[mem_index + 3] = (char)((data >> 24) & 0xFF); // Byte 3
        }
        else if (i == length - 1) 
        {
            if (last_be & 1) memory[mem_index] = (char)(data & 0xFF);          // Byte 0
            if (last_be & 2) memory[mem_index + 1] = (char)((data >> 8) & 0xFF);  // Byte 1
            if (last_be & 4) memory[mem_index + 2] = (char)((data >> 16) & 0xFF); // Byte 2
            if (last_be & 8) memory[mem_index + 3] = (char)((data >> 24) & 0xFF); // Byte 3
        }
        else 
        {
            memory[mem_index] = (char)(data & 0xFF);           // Byte 0
            memory[mem_index + 1] = (char)((data >> 8) & 0xFF);  // Byte 1
            memory[mem_index + 2] = (char)((data >> 16) & 0xFF); // Byte 2
            memory[mem_index + 3] = (char)((data >> 24) & 0xFF); // Byte 3
        }
    }
}
unsigned int* create_completion(unsigned int packets[], const char *memory)
{
    (void)packets;
    (void)memory;
    return NULL;
}