#pragma once
#ifndef MAPPERS_H
#define MAPPERS_H

void MapperHandler(unsigned short address, unsigned char value);
void MMC3IRQCountdown();
void MMC2SetLatch(unsigned char latch, unsigned char value);
void MMC2SwitchCHR();
extern bool MMC3Interrupt;
extern unsigned char MMCIRQEnable;

#endif