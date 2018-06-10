//
// Created by nidzo on 20.5.18..
//

#ifndef SS_SYMBOL_H
#define SS_SYMBOL_H

#include <string>

class Symbol
{
public:
    enum Type{LABEL, SECTION};
    Symbol(const std::string &name, Type type, const std::string &section, uint16_t offset, bool global, int seq, uint16_t length);
    Symbol(const std::string &line, bool &valid);
    Symbol();

protected:
    std::string name;
    Type type;
    std::string section;
    int seq;
    uint16_t offset;
    uint16_t length;
    bool global;
public:
    const std::string &getName() const;

    uint16_t getLength() const;

    void setLength(uint16_t length);

    const Type getType() const;

    const std::string& getSection() const;

    int getOffset() const;

    bool isGlobal() const;

    void setOffset(uint16_t offset);

    void setGlobal(bool global);

    const int getSeq() const;
};


#endif //SS_SYMBOL_H
