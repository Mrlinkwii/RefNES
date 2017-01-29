
#include "romhandler.h"
#include "cpu.h"
#include "ppu.h"
#include "memory.h"

unsigned char SRwrites = 0; //Shift register for MMC1 wrappers
unsigned char MMCbuffer;
unsigned char MMCcontrol;
unsigned char MMCIRQCounterLatch = 0;
unsigned char MMCIRQCounter = 0;
unsigned char MMCIRQEnable = 0;
unsigned char MMC2Latch0 = 0xFE;
unsigned char MMC2Latch1 = 0xFE;

void MMC2SetLatch(unsigned char latch, unsigned char value) {
	if (latch == 0) {
		MMC2Latch0 = value;
	}
	else {
		MMC2Latch1 = value;
	}
}

void MMC3IRQCountdown() {
	if (mapper != 4) return;

	if (MMCIRQEnable == 1) {
		if (--MMCIRQCounter == 0) {
			MMCIRQCounter = MMCIRQCounterLatch;
			CPUPushAllStack();
			PC = memReadPC(0xFFFE);
		}
	}
}

void MMC3ChangePRG(unsigned char PRGNum) {
	unsigned char inversion = (MMCcontrol >> 7) & 0x1;
	unsigned char bankmode = (MMCcontrol >> 6) & 0x1;
	
	if ((MMCcontrol & 0x7) <= 5) {  //CHR bank		
		if (chrsize == 0) return;
		if ((MMCcontrol & 0x7) < 2) { //2k CHR banks
			unsigned short address = inversion ? 0x1000 : 0x0000;
			address += (MMCcontrol & 0x7) * 0x800;
			CPU_LOG("MAPPER Switching to 2K CHR-ROM number %d at 0x%x\n", PRGNum, address);
			memcpy(&PPUMemory[address], ROMCart + ((prgsize) * 16384) + (PRGNum * 1024), 0x800);
		}
		else { //1K CHR banks
			unsigned short address = inversion ? 0x0000 : 0x1000;
			address += ((MMCcontrol & 0x7) - 2) * 0x400;
			CPU_LOG("MAPPER Switching to 1K CHR-ROM number %d at 0x%x\n", PRGNum, address);
			memcpy(&PPUMemory[address], ROMCart + ((prgsize) * 16384) + (PRGNum * 1024), 0x400);
		}
	}
	if ((MMCcontrol & 0x7) == 6) { //8k program at 0x8000 or 0xC000(swappable)
		unsigned short address = bankmode ? 0xC000 : 0x8000;
		unsigned short fixed = bankmode ? 0x8000 : 0xC000;
		CPU_LOG("MAPPER Switching to 8K PRG-ROM number %d at 0x%x\n", PRGNum, address);
		memcpy(&CPUMemory[address], ROMCart + (PRGNum * 8192), 0x2000);
		memcpy(&CPUMemory[fixed], ROMCart + (((prgsize * 2) - 2) * 8192), 0x2000);
	}
	if ((MMCcontrol & 0x7) == 7) { //8k program at 0xA000 
		CPU_LOG("MAPPER Switching to 8K PRG-ROM number %d at 0xA000\n", PRGNum);
		memcpy(&CPUMemory[0xA000], ROMCart + (PRGNum * 8192), 0x2000);
	}
}

void MMC1ChangePRG(unsigned char PRGNum) {
	PRGNum &= 0xF;

	if ((MMCcontrol & 0xC) <= 4) { //32kb
		CPU_LOG("MAPPER Switching to 32K PRG-ROM number %d at 0x8000\n", PRGNum);
		memcpy(&CPUMemory[0x8000], ROMCart + ((PRGNum & ~0x1) * 16384), 0x8000);
	}
	else {
		if ((MMCcontrol & 0xC) == 0x8) {
			CPU_LOG("MAPPER Switching to 16K PRG-ROM number %d at 0xC000\n", PRGNum);
			memcpy(&CPUMemory[0xC000], ROMCart + (PRGNum * 16384), 0x4000);
			memcpy(&CPUMemory[0x8000], ROMCart, 0x4000);
		}
		if ((MMCcontrol & 0xC) == 0xC) {
			CPU_LOG("MAPPER Switching to 16K PRG-ROM number %d at 0x8000\n", PRGNum);
			memcpy(&CPUMemory[0x8000], ROMCart + (PRGNum * 16384), 0x4000);
			memcpy(&CPUMemory[0xC000], ROMCart + ((prgsize - 1) * 16384), 0x4000);
		}

	}


}

