#pragma once
#include <stdint.h>
#include <stdio.h>
void pcap_dump(uint8_t* ptr, uint64_t byte_count, FILE* ofile, uint64_t packet_size);
