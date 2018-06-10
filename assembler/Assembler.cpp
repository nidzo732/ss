#include <sstream>
#include <regex>
#include <iomanip>
#include "Assembler.h"

std::regex SYMBOL_NAME(R"(^[A-Za-z_][A-Za-z_0-9]*$)");

Assembler::Assembler(const File &file, uint16_t startAddress)
        : file(file), startAddress(startAddress), code(nullptr),
          baseCode(nullptr)
{
}

bool Assembler::firstPass()
{
    symbolTable.clear();
    errors.clear();
    locationCounter = startAddress;
    currentSection = "";
    running = true;
    bool stillDoingGlobals = true;
    for (auto &line:file.getLines())
    {
        if (!running) break;
        currentLine = line.getNumber();
        if (locationCounter > MAX_PROGRAM_SIZE)
        {
            emmitError("Program too big to fit in memory");
            return false;
        }
        if (!line.getLabel().empty())
        {
            if (!declareSymbol(line.getLabel(), true, false))
            {
                return false;
            }
        }
        if (line.getType()!=Line::EMPTY && stillDoingGlobals && (line.getType() != Line::DOT_DIRECTIVE ||
                                  line.getDirective() != ".global"))
        {
            stillDoingGlobals = false;
        }
        switch (line.getType())
        {
            case Line::DOT_DIRECTIVE:
                if (!stillDoingGlobals && line.getDirective() == ".global")
                {
                    emmitError(
                            ".global declarations must appear at the start of file",
                            line.getNumber());
                    return false;
                }
                if (!handleDot(line, true)) return false;
                break;
            case Line::INSTRUCTION:
                if (!handleInstruction(line, true)) return false;
                break;
            case Line::LABEL_ONLY:
            case Line::EMPTY:
            default:
                break;
        }
    }
    return true;
}

bool Assembler::secondPass()
{
    errors.clear();
    delete[] baseCode;
    if (locationCounter - startAddress > 0)
    {
        baseCode = new uint8_t[locationCounter - startAddress];
        code = baseCode - startAddress;
    }
    locationCounter = startAddress;
    currentSection = "";
    running = true;
    for (auto &line:file.getLines())
    {
        if (!running) break;
        currentLine = line.getNumber();
        switch (line.getType())
        {
            case Line::DOT_DIRECTIVE:
                if (!handleDot(line, false)) return false;
                break;
            case Line::INSTRUCTION:
                if (!handleInstruction(line, false)) return false;
                break;
            case Line::LABEL_ONLY:
            case Line::EMPTY:
            default:
                break;
        }
    }
    if(!currentSection.empty())
    {
        auto &oldSection=symbolTable[currentSection];
        oldSection.setLength(locationCounter-oldSection.getOffset());
    }
    return true;
}

bool Assembler::declareSymbol(std::string name, bool firstPass, bool global)
{
    if (firstPass)
    {
        if (!global)
        {
            if (symbolTable.count(name) != 0)
            {
                emmitError("Redeclaration of symbol " + name);
                return false;
            }
            if (reservedSymbols.count(name) != 0)
            {
                emmitError("Cannot declare symbol with reserved name " + name);
                return false;
            }
            if (currentSection.empty())
            {
                emmitError("Cannot declare symbol " + name +
                           " outside of section");
                return false;
            }
            symbolTable.insert({name,
                                Symbol(name, Symbol::LABEL, currentSection,
                                       locationCounter, global,
                                       symbolTable.size(), 0)});
        }
    }
    else if (global)
    {
        if (symbolTable.count(name) != 0)
        {
            symbolTable[name].setGlobal(true);
        }
        else
        {
            symbolTable.insert({name,
                                Symbol(name, Symbol::LABEL, "UNKNOWN", 0, true,
                                       symbolTable.size(), 0)});
        }
    }
    return true;
}

bool Assembler::handleDot(const Line &line, bool firstPass)
{
    if (line.getDirective() == ".end")
    {
        running = false;
        return true;
    }
    if (dotHandlers.count(line.getDirective()))
    {
        return dotHandlers[line.getDirective()](*this, line, firstPass);
    }
    else
    {
        return dotHandlers[".section"](*this, line, firstPass);
    }
}