void ChangeLower8KPRG(unsigned char PRGNum) {

	CPU_LOG("MAPPER Switching to 8K PRG-ROM number %d at 0x8000\n", PRGNum);
	memcpy(&CPUMemory[0x8000], ROMCart + (PRGNum * 8192), 0x2000);
}

void ChangeLowerPRG(unsigned char PRGNum) {

	CPU_LOG("MAPPER Switching to 16K PRG-ROM number %d at 0x8000\n", PRGNum);
	memcpy(&CPUMemory[0x8000], ROMCart + (PRGNum * 16384), 0x4000);
}

void ChangeUpperCHR(unsigned char PRGNum) {
	unsigned int PrgSizetotal = prgsize * 16384;

	if (mapper == 9)
	{
		PrgSizetotal = (prgsize * 2) * 8192;
	}

	if ((MMCcontrol & 0x10)) {
		CPU_LOG("MAPPER Switching Upper CHR 4K number %d at 0x1000\n", PRGNum);
		memcpy(&PPUMemory[0x1000], ROMCart + PrgSizetotal + (PRGNum * 4096), 0x1000);
	}
}

void ChangeLowerCHR(unsigned char PRGNum) {
	unsigned int PrgSizetotal = prgsize * 16384;
	if (chrsize == 0) return;
	if (mapper == 9)
	{
		PrgSizetotal = (prgsize * 2) * 8192;
	}
	if ((MMCcontrol & 0x10)) {
		CPU_LOG("MAPPER Switching Lower CHR 4k number %d at 0x0000\n", PRGNum);
		memcpy(PPUMemory, ROMCart + PrgSizetotal + (PRGNum * 4096), 0x1000);
	}
	else {
		CPU_LOG("MAPPER Switching Lower CHR 8k number %d at 0x0000\n", PRGNum);
		memcpy(PPUMemory, ROMCart + PrgSizetotal + ((PRGNum & ~0x1) * 8192), 0x2000);
	}
}

void Change8KCHR(unsigned char PRGNum) {
	unsigned char program = (PRGNum & 0x3) % chrsize;
	CPU_LOG("MAPPER Switching Lower CHR 8k number %d at 0x0000\n", program);
	memcpy(&PPUMemory[0x0000], ROMCart + (prgsize * 16384) + (program * 8192), 0x2000);
}

