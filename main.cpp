//=============================================================================
// physram - Utility for manipulating RAM via physical addresses
//
// Author: Doug Wolf
//
// To run this program, use this command line:
//     sudo ./physram <address> [size]
//           [-save <filename>]
//           [-load <filename>]
//           [-pcap <filename>]
//           [-packet <size>]
//           [-clear [value]]
//
// If run without the "-save", "-pcap", "-load" "-clear" switches, the contents 
// of RAM will be dumped to stdout
//
//-----------------------------------------------------------------------------
//   Date    Vers  Who  What
//-----------------------------------------------------------------------------
// 05-Jul-24  1.1  DWW  First numbered version
// 13-Jul-24  1.2  DWW  Added optional "value" on the "-clear" switch
// 30-Sep-25  1.3  DWW  Added "-pcap" and "-packet" options
//=============================================================================
#define REVISION "1.3"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <exception>
#include <fcntl.h>
#include <cstring>
#include <string>
#include <cstdarg>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "PhysMem.h"
#include "pcap.h"

using namespace std;

uint64_t regionAddr;
uint64_t regionSize = 0x100000;
uint64_t packetSize = 4096;
string   filename;
bool     save  = false;
bool     load  = false;
bool     pcap  = false;
bool     clear = false;
int      clearValue = 0;


PhysMem RAM;

void execute();
void parseCommandLine(const char** argv);
void showHelp();

//=============================================================================
// main() - Dumps the contents of the contiguous buffer to stdout
//=============================================================================
int main(int argc, const char** argv)
{
    parseCommandLine(argv);

    try
    {
        execute();
    }
    catch(const std::exception& e)
    {
        fprintf(stderr, "%s\n", e.what());
        exit(1);
    }
}
//=============================================================================


//=============================================================================
// showHelp() - Display the command line usage 
//=============================================================================
void showHelp()
{
    printf("physram v%s\n", REVISION);
    printf("physram <address> [size]\n");
    printf("        [-clear [value]]\n");
    printf("        [-save <filename>]\n");
    printf("        [-load <filename>]\n");
    printf("        [-pcap <filename>]\n");
    printf("        [-packet <size>]\n");
    exit(1);
}
//=============================================================================


//=============================================================================
// to_u64() - Converts an ASCII string to a 64-bit unsigned int
//=============================================================================
uint64_t to_u64(const char* str)
{
    char token[100];

    // Point to the output buffer
    char *out=token;

    // Skip over whitespace
    while (*str == 32 || *str == 9) ++str;

    // Loop through every character of the input string
    while (true)
    {
        // Fetch the next character
        int c = *str++;

        // If this character is an underscore, skip it
        if (c == '_') continue;

        // If this character is the end of the token, break
        if (c == 0  || c ==  '\n' || c == '\r' || c == 32 || c == 9) break;

        // Output the character to the token buffer.
        *out++ = c;

        // Don't overflow our token buffer
        if ((out - token) == (sizeof(token)-1)) break;
    }

    // Nul-terminate the buffer
    *out = 0;

    // And return it binary value
    return strtoull(token, nullptr, 0);
}
//=============================================================================



//=============================================================================
// parseCommandLine() - Parses the command line parameters
//
// On Exit:  regionAddr = The physical address of the buffer
//           regionSize = The size of the buffer in RAM
//=============================================================================
void parseCommandLine(const char** argv)
{
    int i=1, index = 0, value = 0;

    while (true)
    {
        // Fetch the next token from the command line
        const char* token = argv[i++];

        // If we're out of tokens, we're done
        if (token == nullptr) break;

        // Turn the token into std::string
        string option = token;        

        // If it's the "-save" switch...
        if (option == "-save" && argv[i])
        {
            save = true;
            filename = argv[i++];
            continue;
        }

        if (option == "-load" && argv[i])
        {
            load = true;
            filename = argv[i++];
            continue;
        }

        if (option == "-pcap" && argv[i])
        {
            pcap = true;
            filename = argv[i++];
            continue;
        }


        if (option == "-packet" && argv[i])
        {
            value = to_u64(argv[i++]);
            if (value >= 1 && value <= 9600) packetSize = value;
            continue;
        }


        // If it's the "-clear" switch...
        if (option == "-clear")
        {
            clear = true;
            if (argv[i] && argv[i][0] >= '0' && argv[i][0] <= '9')
            {
                clearValue = (uint8_t)to_u64(argv[i++]);
            }
            continue;
        }

        // Store this parameter into either "address" or "data"
        if (++index == 1)
            regionAddr = to_u64(token);
        else
            regionSize = to_u64(token);
    }

    // If the user failed to give us an address, that's fatal
    if (regionAddr == 0) showHelp();
}
//=============================================================================



