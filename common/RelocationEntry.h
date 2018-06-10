//
// Created by nidzo on 23.5.18..
//

#ifndef SS_RELOCATIONENTRY_H
#define SS_RELOCATIONENTRY_H


#include <cstdint>
#include <string>

class RelocationEntry
{
public:
    RelocationEntry(uint16_t offset, const std::string &targetSymbol, const std::string &type, const std::string &section);
    RelocationEntry(const std::string &line, bool &valid);

    uint16_t getOffset() const;

    const std::string &getTargetSymbol() const;

    const std::string &getType() const;

    const std::string &getSection() const;

protected:
    uint16_t offset;
    std::string targetSymbol;
    std::string type;
    std::string section;
};


#endif //SS_RELOCATIONENTRY_H
