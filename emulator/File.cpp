//
// Created by nidzo on 20.5.18..
//

#include <iostream>
#include "File.h"

File::File(std::ifstream &inputStream, const std::string &fileName)
{
    valid=false;
    name=fileName;
    if(inputStream.fail())
    {
        return;
    }
    std::string line;
    if(inputStream.eof()) return;
    std::getline(inputStream, line);
    trim(line);
    if(line.find("START: ")!=0) return;
    line=line.substr(7);
    start=atoi(line.c_str());
    if(inputStream.eof()) return;
    std::getline(inputStream, line);
    trim(line);
    if(line!="SYMBOLS:") return;
    if(inputStream.eof()) return;
    std::getline(inputStream, line);
    int sectionsToResolve=0;
    length=0;
    while(true)
    {
        if(inputStream.eof()) return;
        std::getline(inputStream, line);
        trim(line);
        if(line=="RELTAB:") break;
        bool v;
        Symbol s(line, v);
        if(!v) return;
        symbols[s.getName()]=s;
        if(s.getName()==s.getSection())
        {
            if(s.getName()==".text" ||
               s.getName()==".data" ||
               s.getName()==".bss" ||
               s.getName()==".rodata")
            {
                sections[s.getName()]=s;
                if(s.getName()!=".bss") sectionsToResolve+=1;
                length+=s.getLength();
            }
            else
            {
                return;
            }
        }
    }
    if(inputStream.eof()) return;
    std::getline(inputStream, line);
    code=std::vector<uint8_t>(length);
    while(true)
    {
        if(inputStream.eof()) return;
        std::getline(inputStream, line);
        trim(line);
        if(line=="CODE:") break;
        bool v;
        RelocationEntry r(line, v);
        if(!v) return;
        relocationEntries.push_back(r);
    }
    valid=true;
    int location=0;
    int bytesToLoad=0;
    while(sectionsToResolve>0)
    {
        if(inputStream.eof()) return;
        std::getline(inputStream, line);
        trim(line);
        if(line==".data" || line==".rodata" ||
                line==".text")
        {
            if(bytesToLoad>0) return;
            if(sections.count(line)==0) return;
            location=sections[line].getOffset();
            bytesToLoad=sections[line].getLength();
        }
        else
        {
            while(!line.empty())
            {
                std::string byte;
                if(line.length()<=2)
                {
                    byte=line;
                    line="";
                }
                else
                {
                    if(line[2]!=' ') return;
                    byte=line.substr(0,2);
                    line=line.substr(3);
                }
                uint8_t val=strtol(byte.c_str(), nullptr, 16);
                bytesToLoad--;
                if(bytesToLoad<0) return;
                if(location-start>=code.size()) return;
                code[location-start]=val;
                location++;
                if(bytesToLoad==0)
                {
                    sectionsToResolve--;
                    break;
                }
            }
        }
    }
    for(auto &symbol: symbols)
    {
        if(symbol.second.getSection()=="UNKNOWN") continue;
        if(sections.count(symbol.second.getSection())==0) return;
    }
    valid=true;
}
void File::trim(std::string &line, std::string additional)
{
    while(line.length()>0 && (std::isspace(line[0]) || additional.find(line[0])!=std::string::npos)) line.erase(0,1);
    while(line.length()>0 && (std::isspace(line[line.length()-1]) || additional.find(line[line.length()-1])!=std::string::npos)) line.erase(line.length()-1,1);
}
bool File::isValid() const
{
    return valid;
}
uint16_t File::getStart() const
{
    return start;
}

uint16_t File::getLength() const
{
    return length;
}

const std::string &File::getName() const
{
    return name;
}

const std::unordered_map<std::string, Symbol> &File::getSymbols() const
{
    return symbols;
}

bool File::relocate(std::unordered_map<std::string, Symbol> &globalSymbols, int32_t fileDelta)
{
    bool ok=true;
    for(auto &entry:relocationEntries)
    {
        if(symbols.count(entry.getTargetSymbol())>0 &&
                symbols[entry.getTargetSymbol()].getSection()!="UNKNOWN")
        {
            ok&=relocate(entry, symbols[entry.getTargetSymbol()], fileDelta);
        }
        else if(globalSymbols.count(entry.getTargetSymbol())>0)
        {
            auto symbol=globalSymbols[entry.getTargetSymbol()];
            ok&=relocate(entry, symbol, fileDelta);
        }
        if(!ok) break;
    }
    return ok;
}

bool File::relocate(const RelocationEntry &entry, const Symbol &target, int32_t fileDelta)
{
    if(!sections.count(entry.getSection())) return false;
    auto &section=sections[entry.getSection()];
    if(entry.getOffset()<section.getOffset() || entry.getOffset()>=section.getOffset()+section.getLength()) return false;
    int location=entry.getOffset()+fileDelta-start;
    int length=atoi(entry.getType().substr(3).c_str());
    if (location+length>code.size()) return false;
    int32_t val=0;
    int sh=0;
    for(int i=location;i<location+length;i++)
    {
        val|=code[i]<<sh;
        sh+=8;
    }
    std::string reltype=entry.getType().substr(0,3);
    if(reltype=="ABS")
    {
        if(target.isGlobal())
        {
            val=target.getOffset();
        }
        else
        {
            val+=fileDelta;
        }
    }
    else
    {
        if(target.isGlobal())
        {
            if(target.getOffset()-(location+start+fileDelta)>INT16_MAX || target.getOffset()-(location+start+fileDelta)<INT16_MIN)
            {
                std::cerr<<"PC-relative offsets been truncated during relocation\n";
                std::cerr<<"Relocation failed\n";
                return false;
            }
            val+=target.getOffset()-(location+start+fileDelta);
        }
    }
    for(int i=location;i<location+length;i++)
    {
        code[i]=val&(255);
        val>>=8;
    }
    return true;
}

const std::vector<uint8_t> &File::getCode() const
{
    return code;
}
