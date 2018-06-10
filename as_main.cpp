#include <iostream>
#include <regex>
#include <string>
#include <fstream>
#include <unistd.h>
#include <cstdint>

#include "assembler/File.h"
#include "assembler/Assembler.h"


bool getArgs(int argc, char **argv, uint16_t &startAddress, std::string &outfile, std::string &infile)
{
    int opt;
    while((opt=getopt(argc, argv, "s:o:"))!=-1)
    {
        if(opt=='?')
        {
            std::cerr<< "Format "<<argv[0]<<" [-o OUTPUT_FILE][-s START_ADDRESS] input_file\n";
            std::cerr<<"Arguments:\n-o OUTPUT_FILE_NAME (optional, default a.o)\n-s START_ADDRESS (optional, default 0)";
            return false;
        }
        switch(opt)
        {
            case 's':
            {
                auto startAddr = atoi(optarg);
                if (startAddr < 0)
                {
                    std::cerr << "Start address can't be negative";
                    return false;
                }
                if (startAddr >= UINT16_MAX)
                {
                    std::cerr << "Start address can't be greater than "
                              << UINT16_MAX;
                    return false;
                }
                startAddress = (uint16_t) startAddr;
                break;
            }
            case 'o':
                outfile=optarg;
                break;
            default:
                break;
        }
    }
    if(argc<=optind)
    {
        std::cout <<"Please specify input file\n";
        return false;
    }
    infile=argv[optind];
    return true;
}
int main(int argc, char **argv)
{
    uint16_t startAddress=16;
    std::string outfile="a.o";
    std::string infile;
    if(!getArgs(argc, argv, startAddress, outfile, infile))
    {
        return -1;
    }
    if(startAddress<WORD_SIZE*IVT_SIZE)
    {
        std::cerr<<"Warning: program will overlap with IV table\n";
    }
    std::ifstream ifs(infile,std::ios_base::in);
    File f(ifs, infile);
    ifs.close();
    if(!f.isValid())
    {
        for (const auto &message:f.getErrors())
        {
            std::cerr<<message<<"\n";
        }
        return -1;
    }
    std::ofstream ofs(outfile, std::ios_base::out);
    Assembler as(f, startAddress);
    if(!as.firstPass() || !as.secondPass())
    {
        std::cerr<<"Assembly failed\n";
        for(const auto &error:as.getErrors())
        {
            std::cerr<<error<<"\n";
        }
        std::cerr<<as.getErrors().size()/2<<" errors\n";
        return -1;
    }
    else if(!ofs.fail())
    {
        if(!as.getWarnings().empty())
        {
            for(const auto &warning:as.getWarnings())
            {
                std::cerr<<warning<<"\n";
            }
            std::cerr<<"Warnings exist\n";
        }
        std::cerr<<"Compile successful\n";
        ofs<<"START: "<<startAddress<<"\n";
        as.outputSymbolTable(ofs);
        as.outputRelocationTable(ofs);
        as.outputCode(ofs, false);
        return 0;
    }
    else
    {
        std::cerr <<"Failed to open output file";
        return -1;
    }
}