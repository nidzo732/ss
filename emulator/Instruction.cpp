//
// Created by nidzo on 5.6.18..
//

#include "Instruction.h"
#include "../common/machine_params.h"

Instruction::Instruction(uint16_t firstWord)
{
    unsigned tword=firstWord&255u;
    firstWord>>=8u;
    firstWord|=tword<<8u;
    condition=(Condition)((firstWord&(3u<<14u))>>14u);
    opcode=((firstWord&(15u<<10u))>>10u);
    type1=(OperandType)((firstWord&(3u<<8u))>>8u);
    value1=((firstWord&(7u<<5u))>>5u);
    if(type1==ABS && value1==7)
    {
        type1=REGDIR;
        value1=PSW_REGISTER;
    }
    type2=(OperandType)((firstWord&(3u<<3u))>>3u);
    value2=firstWord&7u;
    if(type2==ABS && value2==7)
    {
        type2=REGDIR;
        value2=PSW_REGISTER;
    }
}

bool Instruction::valid()
{
    if(type1!=REGDIR && type2!=REGDIR) return false;
    else return true;
}

bool Instruction::needSecondWord()
{
    if(type1==ABS && value1!=7 || type1==MEMDIR || type1==REGIND) return true;
    if(type2==ABS && value2!=7 || type2==MEMDIR || type2==REGIND) return true;
    return false;
}

void Instruction::putSecondWord(uint16_t secondWord)
{
    this->secondWord=secondWord;
}

Instruction::Instruction()
{

}

Instruction::Condition Instruction::getCondition() const
{
    return condition;
}

unsigned int Instruction::getOpcode() const
{
    return opcode;
}

Instruction::OperandType Instruction::getType1() const
{
    return type1;
}

Instruction::OperandType Instruction::getType2() const
{
    return type2;
}

unsigned int Instruction::getValue1() const
{
    return value1;
}

unsigned int Instruction::getValue2() const
{
    return value2;
}

uint16_t Instruction::getSecondWord() const
{
    return secondWord;
}
