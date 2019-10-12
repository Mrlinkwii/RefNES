#include "memory.h"
#include "cpu.h"
#include "ppu.h"
#include "common.h"
#include "romhandler.h"

//All memory is defined here and handled with memory.c, just to keep it all in one place.

unsigned char CPUMemory[0x10000]; //64kb memory Main CPU Memory

char* ROMCart;

//#define MEM_LOGGING

void CleanUpMem() {
    if (ROMCart != NULL) {
        free(ROMCart);
    }
}
void MemReset() {
    memset(CPUMemory, 0, 0x10000); //Reset only IO registers, everything else is "indetermined" just in case a game resets.
    
}



void CopyRomToMemory() {
    if (prgsize > 1) {
        if (mapper == 9) {
            memcpy(&CPUMemory[0x8000], ROMCart, 0x2000);
            //prg size is 16kb, multiply it by 2 to get 8k sizes, -3 for last 3
            memcpy(&CPUMemory[0xA000], ROMCart + (((prgsize * 2) - 3) * 8192), 0x6000);
        }
        else {
            memcpy(&CPUMemory[0x8000], ROMCart, 0x4000);
            memcpy(&CPUMemory[0xC000], ROMCart + ((prgsize - 1) * 16384), 0x4000);
        }
        
    }
    else {
        memcpy(&CPUMemory[0x8000], ROMCart, 0x4000);
        memcpy(&CPUMemory[0xC000], ROMCart, 0x4000);
    }

    if (chrsize > 0) {
        memcpy(&PPUMemory[0x0000], ROMCart + (prgsize * 16384), 0x2000);
    }
}

void LoadRomToMemory(FILE * RomFile, long lSize) {

    if (ROMCart == NULL) {
        ROMCart = (char*)malloc(lSize);
    }
    else {
        realloc(ROMCart, lSize);
    }
    
    fread(ROMCart, 1, lSize, RomFile);
    //CopyRomToMemory();
}



/* CPU Addressing Functions */
// These are called from memGetAddr

unsigned short MemAddrAbsolute(bool iswrite, bool writeonly) {
    unsigned short fulladdress;
    
    fulladdress = CPUMemory[PC + 2] << 8 | CPUMemory[PC + 1];
#ifdef MEM_LOGGING
    CPU_LOG("Absolute Mem %x PC = %x \n", fulladdress, PC);
#endif
    PCInc = 3;

    if (iswrite == false)
        CPUIncrementCycles(2);
    else
        CPUIncrementCycles(2);

    return fulladdress;

}

unsigned short MemAddrAbsoluteY(bool iswrite, bool haspenalty, bool writeonly) {
    unsigned short fulladdress = ((CPUMemory[(PC + 2)] << 8) | CPUMemory[PC + 1]);
    if (((fulladdress & 0xFF00) != ((fulladdress + Y) & 0xFF00) && haspenalty) || iswrite)
        CPUIncrementCycles(1);

    fulladdress += Y;
#ifdef MEM_LOGGING
    CPU_LOG("AbsoluteY Mem %x PC = %x \n", fulladdress, PC);
#endif
    PCInc = 3;
    if (iswrite == false)
        CPUIncrementCycles(2);
    else
        CPUIncrementCycles(2);
    return fulladdress;
}

unsigned short MemAddrAbsoluteX(bool iswrite, bool haspenalty, bool writeonly) {
    unsigned short fulladdress = ((CPUMemory[(PC + 2)] << 8) | CPUMemory[PC + 1]);
    if (((fulladdress & 0xFF00) != ((fulladdress + X) & 0xFF00) && haspenalty) || iswrite)
        CPUIncrementCycles(1);

    fulladdress += X;
#ifdef MEM_LOGGING
    CPU_LOG("AbsoluteX Mem %x PC = %x \n", fulladdress, PC);
#endif
    PCInc = 3;
    if (iswrite == false)
        CPUIncrementCycles(2);
    else
        CPUIncrementCycles(2);

    return fulladdress;
}

unsigned short MemAddrPreIndexed(bool iswrite, bool writeonly) {
    unsigned short fulladdress;
    unsigned short address = CPUMemory[PC + 1] + X;

    fulladdress = (CPUMemory[(address + 1) & 0xFF] << 8) | CPUMemory[address & 0xFF];
#ifdef MEM_LOGGING
    CPU_LOG("Pre Indexed Mem %x PC = %x \n", fulladdress, PC);
#endif
    PCInc = 2;
    CPUIncrementCycles(4);

    return fulladdress;
}

unsigned short MemAddrPostIndexed(bool iswrite, bool haspenalty, bool writeonly) {
    unsigned short fulladdress;
    unsigned short address = CPUMemory[PC + 1];

    fulladdress = ((CPUMemory[(address + 1) & 0xFF] << 8) | CPUMemory[address & 0xFF]);
    if(((fulladdress & 0xFF00) != ((fulladdress + Y) & 0xFF00) && haspenalty) || iswrite)
        CPUIncrementCycles(1);

    fulladdress += Y;
#ifdef MEM_LOGGING
    CPU_LOG("Post Indexed Mem %x PC = %x \n", fulladdress, PC);
#endif
    PCInc = 2;
    CPUIncrementCycles(3);

    return fulladdress;
}

