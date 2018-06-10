//
// Created by nidzo on 2.6.18..
//

#include <cstdlib>
#include <algorithm>
#include <iostream>
#include "emulator/Memory.h"
#include "emulator/Machine.h"
#include "common/Symbol.h"
#include "common/RelocationEntry.h"
#include "emulator/File.h"

#define CNT 104857600
uint16_t v[CNT];
uint16_t a[CNT];
int main(int argc, char **argv)
{
    std::vector<File> files;
    if(argc<2)
    {
        std::cerr<<"No input files given\n";
        return -1;
    }
    for(int i=1;i<argc;i++)
    {
        std::ifstream ifs(argv[i]);
        File f(ifs, argv[i]);
        ifs.close();
        if(!f.isValid())
        {
            std::cerr<<"File "<<argv[i]<<" is invalid2\n";
            return -1;
        }
        files.push_back(f);
    }
    std::sort(files.begin(), files.end(), [](const File &x1, const File &x2)->bool{return x1.getStart()<x2.getStart();});
    std::string prevName="";
    uint16_t prevEnd=0;
    for(auto& file:files)
    {
        if(file.getStart()<prevEnd)
        {
            std::cerr<<"Files "<<prevName<<" and "<<file.getName()<<" overlap \n";
            return -1;
        }
        prevName=file.getName();
        prevEnd=file.getStart()+file.getLength();
    }
    std::unordered_map<std::string, Symbol> globalSymbols;
    for(auto &file: files)
    {
        for(auto &symbolPair:file.getSymbols())
        {
            if(symbolPair.second.isGlobal() &&
                    symbolPair.second.getSection()!="UNKNOWN")
            {
                if(globalSymbols.count(symbolPair.first))
                {
                    std::cerr<<"Duplicate definition of "<<symbolPair.first<<"\n";
                    return -1;
                }
                globalSymbols[symbolPair.first]=symbolPair.second;
            }
        }
    }
    for(auto &file: files)
    {
        for(auto &symbolPair:file.getSymbols())
        {
            if(symbolPair.second.isGlobal() &&
               symbolPair.second.getSection()=="UNKNOWN" &&
               globalSymbols.count(symbolPair.first)==0)
            {
                std::cerr<<"Unresolved symbol "<<symbolPair.first;
                return -1;
            }
        }
    }
    if(globalSymbols.count("START")==0)
    {
        std::cerr<<"Missing symbol START\n";
        return -1;
    }
    for(auto &file: files)
    {
        if(!file.relocate(globalSymbols, 0))
        {
            std::cerr<<"File "<<file.getName()<<" is invalid1\n";
            return -1;
        };
    }
    Machine m;
    for(auto &file: files)
    {
        m.getMemory().blkwrite(file.getStart(), file.getStart()+file.getLength(), file.getCode());
    }
    m.setRegister(PC_REGISTER, globalSymbols["START"].getOffset());
    auto result = m.run();
    std::cout<<"\n";
    if(result)
    {
        std::cout<<"Emulator exited correctly\n";
        return 0;
    }
    else
    {
        std::cout<<"Emulator exited incorrectly\n";
        return -1;
    }
}