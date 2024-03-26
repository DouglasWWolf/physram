//=============================================================================
// physram - Utility for manipulating RAM via physical addresses
//
// Author: Doug Wolf
//
// To run this program, use this command line:
//     sudo ./physram <address> [size] [-save <filename>] [-clear]
//
// If run without the "-save" or "-clear" switches, the contents of RAM will
// be dumped to stdout
//
//=============================================================================

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <exception>
#include <fcntl.h>
#include <cstring>
#include <string>
#include <cstdarg>
#include "PhysMem.h"

using namespace std;

uint64_t regionAddr;
uint64_t regionSize = 0x100000;
string   filename;
bool     save = false;
bool     clear = false;

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
    printf("physram <address> [size] [-clear] [-save <filename>]\n");
    exit(1);
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
    int i=1, index = 0;

    while (true)
    {
        // Fetch the next token from the command line
        const char* token = argv[i++];

        // If we're out of tokens, we're done
        if (token == nullptr) break;

        // If it's the "-save" switch...
        if (strcmp(token, "-save") == 0)
        {
            save = true;
            token = argv[i++];
            if (token == nullptr) showHelp();
            filename = token;
            continue;
        }

        // If it's the "-clear" switch...
        if (strcmp(token, "-clear") == 0)
        {
            clear = true;
            continue;
        }

        // Store this parameter into either "address" or "data"
        if (++index == 1)
            regionAddr = strtoull(token, 0, 0);
        else
            regionSize = strtoull(token, 0, 0);
    }

    // If the user failed to give us an address, that's fatal
    if (regionAddr == 0) showHelp();
}
//=============================================================================






//=============================================================================
// execute() - Main-line execution
//=============================================================================
void execute()
{
    // Map the contiguous buffer into user-space
    RAM.map(regionAddr, regionSize);

    // Fetch a pointer to the first byte of the buffer
    uint8_t* ptr = RAM.bptr();

    // If we're supposed to clear the RAM, make it so
    if (clear)
    {
        memset(ptr, 0, regionSize);
        exit(0);
    }
    
    // If we're supposed to save the RAM into a file...
    if (save)
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

    // Otherwise, just copy the RAM buffer to stdout.
    while (regionSize--)
    {
        putc(*ptr++, stdout);
    }
}
//=============================================================================




