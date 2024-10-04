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

unsigned int extract_bits(unsigned int value, int start, int count) 
{
    unsigned int mask = (1 << count) - 1;
    return (value >> start) & mask;
}

void print_packet(unsigned int packet[])
{
    int packet_type = extract_bits(packet[0], 24, 8);
    int length = extract_bits(packet[0], 0, 8);
    int address = packet[1] & 0xFFFFFFFF;
    int requester_id = extract_bits(packet[1], 16, 16);
    int tag = extract_bits(packet[1], 8, 8);
    int last_be = extract_bits(packet[2], 28, 4);
    int first_be = extract_bits(packet[2], 24, 4);

    printf("%d\n", packet_type);
    printf("%d\n", address);
    printf("%d\n", length);
    printf("%d\n", requester_id);
    printf("%d\n", tag);
    printf("%d\n", last_be);
    printf("%d\n", first_be);

    if (packet_type == 0x40)
    {
        for (int i = 3; i < 6; i++) 
        {
            int payload_data = (int) packet[i];
            printf("%d\n", payload_data);
        }
    }
}