bool Assembler::handleInstruction(const Line &line, bool firstPass)
{
    if (currentSection.empty())
    {
        emmitError("Cannot put instructions outside of section",
                   line.getNumber());
        return false;
    }
    if (currentSection == ".bss")
    {
        emmitError("Cannot put instructions in bss of section",
                   line.getNumber());
        return false;
    }
    std::string instruction = line.getInstruction();
    std::string condition;
    auto location = locationCounter;
    for (auto ins:instructionHandlers)
    {
        if (ins.first + "eq" == instruction || ins.first + "ne" == instruction
            || ins.first + "gt" == instruction ||
            ins.first + "al" == instruction)
        {
            condition = instruction.substr(instruction.length() - 2);
            instruction = instruction.substr(0, instruction.length() - 2);
            break;
        }
    }
    if (instructionHandlers.count(instruction) > 0)
    {
        auto result = instructionHandlers[instruction](*this,
                                                       Line(line, instruction),
                                                       firstPass);
        if (result && (!firstPass))
        {
            uint8_t cnd = 3;
            if (!condition.empty())
            {
                if (condition == "eq") cnd = 0;
                else if (condition == "ne") cnd = 1;
                else if (condition == "gt") cnd = 2;
                else cnd = 3;
            }
            code[location] |= (cnd << 6u);
            return true;
        }
        else return result;
    }
    else
    {
        emmitError("Unknown instruction " + line.getInstruction(),
                   line.getNumber());
        return false;
    }
}

bool Assembler::dotCharHandler(Assembler &assembler, const Line &line,
                               bool firstPass)
{
    if (assembler.currentSection.empty())
    {
        assembler.emmitError("Cannot declare data outside of section",
                             line.getNumber());
        return false;
    }
    const auto &arg = line.getDotArg();
    if (std::regex_match(arg, SYMBOL_NAME))
    {
        if (!firstPass)
        {
            assembler.emmitWarning("Symbol value may be truncated");
            if (!assembler.putSymbol(arg, false, assembler.locationCounter, 1))
            {
                return false;
            }
        }
    }
    else
    {

        int value;
        if (!getInt(arg, value))
        {
            assembler.emmitError("Invalid argument for .char " + arg,
                                 line.getNumber());
            return false;
        }
        if (!firstPass)
        {
            if(value>UINT8_MAX || value<INT8_MIN) assembler.emmitWarning("Value may be truncated");
            assembler.emmitValue(value, assembler.locationCounter, 1);
        }
    }
    assembler.locationCounter += 1;
    return true;
}

bool Assembler::dotWordHandler(Assembler &assembler, const Line &line,
                               bool firstPass)
{
    if (assembler.currentSection.empty())
    {
        assembler.emmitError("Cannot declare data outside of section",
                             line.getNumber());
        return false;
    }
    const auto &arg = line.getDotArg();
    if (std::regex_match(arg, SYMBOL_NAME))
    {
        if (!firstPass)
        {
            if (!assembler.putSymbol(arg, false, assembler.locationCounter, 2))
            {
                return false;
            }
        }
    }
    else
    {
        int value;
        if (!getInt(arg, value))
        {
            assembler.emmitError("Invalid argument for .word " + arg,
                                 line.getNumber());
            return false;
        }
        if (!firstPass)
        {
            if(value>UINT16_MAX || value<INT16_MIN) assembler.emmitWarning("Value may be truncated");
            assembler.emmitValue(value, assembler.locationCounter, 2);
        }
    }
    assembler.locationCounter += 2;
    return true;
}

bool Assembler::dotLongHandler(Assembler &assembler, const Line &line,
                               bool firstPass)
{
    if (assembler.currentSection.empty())
    {
        assembler.emmitError("Cannot declare data outside of section",
                             line.getNumber());
        return false;
    }
    const auto &arg = line.getDotArg();
    if (std::regex_match(arg, SYMBOL_NAME))
    {
        if (!firstPass)
        {
            if (!assembler.putSymbol(arg, false, assembler.locationCounter, 4))
            {
                return false;
            }
        }
    }
    else
    {
        int value;
        if (!getInt(arg, value))
        {
            assembler.emmitError("Invalid argument for .long " + arg,
                                 line.getNumber());
            return false;
        }
        if (!firstPass)
        {
            assembler.emmitValue(value, assembler.locationCounter, 4);
        }
    }
    assembler.locationCounter += 4;
    return true;
}

