//
// Created by nidzo on 20.5.18..
//

#ifndef SS_FILE_H
#define SS_FILE_H

#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include "../common/Symbol.h"
#include "../common/RelocationEntry.h"

class File
{
public:
    File(std::ifstream &inputStream, const std::string &fileName);
    static void trim(std::string &line, std::string additional="");
    bool relocate(std::unordered_map<std::string, Symbol> &globalSymbols, int32_t fileDelta);
protected:
    bool valid;
    bool relocate(const RelocationEntry &entry, const Symbol &target, int32_t fileDelta);
    std::unordered_map<std::string, Symbol> symbols;
    std::unordered_map<std::string, Symbol> sections;
    std::vector<RelocationEntry> relocationEntries;
    std::vector<uint8_t> code;
    uint16_t start;
    uint16_t length;
    std::string name;
public:
    bool isValid() const;

    uint16_t getStart() const;

    uint16_t getLength() const;

    const std::string &getName() const;

    const std::unordered_map<std::string, Symbol> &getSymbols() const;

    const std::vector<uint8_t> &getCode() const;
};


#endif //SS_FILE_H
