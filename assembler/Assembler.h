//
// Created by nidzo on 20.5.18..
//

#ifndef SS_ASSEMBLER_H
#define SS_ASSEMBLER_H


#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include <set>
#include "../common/Symbol.h"
#include "File.h"
#include "../common/RelocationEntry.h"
#include "../common/machine_params.h"

class Assembler
{
public:
    explicit Assembler(const File &file, uint16_t startAddress);
    bool firstPass();
    bool secondPass();

    void outputSymbolTable(std::ostream &stream);
    void outputRelocationTable(std::ostream &stream);
    void outputCode(std::ostream &stream, bool binary=false);

    const std::vector<std::string> &getErrors() const;
    const std::vector<std::string> &getWarnings() const;

protected:
    bool declareSymbol(std::string name, bool firstPass, bool global);
    bool declareSection(std::string name, bool firstPass);
    bool handleDot(const Line &line, bool firstPass);
    bool handleInstruction(const Line &line, bool firstPass);
    bool emmitArguments(const Line &line, uint16_t location);
    bool emmitArguments(const Operand &arg0, const Operand& arg1, uint16_t location);
    bool putSymbol(std::string symbol, bool relative, uint16_t location, uint16_t length);
    bool emmitValue(uint32_t value, uint16_t location, uint16_t length);
    void emmitError(const std::string &message, int line=-1);
    void emmitWarning(const std::string &message, int line=-1);
    bool getOperand(const Operand &op, uint8_t& operand);
    std::string sectionAt(uint16_t location);
    uint8_t *code;
    uint8_t *baseCode;
    uint currentLine;
    const File &file;
    const uint16_t startAddress;
    bool running;
    std::string currentSection;
    uint32_t locationCounter;
    std::unordered_map<std::string, Symbol> symbolTable;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    std::vector<RelocationEntry> relocations;

    static std::unordered_map<std::string, std::function<bool(Assembler&, const Line&, bool)> > dotHandlers;
    static std::unordered_map<std::string, uint8_t > opcodes;
    static std::set<std::string> allowedSections;
    static std::set<std::string> reservedSymbols;
    static std::unordered_map<std::string, std::function<bool(Assembler&, const Line&, bool)> > instructionHandlers;
    static bool dotCharHandler(Assembler &assembler, const Line &line, bool firstPass);
    static bool dotWordHandler(Assembler &assembler, const Line &line, bool firstPass);
    static bool dotLongHandler(Assembler &assembler, const Line &line, bool firstPass);
    static bool dotAlignHandler(Assembler &assembler, const Line &line, bool firstPass);
    static bool dotSkipHandler(Assembler &assembler, const Line &line, bool firstPass);
    static bool dotGlobalHandler(Assembler &assembler, const Line &line, bool firstPass);
    static bool dotSectionHandler(Assembler &assembler, const Line &line, bool firstPass);

    static bool binaryInstructionHandler(Assembler &assembler, const Line &line, bool firstPass);
    static bool unaryInstructionHandler(Assembler &assembler, const Line &line, bool firstPass);
    static bool noargInstructionHandler(Assembler &assembler, const Line &line, bool firstPass);
    static bool retInstructionHandler(Assembler &assembler, const Line &line, bool firstPass);
    static bool jmpInstructionHandler(Assembler &assembler, const Line &line, bool firstPass);

    static bool getInt(std::string strInt, int &value);
};


#endif //SS_ASSEMBLER_H
