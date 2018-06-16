//
// Created by nidzo on 2.6.18..
//

#include <cstring>
#include <cstdint>
#include "Memory.h"
#include "../common/machine_params.h"

Memory::Memory()
:memory(MEMORY_SIZE), readBits(MEMORY_SIZE/8), writeBits(MEMORY_SIZE/8), executeBits(MEMORY_SIZE/8)
{
}

bool Memory::write(uint16_t address, uint8_t data)
{
    memory[address]=data;
    kbdInOk=true;
    return true;
}

bool Memory::write(uint16_t address, uint16_t data)
{
    if(address==MEMORY_SIZE-1) return false;
    return write(address, (uint8_t)(data&255u)) && write(address+1, (uint8_t)((data>>8u)));
}

bool Memory::read(uint16_t address, uint16_t &data)
{
    if(address==MEMORY_SIZE-1) return false;
    data=memory[address+1];
    data<<=8u;
    data|=memory[address];
    if(address==KBD_IN) kbdInOk=true;
    return true;
}

bool Memory::blkwrite(uint16_t start, uint16_t end, const std::vector<uint8_t> &data)
{
    if(start>end) return false;
    std::copy(data.begin(), data.end(), memory.begin()+start);
    return false;
}

volatile bool Memory::isKbdInOk() const
{
    return kbdInOk;
}

volatile void Memory::setKbdInOk(bool kbdInOk)
{
    Memory::kbdInOk = kbdInOk;
}


