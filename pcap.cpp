#include <stdint.h>
#include <stdio.h>

//=============================================================================
// This is the header of a PCAP file
//=============================================================================
#pragma pack(push, 1)
struct pcap_file_header_t
{
    uint32_t    magic_number;
    uint16_t    major_version;
    uint16_t    minor_version;
    uint32_t    reserved1;
    uint32_t    reserved2;
    uint32_t    snaplen;
    uint32_t    link_type;
};
static pcap_file_header_t pcap_file_header;
#pragma pack(pop)
//=============================================================================


//=============================================================================
// This is a PCAP packet header
//=============================================================================
#pragma pack(push, 1)
struct pcap_packet_header_t
{
    uint32_t    ts_seconds;
    uint32_t    ts_nanoseconds;
    uint32_t    length1;
    uint32_t    length2;
};
static pcap_packet_header_t pcap_packet_header;
#pragma pack(pop)
//=============================================================================



//=============================================================================
// pcap_dump() - Writes RAM to disk as a PCAP file
//=============================================================================
void pcap_dump(uint8_t* ptr, uint64_t byte_count, FILE* ofile, uint64_t packet_size)
{
    // Create the PCAP header
    pcap_file_header.magic_number  = 0xA1B23C4D;
    pcap_file_header.major_version = 2;
    pcap_file_header.minor_version = 4;
    pcap_file_header.reserved1     = 0;
    pcap_file_header.reserved2     = 0;
    pcap_file_header.snaplen       = 65535;
    pcap_file_header.link_type     = 1;

     // Write the PCAP header
    fwrite(&pcap_file_header, sizeof(pcap_file_header), 1, ofile);

    // Build the PCAP packet header
    pcap_packet_header.ts_seconds     = 0;
    pcap_packet_header.ts_nanoseconds = 0;
    pcap_packet_header.length1        = packet_size;
    pcap_packet_header.length2        = packet_size;

    while (byte_count)
    {
        // Write the PCAP header
        fwrite(&pcap_packet_header, sizeof(pcap_packet_header), 1, ofile);

        // Write the packet-data
        fwrite(ptr, packet_size, 1, ofile);

        if (byte_count >= packet_size)
        {
            byte_count -= packet_size;
            ptr        += packet_size;
        }
        else byte_count = 0;
    }
}
//=============================================================================
