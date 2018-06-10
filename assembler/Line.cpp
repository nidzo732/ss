//
// Created by nidzo on 19.5.18..
//

#include "Line.h"
#include <string>
#include <regex>
#include <iostream>
#include <utility>

std::regex COMMENT_LINE(R"(^#.*$)");
std::regex DOTLINE(R"(^([A-Za-z_][A-Za-z0-9_]*\s*\:\s*)?(\.[A-Za-z0-9_]+)(\s*\.?[A-Za-z0-9_\s\,\-]+)?$)");
std::regex INSLINE(R"(^([A-Za-z_][A-Za-z0-9_]*\s*\:\s*)?([A-Za-z0-9_]+)(\s*[\-A-Za-z0-9_\&\*\[\]\$]+)?(\s*\,\s*[\-A-Za-z0-9_\&\*\[\]\$]+)?$)");
std::regex LABLINE(R"(^[A-Za-z_][A-Za-z0-9_]*\s*\:\s*$)");

Line::Line(const std::string &ln, uint number)
    :line(ln), number(number)
{
    trim(line);
    std::smatch match;
    valid=false;
    if(line.length()==0 || std::regex_match(line, COMMENT_LINE))
    {
        valid=true;
        type=EMPTY;
    }
    if(std::regex_match(line, match, DOTLINE))
    {
        label=match[1];
        trim(label,":");
        directive=match[2];
        trim(directive);
        std::string arg=match[3];
        trim(arg);
        dotArg=arg;
        type=DOT_DIRECTIVE;
        valid=true;
    }
    else if(std::regex_match(line, match, INSLINE))
    {
        type=INSTRUCTION;
        label=match[1];
        trim(label,":");
        instruction=match[2];
        trim(instruction);
        std::transform(instruction.begin(), instruction.end(), instruction.begin(), [](char c){ return tolower(c);});
        std::string arg=match[3];
        trim(arg);
        arg0=Operand(arg);
        arg=match[4];
        trim(arg, ",");
        arg1=Operand(arg);
        valid=true;
    }
    else if(std::regex_match(line, match, LABLINE))
    {
        type=LABEL_ONLY;
        label=line;
        trim(label,":");
        valid=true;
    }
    valid&=(arg0.isValid() && arg1.isValid());
}

void Line::trim(std::string &line, std::string additional)
{
    while(line.length()>0 && (std::isspace(line[0]) || additional.find(line[0])!=std::string::npos)) line.erase(0,1);
    while(line.length()>0 && (std::isspace(line[line.length()-1]) || additional.find(line[line.length()-1])!=std::string::npos)) line.erase(line.length()-1,1);
}

const std::string &Line::getLine() const
{
    return line;
}

const std::string &Line::getLabel() const
{
    return label;
}

const std::string &Line::getDirective() const
{
    return directive;
}

const Operand &Line::getArg0() const
{
    return arg0;
}

const Operand &Line::getArg1() const
{
    return arg1;
}

const std::string &Line::getInstruction() const
{
    return instruction;
}

bool Line::isValid() const
{
    return valid;
}

Line::LineType Line::getType() const
{
    return type;
}

uint Line::getNumber() const
{
    return number;
}

const std::string &Line::getDotArg() const
{
    return dotArg;
}

Line::Line(const Line &line, const std::string &replacedInstruction)
:Line(line.getLine(), line.getNumber())
{
    instruction= replacedInstruction;
}
