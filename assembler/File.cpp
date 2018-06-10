//
// Created by nidzo on 20.5.18..
//

#include "File.h"
#include "../emulator/File.h"

File::File(std::ifstream &inputStream, std::string fileName)
{
    if(inputStream.fail())
    {
        valid=false;
        errors.push_back("Failed to open file "+fileName);
        return;
    }
    valid=true;
    uint lineNumber=0;
    std::string line;
    while(!inputStream.eof())
    {
        std::getline(inputStream, line);
        lineNumber++;
        Line l(line, lineNumber);
        if(!Line(line, lineNumber).isValid())
        {
            errors.push_back("Syntax error on line "+std::to_string(lineNumber)+"\n"+line);
            valid=false;
        }
        lines.push_back(l);
        if(l.getDirective()==".end") break;
    }
}

bool File::isValid() const
{
    return valid;
}

const std::vector<std::string> &File::getErrors() const
{
    return errors;
}

const std::vector<Line> &File::getLines() const
{
    return lines;
}
