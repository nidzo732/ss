//
// Created by nidzo on 20.5.18..
//

#ifndef SS_FILE_H
#define SS_FILE_H

#include <fstream>
#include <string>
#include <vector>
#include "Line.h"

class File
{
public:
    File(std::ifstream &inputStream, std::string fileName);

protected:
    bool valid;
public:
    bool isValid() const;

    const std::vector<std::string> &getErrors() const;

    const std::vector<Line> &getLines() const;


protected:
    std::vector<std::string> errors;
    std::vector<Line> lines;
};


#endif //SS_FILE_H