bool Assembler::dotAlignHandler(Assembler &assembler, const Line &line,
                                bool firstPass)
{
    const auto &arg = line.getDotArg();
    int value;
    if (!getInt(arg, value))
    {
        assembler.emmitError("Invalid argument for .align " + arg,
                             line.getNumber());
        return false;
    }
    assembler.locationCounter =
            ((assembler.locationCounter + value - 1) / value) * value;
    return true;
}

bool Assembler::dotSkipHandler(Assembler &assembler, const Line &line,
                               bool firstPass)
{
    const auto &arg = line.getDotArg();
    int value;
    if (!getInt(arg, value) || value < 0)
    {
        assembler.emmitError("Invalid argument for .skip " + arg,
                             line.getNumber());
        return false;
    }
    assembler.locationCounter += value;
    return true;
}

bool Assembler::dotGlobalHandler(Assembler &assembler, const Line &line,
                                 bool firstPass)
{
    if (line.getDotArg().empty())
    {
        assembler.emmitError("Empty argument list for .global",
                             line.getNumber());
        return false;
    }
    std::string argument = "";
    for (auto chr:line.getDotArg())
    {
        if (chr == ',' || chr == ' ' || chr == '\t')
        {
            if (!argument.empty())
            {
                if (!assembler.declareSymbol(argument, firstPass, true))
                {
                    return false;
                }
            }
            argument = "";
        }
        else
        {
            argument += chr;
        }
    }
    if (!argument.empty())
        return assembler.declareSymbol(argument, firstPass, true);
    return true;
}

bool Assembler::dotSectionHandler(Assembler &assembler, const Line &line,
                                  bool firstPass)
{
    std::string section;
    if (line.getDirective() == ".section") section = line.getDotArg();
    else section = line.getDirective();
    if (allowedSections.count(section) == 0)
    {
        assembler.emmitError("Invalid section name " + section);
        return false;
    }
    return assembler.declareSection(section, firstPass);
}

bool Assembler::getInt(std::string strInt, int &value)
{
    int base = 10;
    if (strInt.find("0x") == 0 || strInt.find("0X") == 0)
    {
        strInt = strInt.substr(2);
        base = 16;
    }
    try
    {
        size_t idx;
        value = std::stoi(strInt, &idx, base);
        return idx == strInt.length();
    }
    catch (std::invalid_argument &ex)
    {
        return false;
    }
    catch (std::out_of_range &ex)
    {
        return false;
    }
}

void Assembler::emmitError(const std::string &message, int line)
{
    if (line != -1)
    {
        errors.push_back("Error on line " + std::to_string(line));
    }
    else
    {
        errors.push_back("Error on line " + std::to_string(currentLine));
    }
    errors.push_back(message);
}
void Assembler::emmitWarning(const std::string &message, int line)
{
    if (line != -1)
    {
        warnings.push_back("Warning on line " + std::to_string(line));
    }
    else
    {
        warnings.push_back("Warning on line " + std::to_string(currentLine));
    }
    warnings.push_back(message);
}

bool Assembler::declareSection(std::string name, bool firstPass)
{
    if (firstPass)
    {
        if (symbolTable.count(name) != 0)
        {
            emmitError("Redeclaration of section " + name);
            return false;
        }
        symbolTable.insert({name,
                            Symbol(name, Symbol::SECTION, name, locationCounter,
                                   false, symbolTable.size(), 0)});
    }
    if(!currentSection.empty())
    {
        auto &oldSection=symbolTable[currentSection];
        oldSection.setLength(locationCounter-oldSection.getOffset());
    }
    currentSection = name;
    return true;
}

const std::vector<std::string> &Assembler::getErrors() const
{
    return errors;
}
const std::vector<std::string> &Assembler::getWarnings() const
{
    return warnings;
}

std::string binstr(uint8_t v)
{
    std::string bin = "00000000";
    int mask = 1;
    auto iter = bin.end();
    while (iter != bin.begin())
    {
        iter--;
        if (mask & v) *iter = '1';
        mask <<= 1;
    }
    return bin;
}

