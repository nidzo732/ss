//
// Created by nidzo on 20.5.18..
//

#include "Symbol.h"
#include <regex>

std::regex SYMBOL_LINE(R"(^([0-9]+)\s*([A-Za-z_\.][A-Za-z0-9_]*)\s*([A-Za-z_\.][A-Za-z0-9_]*)\s*([0-9]+)\s*([0-9]+)\s*([A-Za-z]+)$)");


Symbol::Symbol(const std::string &name, Symbol::Type type, const std::string &section, uint16_t offset,
               bool global, int seq, uint16_t length)
:name(name), type(type), section(section), offset(offset), global(global), seq(seq), length(length)
{

}

const std::string &Symbol::getName() const
{
    return name;
}

const Symbol::Type Symbol::getType() const
{
    return type;
}

const std::string& Symbol::getSection() const
{
    return section;
}

int Symbol::getOffset() const
{
    return offset;
}

bool Symbol::isGlobal() const
{
    return global;
}

void Symbol::setOffset(uint16_t offset)
{
    Symbol::offset = offset;
}

void Symbol::setGlobal(bool global)
{
    Symbol::global = global;
}

const int Symbol::getSeq() const
{
    return seq;
}

Symbol::Symbol()
:name(""),seq(-1),type(Symbol::LABEL)
{

}

uint16_t Symbol::getLength() const
{
    return length;
}

void Symbol::setLength(uint16_t length)
{
    Symbol::length = length;
}

Symbol::Symbol(const std::string &line, bool &valid)
{
    std::smatch match;
    if(!std::regex_match(line, match, SYMBOL_LINE))
    {
        valid=false;
        return;
    }
    std::string seqStr=match[1];
    seq=atoi(seqStr.c_str());
    name=match[2];
    section=match[3];
    std::string offsStr=match[4];
    offset=atoi(offsStr.c_str());
    std::string lenStr=match[5];
    length=atoi(lenStr.c_str());
    global=std::string(match[6])=="GLOBAL";
    valid=true;
}