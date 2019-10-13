
#include "common.h"
#include "cpu.h"
#include "memory.h"
#include "romhandler.h"
#include <stdio.h>

unsigned char prgsize;
unsigned char chrsize;
unsigned char ines_flags6;
unsigned char singlescreen;
unsigned char ines_flags10;
unsigned char mapper;


int LoadRom(const char *Filename) {
    FILE * pFile;  //Create File Pointer
    long lSize;   //Need a variable to tell us the size
    unsigned char ROMHeader[16];
    unsigned int inesIdent;
    

    fopen_s(&pFile, Filename, "rb");     //Open the file, args r = read, b = binary

    if (pFile != NULL)  //If the file exists
    {
        //Reset();
        fseek(pFile, 0, SEEK_END); //point it at the end
        lSize = ftell(pFile) - 16; //Save the filesize
        rewind(pFile); //Start the file at the beginning again
        fread(&ROMHeader, 1, 16, pFile); //Read in the file
        inesIdent = (ROMHeader[0] + (ROMHeader[1] << 8) + (ROMHeader[2] << 16) + (ROMHeader[3] << 24));


        if (inesIdent != 0x1A53454E)
        {
            CPU_LOG("Header not on ROM file. MagicNumber = %x\n", inesIdent);
            fclose(pFile); //Close the file
        }
        else
        {
            prgsize = ROMHeader[4];
            chrsize = ROMHeader[5];
            ines_flags6 = ROMHeader[6];
            singlescreen = 0;
            ines_flags10 = ROMHeader[10];
            mapper = ((ROMHeader[6] >> 4) & 0xF) | (ROMHeader[7] & 0xF0);
            memset(CartridgeSRAM, 0, sizeof(CartridgeSRAM));
            CPU_LOG("prgsize=%d, chrsize=%d, flags6=%x, flags8=%x, flags10=%x mapper=%d\n", prgsize, chrsize, ines_flags6, ROMHeader[8],  ines_flags10, mapper);
            fseek(pFile, 16, SEEK_SET); //Point it past the header
            LoadRomToMemory(pFile, lSize);
            //fread(CPUMemory, 1, lSize, pFile); //Read in the file
            fclose(pFile); //Close the file
        }
        return 0;
    }
    else
    {
        //User cancelled, either way, do nothing.
        return 1;
    }

}