void Assembler::outputSymbolTable(std::ostream &stream)
{
    stream << "SYMBOLS:\n";
    stream
            << "#         Name            Section         Offset          Length          Visibility\n";
    std::vector<std::string> symbols(symbolTable.size());
    for (auto symb:symbolTable)
    {
        symbols[symb.second.getSeq()] = symb.second.getName();
    }
    auto flags = stream.flags();
    for (auto &symbol:symbols)
    {
        stream << std::setw(10) << std::left;
        stream << symbolTable[symbol].getSeq();
        stream << std::setw(16) << std::left;
        stream << symbolTable[symbol].getName();
        stream << std::setw(16) << std::left;
        stream << symbolTable[symbol].getSection();
        stream << std::setw(16) << std::left;
        stream << symbolTable[symbol].getOffset();
        stream << std::setw(16) << std::left;
        stream << symbolTable[symbol].getLength();
        if (symbolTable[symbol].isGlobal())
        {
            stream << "GLOBAL";
        }
        else
        {
            stream << "LOCAL";
        }
        stream << "\n";
    }
    stream.flags(flags);
}

bool Assembler::binaryInstructionHandler(Assembler &assembler, const Line &line,
                                         bool firstPass)
{
    if (line.getArg0().getType() == Operand::NONE ||
        line.getArg0().getType() == Operand::NONE)
    {
        assembler.emmitError(
                "Instruction " + line.getInstruction() + " requires 2 operands",
                line.getNumber());
        return false;
    }
    if (line.getArg0().getType() != Operand::OperandType::REGISTER &&
        line.getArg1().getType() != Operand::OperandType::REGISTER)
    {
        assembler.emmitError("At least one operand must be a register",
                             line.getNumber());
        return false;
    }
    if ((line.getArg0().getType() == Operand::ABS_VAL ||
        line.getArg0().getType() == Operand::OperandType::SYMB_VAL)
            && line.getInstruction()!="cmp" && line.getInstruction()!="test")
    {
        assembler.emmitError("Left operand cannot be absolute value",
                             line.getNumber());
        return false;
    }

    int len = 2;
    if (line.getArg0().getType() != Operand::OperandType::REGISTER
        || line.getArg1().getType() != Operand::OperandType::REGISTER)
    {
        len += 2;
    }
    if (!firstPass)
    {
        assembler.code[assembler.locationCounter] =
                opcodes[line.getInstruction()] << 2;
        if (!assembler.emmitArguments(line, assembler.locationCounter))
            return false;
    }
    assembler.locationCounter += len;
    return true;
}

bool Assembler::unaryInstructionHandler(Assembler &assembler, const Line &line,
                                        bool firstPass)
{
    if (line.getArg0().getType() == Operand::NONE)
    {
        assembler.emmitError("Instruction " + line.getInstruction() +
                             " requires one argument", line.getNumber());
        return false;
    }
    if (line.getArg1().getType() != Operand::NONE)
    {
        assembler.emmitError(
                "Too many arguments for instruction " + line.getInstruction(),
                line.getNumber());
        return false;
    }
    auto instruction = line.getInstruction();
    std::transform(instruction.begin(), instruction.end(), instruction.begin(),
                   [](char c) { return tolower(c); });
    if (instruction == "pop" &&
        (line.getArg0().getType() == Operand::OperandType::SYMB_VAL ||
         line.getArg0().getType() == Operand::OperandType::ABS_VAL))
    {
        assembler.emmitError("POP cannot work work with absolute operands");
        return false;
    }
    int len = 2;
    if (line.getArg0().getType() != Operand::REGISTER)
    {
        len += 2;
    }
    if (!firstPass)
    {
        assembler.code[assembler.locationCounter] =
                opcodes[line.getInstruction()] << 2;
        if (!assembler.emmitArguments(line, assembler.locationCounter))
            return false;
    }
    assembler.locationCounter += len;
    return true;
}