//=============================================================================
// perform_save() - Saves the memory region to a binary file
//=============================================================================
void perform_save(uint8_t* ptr)
{
    FILE* ofile = fopen(filename.c_str(), "w");
    if (ofile == nullptr)
    {
        fprintf(stderr, "Can't create %s\n", filename.c_str());
        exit(1);
    } 
    fwrite(ptr, 1, regionSize, ofile);
    fclose(ofile);
    exit(1);       
}
//=============================================================================


//=============================================================================
// perform_pcap() - Saves the memory region to a pcap file
//=============================================================================
void perform_pcap(uint8_t* ptr)
{
    FILE* ofile = fopen(filename.c_str(), "w");
    if (ofile == nullptr)
    {
        fprintf(stderr, "Can't create %s\n", filename.c_str());
        exit(1);
    } 
    pcap_dump(ptr, regionSize, ofile, packetSize);
    fclose(ofile);
    exit(1);       
}
//=============================================================================



//=============================================================================
// perform_load() - Loads a file into the memory region
//=============================================================================
void perform_load(uint64_t file_size, uint8_t* dest_ptr)
{
    const uint32_t BLOCK_SIZE = 0x100000;
    uint64_t bytes_loaded = 0;

    // Get a char* to the filename
    const char* fn = filename.c_str();     

    FILE* ifile = fopen(fn, "r");
    if (ifile == nullptr)
    {
        fprintf(stderr, "physram: can't open %s\n", fn);
        exit(1);
    }

    // Load the file into RAM
    while (bytes_loaded < file_size)
    {
        int64_t this_block_size = fread(dest_ptr, 1, BLOCK_SIZE, ifile);
        if (this_block_size < 0)
        {
            fprintf(stdout, "physram: failure while reading %s\n", fn);
            exit(1);            
        }
        dest_ptr     += this_block_size;
        bytes_loaded += this_block_size;
    }

    // Close the file, and exit, we're done
    fclose(ifile);
    exit(0);
}
//=============================================================================



//=============================================================================
// get_file_size() - Returns the size of the input file
//=============================================================================
uint64_t get_file_size()
{
    struct stat64 ss;

    // Get a char* to the filename
    const char* fn = filename.c_str();     

    // Stat the file, and complain if we can't
    if (stat64(fn, &ss) < 0)

    {
        fprintf(stderr, "physram: can't stat %s\n", fn);
        exit(1);
    }   

    // Fetch the file-size in bytes
    uint64_t file_size = ss.st_size;

    // And hand the file-size to the caller
    return file_size;
}
//=============================================================================



//=============================================================================
// execute() - Main-line execution
//=============================================================================
void execute()
{
    uint64_t file_size = 0;

    // If we're going to ber loading a file, make sure it will fit
    if (load)
    {
        file_size = get_file_size();
        if (file_size > regionSize)
        {
            fprintf
            (
                stderr,
                "physram: file size of %lu bytes too big to fit "
                "into region of %lu bytes\n",
                file_size,
                regionSize
            );
            exit(1);
        }
    }

    // Map the contiguous buffer into user-space
    RAM.map(regionAddr, regionSize);

    // Fetch a pointer to the first byte of the buffer
    uint8_t* ptr = RAM.bptr();

    // If we're supposed to clear the RAM, make it so
    if (clear)
    {
        memset(ptr, clearValue, regionSize);
        exit(0);
    }
    
    // If we're supposed to save the RAM into a file...
    if (save) perform_save(ptr);

    // If we're suppoed to save the RAM as a PCAP file...
    if (pcap) perform_pcap(ptr);

    // If we're supposed to load data into RAM from a file...
    if (load) perform_load(file_size, ptr);

    // Otherwise, just copy the RAM buffer to stdout.
    while (regionSize--)
    {
        putc(*ptr++, stdout);
    }
}
//=============================================================================