unsigned short MemAddrImmediate(bool iswrite) {
    //We don't read from memory here, we just read the PC + 1 position value
    unsigned short fulladdress = PC + 1;
#ifdef MEM_LOGGING
    CPU_LOG("Immediate %x PC = %x \n", fulladdress, PC);
#endif
    PCInc = 2;

    return fulladdress;
}

unsigned short MemAddrZeroPage(bool iswrite, bool writeonly) {
    unsigned short fulladdress = CPUMemory[PC + 1];
#ifdef MEM_LOGGING
    CPU_LOG("Zero Page %x PC = %x \n", fulladdress, PC);
#endif
    PCInc = 2;
    if (iswrite == false || writeonly)
        CPUIncrementCycles(1);
    else
        CPUIncrementCycles(2);
    return fulladdress;
}

unsigned short MemAddrZeroPageIndexed(bool iswrite, bool writeonly) {
    unsigned short fulladdress;
    

    if ((Opcode & 0xC0) == 0x80 && (Opcode & 0x3) > 1) {
        fulladdress = (CPUMemory[PC + 1] + Y) & 0xFF;
        //CPU_LOG("BANANA Zero Page Y Indexed %x PC = %x Opcode %x", fulladdress, PC, Opcode);
    }
    else {
        fulladdress = (CPUMemory[PC + 1] + X) & 0xFF;
    }
#ifdef MEM_LOGGING
    CPU_LOG("Zero Page Indexed %x PC = %x \n", fulladdress, PC);
#endif
    PCInc = 2;
    CPUIncrementCycles(2);
    return fulladdress;
}

//Generic memread interfact for use most of the time, determines the standard address modes.

unsigned short memGetAddr(bool iswrite, bool haspenalty, bool writeonly = false) {
    unsigned short value;

    switch (Opcode & 0x1C) {
    case 0x0: //ALU Pre-Indexed Indirect (X) / Control+RWM Immediate
        if (!(Opcode & 0x1)) {//Not ALU or Undocumented
            value = MemAddrImmediate(iswrite);
        }
        else {
            value = MemAddrPreIndexed(iswrite, writeonly);
        }
        break;
    case 0x4: //Zero Page
        value = MemAddrZeroPage(iswrite, writeonly);
        break;
    case 0x8: //Immediate
        if (!(Opcode & 0x1)) {
#ifdef MEM_LOGGING
            CPU_LOG("BANANA Implied not immediate??\n");
#endif
        }
        value = MemAddrImmediate(iswrite);
        break;
    case 0xC: //Absolute        
        value = MemAddrAbsolute(iswrite, writeonly);
        break;
    case 0x10://Post-indexed indirect (Y)
        value = MemAddrPostIndexed(iswrite, haspenalty, writeonly);
        break;
    case 0x14://ZeroPage Indexed, X or Y
            value = MemAddrZeroPageIndexed(iswrite, writeonly);
        break;
    case 0x18://Absolute,Y 
        if (!(Opcode & 0x1)) {
            return 0;
        }
        value = MemAddrAbsoluteY(iswrite, haspenalty, writeonly);
        break;
    case 0x1C://Absolute,X
        if ((Opcode & 0x3) > 1 && (Opcode & 0xC0) == 0x80) {
            //CPU_LOG("BANANA Reading Absolute Y instead of Absolute X Opcode %x", Opcode);
            value = MemAddrAbsoluteY(iswrite, haspenalty, writeonly);
        }
        else {
            value = MemAddrAbsoluteX(iswrite, haspenalty, writeonly);
        }
        break;

    }
    return value;
}

/* Generic Interfaces */
unsigned char memRead(bool haspenalty) {
    unsigned short address = memGetAddr(false, haspenalty);
    //CPU_LOG("Reading from address %x, value %x\n", address, CPUMemory[address]);
    if (address >= 0x2000 && address < 0x4000) {
        return PPUReadReg(address);
    }
    else
    if (address >= 0x4000 && address < 0x4020) {

        return ioRegRead(address);
    }
    else
    if (address >= 0x4020 && address < 0x6000)
    {
        return CartridgeExpansionRead(address);
    }
    else if(address >= 0x6000 && address < 0x8000)
    {
        CPU_LOG("Reading External RAM %x\n", address - 0x6000);
        return ExpansionRAM2[address - 0x6000];
    }
    else
    if (address >= 0x800 && address < 0x2000) {
        //CPU_LOG("Wrapping CPU mem address %x\n", address & 0x7FF);
        address = address & 0x7FF;
    }
    //CPU_LOG("value = %x\n", CPUMemory[address]);
    return CPUMemory[address];
}