bool Assembler::noargInstructionHandler(Assembler &assembler, const Line &line,
                                        bool firstPass)
{
    if (line.getArg0().getType() != Operand::NONE)
    {
        assembler.emmitError(
                "Instruction " + line.getInstruction() + " has no arguments",
                line.getNumber());
        return false;
    }
    if (!firstPass)
    {
        assembler.code[assembler.locationCounter] =
                opcodes[line.getInstruction()] << 2;
        assembler.code[assembler.locationCounter + 1] = 0xe7;
    }
    assembler.locationCounter += 2;
    return true;
}

bool Assembler::emmitArguments(const Operand &arg0, const Operand &arg1,
                               uint16_t location)
{
    uint8_t firstByte = code[location];
    uint8_t secondByte = 0;//code[location+1];
    uint8_t firstOperand;
    uint8_t secondOperand;
    if (!getOperand(arg0, firstOperand)) return false;
    if (!getOperand(arg1, secondOperand)) return false;
    firstByte |= (firstOperand >> 3u);
    secondByte |= (firstOperand & 7) << 5u;
    secondByte |= secondOperand;
    code[location] |= firstByte;
    code[location + 1] = secondByte;
    auto type1 = arg0.getType();
    switch (type1)
    {
        case Operand::OperandType::ABS_VAL:
        case Operand::OperandType::MEMDIR:
        case Operand::OperandType::REGISTER_OFFS_ABS:
            if(arg0.getNumericValue()>INT16_MAX || arg0.getNumericValue()<INT16_MIN) emmitWarning("The value may be truncated");
            emmitValue(arg0.getNumericValue(), location + 2, 2);
            break;
        case Operand::OperandType::SYMB_VAL:
        case Operand::OperandType::SYMB:
        case Operand::OperandType::REGISTER_OFFS_SYM:
            if (!putSymbol(arg0.getSymbol(), false, location + 2, 2))
                return false;
            break;
        case Operand::OperandType::PCREL:
            if (!putSymbol(arg0.getSymbol(), true, location + 2, 2))
                return false;
            break;
        case Operand::NONE:
            break;
        case Operand::REGISTER:
            break;
    }
    auto type2 = arg1.getType();
    switch (type2)
    {
        case Operand::OperandType::ABS_VAL:
        case Operand::OperandType::MEMDIR:
        case Operand::OperandType::REGISTER_OFFS_ABS:
            if(arg1.getNumericValue()>INT16_MAX || arg1.getNumericValue()<INT16_MIN) emmitWarning("The value may be truncated");
            emmitValue(arg1.getNumericValue(), location + 2, 2);
            break;
        case Operand::OperandType::SYMB_VAL:
        case Operand::OperandType::SYMB:
        case Operand::OperandType::REGISTER_OFFS_SYM:
            if (!putSymbol(arg1.getSymbol(), false, location + 2, 2))
                return false;
            break;
        case Operand::OperandType::PCREL:
            if (!putSymbol(arg1.getSymbol(), true, location + 2, 2))
                return false;
            break;
        case Operand::NONE:
            break;
        case Operand::REGISTER:
            break;
    }
    return true;
}

bool Assembler::emmitArguments(const Line &line, uint16_t location)
{
    return emmitArguments(line.getArg0(), line.getArg1(), location);
}

bool Assembler::retInstructionHandler(Assembler &assembler, const Line &line,
                                      bool firstPass)
{
    if (line.getArg0().getType() != Operand::NONE)
    {
        assembler.emmitError(
                "Instruction " + line.getInstruction() + " has no arguments",
                line.getNumber());
        return false;
    }
    return unaryInstructionHandler(assembler, Line("pop pc", line.getNumber()),
                                    firstPass);
}

bool Assembler::jmpInstructionHandler(Assembler &assembler, const Line &line,
                                      bool firstPass)
{
    if (line.getArg0().getType() == Operand::NONE)
    {
        assembler.emmitError("Instruction " + line.getInstruction() +
                             " requires one argument", line.getNumber());
        return false;
    }
    if (line.getArg1().getType() != Operand::NONE)
    {
        assembler.emmitError(
                "Too many arguments for instruction " + line.getInstruction(),
                line.getNumber());
        return false;
    }
    if (line.getArg0().getType() == Operand::OperandType::PCREL)
    {
        if (!firstPass)
        {
            assembler.code[assembler.locationCounter] = opcodes["add"] << 2;
            assembler.code[assembler.locationCounter] |= 1;
            assembler.code[assembler.locationCounter + 1] = 0xe0;
            if (!assembler.putSymbol(line.getArg0().getSymbol(), true,
                                     assembler.locationCounter + 2, 2))
            {
                return false;
            }
        }
        assembler.locationCounter += 4;
        return true;
    }
    else
    {
        return binaryInstructionHandler(assembler, Line("mov pc, " +
                                                        line.getArg0().getValue(),
                                                        line.getNumber()),
                                        firstPass);
    }
}

