//
// Created by nidzo on 2.6.18..
//

#ifndef SS_MEMORY_H
#define SS_MEMORY_H


#include <vector>
#include <cstdint>

class Memory
{
public:
    Memory();
    bool write(uint16_t address, uint8_t data);
    bool write(uint16_t address, uint16_t data);

    bool read(uint16_t address, uint16_t &data);
    bool blkwrite(uint16_t start, uint16_t end, const std::vector<uint8_t> &data);
protected:
    std::vector<uint8_t> readBits;
    std::vector<uint8_t> writeBits;
    std::vector<uint8_t> executeBits;
    std::vector<uint8_t> memory;
};


#endif //SS_MEMORY_H