void memWrite(unsigned char value, bool writeonly) {
    unsigned short address = memGetAddr(true, false, writeonly);
#ifdef MEM_LOGGING
    CPU_LOG("Writing to address %x, value %x\n", address, value);
#endif

    if (address >= 0x8000) { //Mapper
        //CPU_LOG("MAPPER HANDLER write to address %x with %x\n", address, value);
        MapperHandler(address, value);
        return;
    }else
    if ((address >= 0x2000 && address < 0x4000)) {
        PPUWriteReg(address, value);
        return;
    }
    else
    if (address >= 0x4000 && address < 0x4020) {
        ioRegWrite(address, value);
        return;
    }
    else
    if (address >= 0x4020 && address < 0x6000)
    {
        return CartridgeExpansionWrite(address, value);
    }
    else if(address >= 0x6000 && address < 0x8000)
    {
        CPU_LOG("Writing to External RAM %x\n", address - 0x6000);
        ExpansionRAM2[address - 0x6000] = value;
        return;
    }
    else
    if (address >= 0x800 && address < 0x2000) {
        //CPU_LOG("Wrapping CPU mem address %x\n", address & 0x7FF);
        address = address & 0x7FF;
    }
    //CPU_LOG("\n");
    CPUMemory[address] = value;
}

unsigned short memReadPC(unsigned short address) {
    unsigned short value;

    if (address >= 0x2000 && address < 0x4000) {
        return PPUReadReg(address);
    }
    else
    if (address >= 0x800 && address < 0x2000) {
        //CPU_LOG("Wrapping CPU mem address %x\n", address & 0x7FF);
        address = address & 0x7FF;
    }
    else if (address >= 0x6000 && address < 0x8000)
    {
        CPU_LOG("Reading External RAM PC %x\n", address - 0x6000);
        value = (ExpansionRAM2[(address - 0x6000)+1] << 8) | ExpansionRAM2[(address - 0x6000)];
        return value;
    }
    //CPU_LOG("value = %x\n", (CPUMemory[address + 1] << 8) | CPUMemory[address]);
    value = (CPUMemory[address + 1] << 8) | CPUMemory[address];
    return value;

}

unsigned short memReadPCIndirect() {
    unsigned short address;
    unsigned short value;
    unsigned short masked;
    address = (CPUMemory[PC + 2] << 8) | CPUMemory[PC + 1];
    if (address >= 0x2000 && address < 0x4000) {
        //CPU_LOG("Wrapping PPU reg address %x\n", 0x2000 + (address & 0x7));
        return PPUReadReg(address);
    } else
    if (address >= 0x800 && address < 0x2000) {
        //CPU_LOG("Wrapping CPU mem address %x\n", address & 0x7FF);
        address = address & 0x7FF;        
    }
    else if (address >= 0x6000 && address < 0x8000)
    {
        CPU_LOG("Reading External RAM Indirect %x\n", address - 0x6000);
        value = (ExpansionRAM2[(address - 0x6000) + 1] << 8) | ExpansionRAM2[(address - 0x6000)];
        return value;
    }
    masked = (address & 0xFF00) | ((address+1) & 0xFF);
    //CPU_LOG("Indirect read from %x ", address);
    value = (CPUMemory[masked] << 8) | CPUMemory[address];
    //CPU_LOG("\n");
    //CPU_LOG("returned value %x, values at address upper = %x lower = %x\n", value, CPUMemory[address + 1], CPUMemory[address]);
    
    return value;

}

void memWriteValue(unsigned short address, unsigned char value) {
    if (address >= 0x2000 && address < 0x4000) {
        CPU_LOG("Wrapping PPU reg address %x\n", 0x2000 + (address & 0x7));
        PPUWriteReg(address, value);
        return;
    }
    else
    if (address >= 0x800 && address < 0x2000) {
        CPU_LOG("Wrapping CPU mem address %x\n", address & 0x7FF);
        address = address & 0x7FF;
    }
    else if (address >= 0x6000 && address < 0x8000)
    {
        CPU_LOG("Writing External RAM %x\n", address - 0x6000);
       ExpansionRAM2[(address - 0x6000)] = value;
       return;
    }
    CPUMemory[address] = value;
}


unsigned char memReadValue(unsigned short address) {
    if (address >= 0x2000 && address < 0x4000) {
        CPU_LOG("Wrapping PPU reg address %x\n", 0x2000 + (address & 0x7));
        return PPUReadReg(address);
    }
    else
    if (address >= 0x4000 && address < 0x4020) {

        return ioRegRead(address);
    }
    else
    if (address >= 0x800 && address < 0x2000) {
        CPU_LOG("Wrapping CPU mem address %x\n", address & 0x7FF);
        address = address & 0x7FF;
    }
    else if (address >= 0x6000 && address < 0x8000)
    {
        CPU_LOG("Reading External RAM %x\n", address - 0x6000);
        return ExpansionRAM2[(address - 0x6000)];
    }
    //CPU_LOG("single value = %x\n", CPUMemory[address]);
    return CPUMemory[address];
}