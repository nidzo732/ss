//
// Created by nidzo on 23.5.18..
//

#include <regex>
#include "RelocationEntry.h"

std::regex LINE_REGEX(R"(^([A-Za-z_\.][A-Za-z0-9_]*)\s*([A-Za-z_\.][A-Za-z0-9_]*)\s*([0-9]+)\s*([A-Za-z_\.][A-Za-z0-9_]*)$)");
uint16_t RelocationEntry::getOffset() const
{
    return offset;
}

const std::string &RelocationEntry::getTargetSymbol() const
{
    return targetSymbol;
}

const std::string &RelocationEntry::getType() const
{
    return type;
}

const std::string &RelocationEntry::getSection() const
{
    return section;
}

RelocationEntry::RelocationEntry(uint16_t offset,
                                 const std::string &targetSymbol,
                                 const std::string &type, const std::string &section)
:offset(offset), targetSymbol(targetSymbol), type(type), section(section)
{

}

RelocationEntry::RelocationEntry(const std::string &line, bool &valid)
{
    std::smatch match;
    if(!std::regex_match(line, match, LINE_REGEX))
    {
        valid=false;
        return;
    }
    targetSymbol=match[1];
    section=match[2];
    std::string offsStr=match[3];
    offset=atoi(offsStr.c_str());
    type=match[4];
    valid = type == "ABS1" || type == "ABS2" || type == "ABS4" || type == "REL2";
}