bool Assembler::putSymbol(std::string symbol, bool relative, uint16_t location,
                          uint16_t length)
{
    auto &symb = symbolTable[symbol];
    uint32_t insertedValue;
    std::string relType = "";
    std::string targetSymbol = symbol;
    if (symb.isGlobal())
    {
        if (relative)
        {
            relType = "REL" + std::to_string(length);
            insertedValue = -2;
        }
        else
        {
            relType = "ABS" + std::to_string(length);
            insertedValue = 0;
        }
    }
    else
    {
        targetSymbol = symb.getSection();
        if (relative)
        {
            relType = "REL" + std::to_string(length);
            if(symb.getOffset()-location-2<INT16_MIN || symb.getOffset()-location-2>INT16_MAX)
            {
                emmitError("Could not fit offset into 16 bits");
                return false;
            }
            insertedValue = symb.getOffset() - location - 2;
        }
        else
        {
            insertedValue = symb.getOffset();
            relType = "ABS" + std::to_string(length);
        }
    }
    if (!(relative && symb.getSection() == currentSection) || symb.isGlobal())
    {
        relocations.push_back(
                RelocationEntry(location, targetSymbol, relType, currentSection));
    }
    emmitValue(insertedValue, location, length);
    return true;
}

bool Assembler::emmitValue(uint32_t value, uint16_t location, uint16_t length)
{
    uint8_t byte;
    while (length > 0)
    {
        byte = value & 255;
        code[location] = byte;
        length--;
        location++;
        value >>= 8;
    }
    return true;
}

void Assembler::outputCode(std::ostream &stream, bool binary)
{
    stream << "CODE:";
    std::string csec;
    auto flags = stream.flags();
    stream << std::hex;
    stream << std::setw(2);
    stream << std::setfill('0');
    int oc = 0;
    for (uint16_t i = startAddress; i < locationCounter; i++, oc++)
    {
        auto sec = sectionAt(i);
        if (!sec.empty())
        {
            csec = sec;
            if (csec != ".bss") stream << "\n" + csec + "\n";
            oc = 0;
        }
        if (csec == ".bss") continue;
        if (!binary)
        {
            if (oc != 0 && oc % 16 == 0) stream << "\n";
            int value = (int) (code[i]);
            if (value == 0)
            {
                stream << "00 ";
            }
            else
            {
                stream << std::hex;
                stream << std::setw(2);
                stream << (int) (code[i]) << ' ';
            }
        }
        else
        {
            if (i != 0 && i % 4 == 0) stream << "\n";
            stream << binstr(code[i]) << " ";
        }
    }
    stream.flags(flags);
}

bool Assembler::getOperand(const Operand &op, uint8_t &operand)
{
    operand = 0;
    switch (op.getType())
    {
        case Operand::OperandType::REGISTER_OFFS_SYM:
        case Operand::REGISTER_OFFS_ABS:
        case Operand::OperandType::REGISTER:
        case Operand::OperandType::PCREL:
        {
            if (op.getRegisterId() > NO_OF_REGISTERS)
            {
                emmitError("Register " + std::to_string(op.getRegisterId()) +
                           " does not exist");
                return false;
            }
            if (op.getRegisterId() == PSW_REGISTER)
            {
                operand = 7;
            }
            else if (op.getType() == Operand::OperandType::REGISTER)
            {
                operand = (1 << 3) | op.getRegisterId();
            }
            else if (op.getType() == Operand::OperandType::PCREL)
            {
                operand = (3 << 3) | PC_REGISTER;
            }
            else
            {
                operand = (3 << 3) | op.getRegisterId();
            }
            break;
        }
        case Operand::OperandType::ABS_VAL:
        case Operand::OperandType::SYMB_VAL:
            operand = 0;
            break;
        case Operand::SYMB:
        case Operand::OperandType::MEMDIR:
            operand = 2 << 3;
            break;
        case Operand::NONE:
            operand=7;
            break;
    }
    return true;
}

