//
// Created by nidzo on 19.5.18..
//

#ifndef SS_OPERAND_H
#define SS_OPERAND_H

#include <string>

class Operand
{
public:
    enum OperandType {NONE, ABS_VAL, SYMB_VAL, SYMB, REGISTER, REGISTER_OFFS_ABS, REGISTER_OFFS_SYM, PCREL, MEMDIR};

    explicit Operand(std::string value);
    Operand();

protected:
    std::string value;
    OperandType type;
public:
    OperandType getType() const;

    int32_t getNumericValue() const;

    uint getRegisterId() const;

    const std::string &getSymbol() const;

    bool isValid() const;

    const std::string &getValue() const;

protected:
    int32_t numericValue;
    uint registerId;
    std::string symbol;
    bool valid;
};


#endif //SS_OPERAND_H
