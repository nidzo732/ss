//
// Created by nidzo on 19.5.18..
//

#ifndef SS_LINE_H
#define SS_LINE_H

#include <string>
#include "Operand.h"

class Line
{
public:
    enum LineType{EMPTY,LABEL_ONLY, DOT_DIRECTIVE, INSTRUCTION};

    Line(const std::string &line, uint number);
    Line(const Line& line, const std::string &replacedInstruction);
    static void trim(std::string &line, std::string additional="");

protected:
    std::string line;
    std::string label;
    std::string directive;
public:
    uint getNumber() const;

protected:
    Operand arg0;
    Operand arg1;
    std::string dotArg;
    std::string instruction;
    bool valid;
    LineType type;
    uint number;
public:
    const std::string &getLine() const;

    const std::string &getDotArg() const;

    const std::string &getLabel() const;

    const std::string &getDirective() const;

    const Operand &getArg0() const;

    const Operand &getArg1() const;

    const std::string &getInstruction() const;

    bool isValid() const;

    LineType getType() const;


};


#endif //SS_LINE_H