std::unordered_map<std::string, std::function<bool(Assembler &, const Line &,
                                                   bool)> >
        Assembler::dotHandlers = {{".char",    Assembler::dotCharHandler},
                                  {".word",    Assembler::dotWordHandler},
                                  {".long",    Assembler::dotLongHandler},
                                  {".align",   Assembler::dotAlignHandler},
                                  {".skip",    Assembler::dotSkipHandler},
                                  {".global",  Assembler::dotGlobalHandler},
                                  {".section", Assembler::dotSectionHandler}};

std::set<std::string> Assembler::allowedSections = {".text", ".data", ".rodata",
                                                    ".bss"};

std::set<std::string> Assembler::reservedSymbols = {"PC", "PSW", "SP",
                                                    "pc", "psw", "sp",
                                                    "r0", "r1", "r2",
                                                    "r3", "r4", "r5",
                                                    "r6", "r7",
                                                    "R0", "R1", "R2",
                                                    "R3", "R4", "R5",
                                                    "R6", "R7"};

std::unordered_map<std::string, std::function<bool(Assembler &, const Line &,
                                                   bool)> >
        Assembler::instructionHandlers = {{"add",  Assembler::binaryInstructionHandler},
                                          {"sub",  Assembler::binaryInstructionHandler},
                                          {"mul",  Assembler::binaryInstructionHandler},
                                          {"div",  Assembler::binaryInstructionHandler},
                                          {"cmp",  Assembler::binaryInstructionHandler},
                                          {"and",  Assembler::binaryInstructionHandler},
                                          {"or",   Assembler::binaryInstructionHandler},
                                          {"not",  Assembler::binaryInstructionHandler},
                                          {"test", Assembler::binaryInstructionHandler},
                                          {"mov",  Assembler::binaryInstructionHandler},
                                          {"shl",  Assembler::binaryInstructionHandler},
                                          {"shr",  Assembler::binaryInstructionHandler},
                                          {"push", Assembler::unaryInstructionHandler},
                                          {"pop",  Assembler::unaryInstructionHandler},
                                          {"call", Assembler::unaryInstructionHandler},
                                          {"iret", Assembler::noargInstructionHandler},
                                          {"ret",  Assembler::retInstructionHandler},
                                          {"jmp",  Assembler::jmpInstructionHandler}};

std::unordered_map<std::string, uint8_t> Assembler::opcodes = {{"add",  0},
                                                               {"sub",  1},
                                                               {"mul",  2},
                                                               {"div",  3},
                                                               {"cmp",  4},
                                                               {"and",  5},
                                                               {"or",   6},
                                                               {"not",  7},
                                                               {"test", 8},
                                                               {"push", 9},
                                                               {"pop",  10},
                                                               {"call", 11},
                                                               {"iret", 12},
                                                               {"mov",  13},
                                                               {"shl",  14},
                                                               {"shr",  15}};

std::string Assembler::sectionAt(uint16_t location)
{
    for (auto iter: symbolTable)
    {
        if (iter.second.getOffset() == location &&
            iter.second.getType() == Symbol::Type::SECTION)
        {
            return iter.second.getName();
        }
    }
    return "";
}

void Assembler::outputRelocationTable(std::ostream &stream)
{
    stream << "RELTAB:\n";
    stream
            << "Name            Section         Offset          TYPE      \n";
    std::vector<std::string> symbols(symbolTable.size());
    for (auto symb:symbolTable)
    {
        symbols[symb.second.getSeq()] = symb.second.getName();
    }
    auto flags = stream.flags();
    for (auto &rel:relocations)
    {
        stream << std::setw(16) << std::left;
        stream << rel.getTargetSymbol();
        stream << std::setw(16) << std::left;
        stream << rel.getSection();
        stream << std::setw(16) << std::left;
        stream << rel.getOffset();
        stream << std::setw(10) << std::left;
        stream << rel.getType();
        stream << "\n";
    }
    stream.flags(flags);
}


