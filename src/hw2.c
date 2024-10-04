#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#include "hw2.h"

void store_values(unsigned int packets[], char *memory)
{
    (void)packets;
    (void)memory;
}

unsigned int* create_completion(unsigned int packets[], const char *memory)
{
    (void)packets;
    (void)memory;
	return NULL;
}

unsigned int extract_bits(unsigned int value, int start, int count) {
    unsigned int mask = (1 << count) - 1;
    return (value >> start) & mask;
}

void print_packet(unsigned int packet[])
{
    int packet_type = extract_bits(packet[0], 24, 8);
    int length = extract_bits(packet[0], 0, 8);
    int address = packet[1] & 0xFFFFFFFF;
    int requester_id = extract_bits(packet[1], 0, 16);
    int tag = extract_bits(packet[1], 16, 8);
    int last_be = extract_bits(packet[2], 28, 4);
    int first_be = extract_bits(packet[2], 24, 4);

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
        for (int i = 3; i < 3 + length; i++) 
        {
            int payload_data = (int) packet[i];
            printf("%d ", payload_data);
        }
        printf("\n");
    } 
    else 
    {
        printf("Data: \n");
    }
}
