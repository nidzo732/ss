//
// Created by nidzo on 5.6.18..
//

#ifndef SS_INSTRUCTION_H
#define SS_INSTRUCTION_H


#include <cstdint>

class Instruction
{
public:
    enum Condition{EQ=0, NE=1, GT=2, AL=3};
    enum OperandType{ABS=0, REGDIR=1, MEMDIR=2, REGIND=3};

    Instruction();
    explicit Instruction(uint16_t firstWord);
    void putSecondWord(uint16_t secondWord);
    bool valid();
    bool needSecondWord();

    Condition getCondition() const;

    unsigned int getOpcode() const;

    OperandType getType1() const;

    OperandType getType2() const;

    unsigned int getValue1() const;

    unsigned int getValue2() const;

    uint16_t getSecondWord() const;

protected:
    Condition condition;
    unsigned opcode;
    OperandType type1;
    OperandType type2;
    unsigned value1;
    unsigned value2;
    uint16_t secondWord;
};


#endif //SS_INSTRUCTION_H
