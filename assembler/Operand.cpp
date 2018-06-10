//
// Created by nidzo on 19.5.18..
//

#include "Operand.h"
#include "Line.h"
#include "../common/machine_params.h"
#include <regex>

std::regex ABS_VALUE(R"(^-?[0-9]+$)");
std::regex SYMB_VALUE(R"(^\&[A-Za-z_][A-Za-z0-9_]*$)");
std::regex SYMBR(R"(^[A-Za-z_][A-Za-z0-9_]*$)");
//std::regex ABS_VALUE(R"(^[0-9]+$)");
std::regex REG_DIRECT(R"(^[Rr]([0-9]+)$)");
std::regex NAMED_REGISTER(R"(^(?:[Pp][Cc])|(?:[Ss][Pp])|(?:[Pp][Ss][Ww])$)");
std::regex REG_OFFSET_ABS(R"(^[Rr]([0-9]+)\[(-?[0-9]+)\]$)");
std::regex REG_OFFSET_SYM(R"(^[Rr]([0-9]+)\[([A-Za-z_][A-Za-z0-9_]*)\]$)");
std::regex NAMED_REG_OFFSET_ABS(R"(^((?:[Pp][Cc])|(?:[Ss][Pp]))\[(-?[0-9]+)\]$)");
std::regex NAMED_REG_OFFSET_SYM(R"(^((?:[Pp][Cc])|(?:[Ss][Pp]))\[([A-Za-z_][A-Za-z0-9_]*)\]$)");
std::regex PCREL_SYM(R"(^\$[A-Za-z_][A-Za-z0-9_]*$)");
std::regex MEMDIR_VALUE(R"(^\*[0-9]+*$)");

Operand::Operand(std::string value)
:value(value)
{
    std::smatch match;
    valid=true;
    if(value.length()==0)
    {
        type=NONE;
    }
    else if(std::regex_match(value, match, MEMDIR_VALUE))
    {
        type=MEMDIR;
        numericValue=(atoi(value.substr(1).c_str()));
    }
    else if(std::regex_match(value, match, ABS_VALUE))
    {
        type=ABS_VAL;
        numericValue=atoi(value.c_str());
    }
    else if(std::regex_match(value, match, SYMB_VALUE))
    {
        type=SYMB_VAL;
        symbol=value.substr(1);
    }
    else if(std::regex_match(value, match, PCREL_SYM))
    {
        type=PCREL;
        symbol=value.substr(1);
        registerId=PC_REGISTER;
    }
    else if(std::regex_match(value, match, REG_DIRECT))
    {
        type=REGISTER;
        std::string regnum=match[1];
        registerId=(uint)atoi(regnum.c_str());
    }
    else if(std::regex_match(value, match, NAMED_REGISTER))
    {
        type=REGISTER;
        std::string reg=value;
        std::transform(reg.begin(), reg.end(), reg.begin(), [](char c){return tolower(c);});
        if(reg=="pc")
        {
            registerId=PC_REGISTER;
        }
        else if(reg=="sp")
        {
            registerId=SP_REGISTER;
        }
        else
        {
            registerId=PSW_REGISTER;
        }
    }
    else if(std::regex_match(value, match, SYMBR))
    {
        type=SYMB;
        symbol=value;
    }
    else if(std::regex_match(value, match, REG_OFFSET_ABS))
    {
        type=REGISTER_OFFS_ABS;
        std::string regnum=match[1];
        registerId=atoi(regnum.c_str());
        std::string numval=match[2];
        numericValue=atoi(numval.c_str());
    }
    else if(std::regex_match(value, match, NAMED_REG_OFFSET_ABS))
    {
        type=REGISTER_OFFS_ABS;
        std::string reg=match[1];
        std::transform(reg.begin(), reg.end(), reg.begin(), [](char c){return tolower(c);});
        if(reg=="pc")
        {
            registerId=PC_REGISTER;
        }
        else if(reg=="sp")
        {
            registerId=SP_REGISTER;
        }
        std::string numval=match[2];
        numericValue=atoi(numval.c_str());
    }
    else if(std::regex_match(value, match, REG_OFFSET_SYM))
    {
        type=REGISTER_OFFS_SYM;
        std::string regnum=match[1];
        registerId=atoi(regnum.c_str());
        symbol=match[2];
        Line::trim(symbol);
    }
    else if(std::regex_match(value, match, NAMED_REG_OFFSET_SYM))
    {
        type=REGISTER_OFFS_SYM;
        std::string reg=match[1];
        std::transform(reg.begin(), reg.end(), reg.begin(), [](char c){return tolower(c);});
        if(reg=="pc")
        {
            registerId=PC_REGISTER;
        }
        else if(reg=="sp")
        {
            registerId=SP_REGISTER;
        }
        symbol=match[2];
        Line::trim(symbol);
    }
    else
    {
        valid=false;
    }
}

Operand::Operand()
:value(""), type(NONE), valid(true)
{

}

Operand::OperandType Operand::getType() const
{
    return type;
}

int32_t Operand::getNumericValue() const
{
    return numericValue;
}

uint Operand::getRegisterId() const
{
    return registerId;
}

const std::string &Operand::getSymbol() const
{
    return symbol;
}

bool Operand::isValid() const
{
    return valid;
}

const std::string &Operand::getValue() const
{
    return value;
}