void MapperHandler(unsigned short address, unsigned char value) {
	CPU_LOG("MAPPER HANDLER Mapper = %d\n", mapper);
	if (mapper == 1) { //MMC1
		if (value & 0x80) {
			CPU_LOG("MAPPER MMC1 Reset shift reg %x\n", value);
			MMCbuffer = value & 0x1;
			SRwrites = 0;
			return;
		}
		MMCbuffer >>= 1;
		MMCbuffer |= (value & 0x1) << 4;
		SRwrites++;
		CPU_LOG("MAPPER Write address=%x number %d, buffer=%x value %x\n", address, SRwrites, MMCbuffer, value);
		if (SRwrites == 5) {
			if ((address & 0xe000) == 0x8000) { //Control register
				CPU_LOG("MAPPER MMC1 Write to control reg %x\n", MMCbuffer);
				MMCcontrol = MMCbuffer;
				if ((MMCcontrol & 0x3) == 2) flags6 |= 1;
				else flags6 &= ~1;
			}
			if ((address & 0xe000) == 0xA000) { //Change program rom in upper bits
				if (!(value & 0x80)) {
					CPU_LOG("MAPPER MMC1 Changing Lower CHR to Program %d\n", MMCbuffer);
					ChangeLowerCHR(MMCbuffer);
				}
			}
			if ((address & 0xe000) == 0xC000) { //Change program rom in upper bits
				if (!(value & 0x80)) {
					CPU_LOG("MAPPER MMC1 Changing upper CHR to Program %d\n", MMCbuffer);
					ChangeUpperCHR(MMCbuffer);
				}
			}
			if ((address & 0xe000) == 0xE000) { //Change program rom in upper bits
				if (!(value & 0x80)) {
					CPU_LOG("MAPPER MMC1 Changing PRG to Program %d\n", MMCbuffer);
					MMC1ChangePRG(MMCbuffer);
				}
			}
			MMCbuffer = 0;
			SRwrites = 0;
		}
	}
	if (mapper == 2) { //UNROM
		ChangeLowerPRG(value);
	}
	if (mapper == 3) { //CNROM
		Change8KCHR(value);
	}
	if (mapper == 4) { //MMC3
		MMCcontrol |= 0x10; //So we can make use of existing functions
		switch (address & 0xE001) {
		case 0x8000: //Bank Select Config
			CPU_LOG("MAPPER MMC3 Control = %x\n", value);
			MMCcontrol = value;
			break;
		case 0x8001: //Bank Data
			MMC3ChangePRG(value);
			break;
		case 0xA000: //Nametable Mirroring
			CPU_LOG("MAPPER MMC3 Mirroring set to = %x\n", ~value & 0x1);
			flags6 = ~(value & 0x1);
			break;
		case 0xA001: //PRG RAM Protect (Don't implement, maybe?)
			break;
		case 0xC000: //IRQ latch/counter value
			CPU_LOG("MAPPER MMC3 counter latch = %d\n", value);
			MMCIRQCounterLatch = value;
			break;
		case 0xC001: //IRQ Reload (Any value)
			CPU_LOG("MAPPER MMC3 IRQ Reload\n");
			MMCIRQCounter = MMCIRQCounterLatch;
			break;
		case 0xE000: //Disable IRQ (any value)
			CPU_LOG("MAPPER MMC3 Disable IRQ\n");
			MMCIRQEnable = 0;
			break;
		case 0xE001: //Enable IRQ (any value)
			CPU_LOG("MAPPER MMC3 Enable IRQ\n");
			MMCIRQEnable = 1;
			break;
		}
	}
	if (mapper == 9) { //MMC2
		MMCcontrol |= 0x10;
		switch (address & 0xE000) {
		case 0xA000: //PRG ROM Select
			CPU_LOG("MAPPER MMC2 PRG Select %d\n", value);
			ChangeLower8KPRG(value & 0xF);
			break;
		case 0xB000: //CHR ROM Lower 4K select FD
			if (MMC2Latch0 == 0xFD) {
				CPU_LOG("MAPPER MMC2 CHR Lower FD Select %d\n", value);
				ChangeLowerCHR(value & 0x1F);
			}
			break;
		case 0xC000: //CHR ROM Lower 4K select FE
			if (MMC2Latch0 == 0xFE) {
				CPU_LOG("MAPPER MMC2 CHR Lower FE Select %d\n", value);
				ChangeLowerCHR(value & 0x1F);
			}
			break;
		case 0xD000: //CHR ROM Upper 4K select FD
			if (MMC2Latch1 == 0xFD) {
				CPU_LOG("MAPPER MMC2 CHR Upper FD Select %d\n", value);
				ChangeUpperCHR(value & 0x1F);
			}
			break;
		case 0xE000: //CHR ROM Upper 4K select FD
			if (MMC2Latch1 == 0xFE) {
				CPU_LOG("MAPPER MMC2 CHR Upper FE Select %d\n", value);
				ChangeUpperCHR(value & 0x1F);
			}
			break;
		case 0xF000:
			CPU_LOG("MAPPER MMC2 Set nametable mirroring %d\n", ~value & 0x1);
			flags6 = ~(value & 0x1);
			break;
		}
	}
}
