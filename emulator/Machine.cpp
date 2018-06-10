//
// Created by nidzo on 3.6.18..
//

#include <thread>
#include <iostream>
#include <termio.h>
#include <zconf.h>
#include <cstring>
#include <signal.h>
#include "Machine.h"

Machine::Machine()
{
    registers[SP_REGISTER] = IO_SEGMENT_START+1;
    registers[PSW_REGISTER] = 1u << 14u;
    registers[PC_REGISTER] = 32;
    running=false;
}

bool Machine::setRegister(uint16_t reg, uint16_t val)
{
    if (reg > NO_OF_REGISTERS) return false;
    registers[reg] = val;
    return true;
}

bool Machine::getRegister(uint16_t reg, uint16_t &val)
{
    if (reg > NO_OF_REGISTERS) return false;
    val = registers[reg];
    return true;
}

bool Machine::step()
{
    std::lock_guard<std::recursive_mutex> lck(mtx);
    Instruction ins;
    //std::cout<<registers[PC_REGISTER];
    if (!fetch(ins)) return false;

    //std::cout<<' '<<ins.getOpcode()<<'\n';
    if(!testConditions(ins.getCondition())) return true;
    if(Machine::instructionExecutors.count(ins.getOpcode())==0) return false;
    return Machine::instructionExecutors[ins.getOpcode()](*this, ins);
}

bool Machine::run()
{
    struct termios new_tio, old_tio;
    tcgetattr(STDIN_FILENO, &old_tio);
    new_tio=old_tio;
    new_tio.c_lflag &=(~ICANON & ~ECHO);
    tcsetattr(STDIN_FILENO,TCSANOW,&new_tio);

    running=true;

    std::thread timer(Machine::periodicInterrupt, this);
    std::thread kbd(Machine::inputReader, this);
    while (registers[PSW_REGISTER] & (1u << 14u))
    {
        if (!step())
        {
            uint16_t badInstructionInterrupt;
            memory.read(4, badInstructionInterrupt);
            if(badInstructionInterrupt!=0) interrupt(2);
            else
            {
                std::cerr<<"Bad instruction hit at "<<registers[PC_REGISTER]<<"(-2 or -4), no interupt defined, halting";
                running = false;
                timer.join();
                kbd.detach();
                tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);
                return false;
            }
        }
    }
    running=false;
    timer.join();
    kbd.detach();
    tcsetattr(STDIN_FILENO,TCSANOW, &old_tio);
    return true;
}

bool Machine::fetch(Instruction &ins)
{
    uint16_t first;
    if (!memory.read(registers[PC_REGISTER], first)) return false;
    registers[PC_REGISTER] += 2;
    ins = Instruction(first);
    if (!ins.valid()) return false;
    if (ins.needSecondWord())
    {
        uint16_t second;
        if (!memory.read(registers[PC_REGISTER], second)) return false;
        registers[PC_REGISTER] += 2;
        ins.putSecondWord(second);
    }
    return true;

}

Memory &Machine::getMemory()
{
    return memory;
}

bool Machine::storeResult(Instruction::OperandType type, unsigned value,
                          uint16_t secondWord, int32_t result)
{
    int16_t realResult=(uint16_t)result;
    z(realResult==0);
    n(realResult<0);
    c(result>realResult);
    v(result>realResult);
    switch (type)
    {
        case Instruction::OperandType::REGDIR:
            if (value > NO_OF_REGISTERS) return false;
            registers[value] = (uint16_t)result;
            return true;
        case Instruction::OperandType::ABS:
            return false;
        case Instruction::OperandType::MEMDIR:
            return memWrite(secondWord, (uint16_t)result);
        case Instruction::OperandType::REGIND:
            if (value > NO_OF_REGISTERS) return false;
            return memWrite(secondWord + registers[value], (uint16_t)result);
    }
}


bool Machine::storeResult(Instruction::OperandType type, unsigned value,
                          uint16_t secondWord, int16_t result)
{
    z(result==0);
    n(result<0);
    c(false);
    v(false);
    switch (type)
    {
        case Instruction::OperandType::REGDIR:
            if (value > NO_OF_REGISTERS) return false;
            registers[value] = (uint16_t)result;
            return true;
        case Instruction::OperandType::ABS:
            return false;
        case Instruction::OperandType::MEMDIR:
            return memWrite(secondWord, (uint16_t)result);
        case Instruction::OperandType::REGIND:
            if (value > NO_OF_REGISTERS) return false;
            return memWrite(secondWord + registers[value], (uint16_t)result);
    }
}
bool Machine::fetchArgument(Instruction::OperandType type, unsigned value,
                            uint16_t secondWord, int16_t &argument)
{
    switch (type)
    {
        case Instruction::OperandType::REGIND:
            if (value > NO_OF_REGISTERS) return false;
            return memory.read(secondWord + registers[value], (uint16_t&)argument);
        case Instruction::OperandType::MEMDIR:
            return memory.read(secondWord, (uint16_t&)argument);
        case Instruction::OperandType::ABS:
            argument = secondWord;
            return true;
        case Instruction::OperandType::REGDIR:
            if (value > NO_OF_REGISTERS) return false;
            argument = registers[value];
            return true;
    }
}

std::unordered_map<unsigned, std::function<bool(Machine &,
                                                Instruction &)>> Machine::instructionExecutors = {
        {0, Machine::addExecutor},
        {1, Machine::subExecutor},
        {2, Machine::mulExecutor},
        {3, Machine::divExecutor},
        {4, Machine::cmpExecutor},
        {5, Machine::andExecutor},
        {6, Machine::orExecutor},
        {7, Machine::notExecutor},
        {8, Machine::testExecutor},
        {9, Machine::pushExecutor},
        {10, Machine::popExecutor},
        {11, Machine::callExecutor},
        {12, Machine::iretExecutor},
        {13, Machine::movExecutor},
        {14, Machine::shlExecutor},
        {15, Machine::shrExecutor},
};

bool Machine::addExecutor(Machine &machine, Instruction &instruction)
{
    int16_t arg1;
    int16_t arg2;
    if(!machine.fetchArgument(instruction.getType1(), instruction.getValue1(), instruction.getSecondWord(), arg1))
    {
        return false;
    }
    if(!machine.fetchArgument(instruction.getType2(), instruction.getValue2(), instruction.getSecondWord(), arg2))
    {
        return false;
    }
    int32_t result=(int32_t)arg1+(int32_t)arg2;
    return machine.storeResult(instruction.getType1(), instruction.getValue1(), instruction.getSecondWord(), result);
}

bool Machine::subExecutor(Machine &machine, Instruction &instruction)
{
    int16_t arg1;
    int16_t arg2;
    if(!machine.fetchArgument(instruction.getType1(), instruction.getValue1(), instruction.getSecondWord(), arg1))
    {
        return false;
    }
    if(!machine.fetchArgument(instruction.getType2(), instruction.getValue2(), instruction.getSecondWord(), arg2))
    {
        return false;
    }
    int32_t result=(int32_t)arg1-(int32_t)arg2;
    return machine.storeResult(instruction.getType1(), instruction.getValue1(), instruction.getSecondWord(), result);
}

bool Machine::mulExecutor(Machine &machine, Instruction &instruction)
{
    int16_t arg1;
    int16_t arg2;
    if(!machine.fetchArgument(instruction.getType1(), instruction.getValue1(), instruction.getSecondWord(), arg1))
    {
        return false;
    }
    if(!machine.fetchArgument(instruction.getType2(), instruction.getValue2(), instruction.getSecondWord(), arg2))
    {
        return false;
    }
    int32_t result=(int32_t)arg1*(int32_t)arg2;
    return machine.storeResult(instruction.getType1(), instruction.getValue1(), instruction.getSecondWord(), result);
}

bool Machine::divExecutor(Machine &machine, Instruction &instruction)
{
    int16_t arg1;
    int16_t arg2;
    if(!machine.fetchArgument(instruction.getType1(), instruction.getValue1(), instruction.getSecondWord(), arg1))
    {
        return false;
    }
    if(!machine.fetchArgument(instruction.getType2(), instruction.getValue2(), instruction.getSecondWord(), arg2))
    {
        return false;
    }
    int32_t result=(int32_t)arg1/(int32_t)arg2;
    return machine.storeResult(instruction.getType1(), instruction.getValue1(), instruction.getSecondWord(), result);
}

bool Machine::cmpExecutor(Machine &machine, Instruction &instruction)
{
    int16_t arg1;
    int16_t arg2;
    if(!machine.fetchArgument(instruction.getType1(), instruction.getValue1(), instruction.getSecondWord(), arg1))
    {
        return false;
    }
    if(!machine.fetchArgument(instruction.getType2(), instruction.getValue2(), instruction.getSecondWord(), arg2))
    {
        return false;
    }
    int32_t result=(int32_t)arg1-(int32_t)arg2;
    auto realResult=(int16_t) result;
    machine.z(realResult==0);
    machine.n(realResult<0);
    machine.c(result>realResult);
    machine.v(result>realResult);
    return true;
}

bool Machine::andExecutor(Machine &machine, Instruction &instruction)
{
    int16_t arg1;
    int16_t arg2;
    if(!machine.fetchArgument(instruction.getType1(), instruction.getValue1(), instruction.getSecondWord(), arg1))
    {
        return false;
    }
    if(!machine.fetchArgument(instruction.getType2(), instruction.getValue2(), instruction.getSecondWord(), arg2))
    {
        return false;
    }
    int16_t result=arg1&arg2;
    return machine.storeResult(instruction.getType1(), instruction.getValue1(), instruction.getSecondWord(), result);
}

bool Machine::orExecutor(Machine &machine, Instruction &instruction)
{
    int16_t arg1;
    int16_t arg2;
    if(!machine.fetchArgument(instruction.getType1(), instruction.getValue1(), instruction.getSecondWord(), arg1))
    {
        return false;
    }
    if(!machine.fetchArgument(instruction.getType2(), instruction.getValue2(), instruction.getSecondWord(), arg2))
    {
        return false;
    }
    int16_t result=arg1|arg2;
    return machine.storeResult(instruction.getType1(), instruction.getValue1(), instruction.getSecondWord(), result);
}

bool Machine::notExecutor(Machine &machine, Instruction &instruction)
{
    int16_t arg2;
    if(!machine.fetchArgument(instruction.getType2(), instruction.getValue2(), instruction.getSecondWord(), arg2))
    {
        return false;
    }
    int16_t result=(~arg2);
    return machine.storeResult(instruction.getType1(), instruction.getValue1(), instruction.getSecondWord(), result);
}

bool Machine::testExecutor(Machine &machine, Instruction &instruction)
{
    int16_t arg1;
    int16_t arg2;
    if(!machine.fetchArgument(instruction.getType1(), instruction.getValue1(), instruction.getSecondWord(), arg1))
    {
        return false;
    }
    if(!machine.fetchArgument(instruction.getType2(), instruction.getValue2(), instruction.getSecondWord(), arg2))
    {
        return false;
    }
    int16_t result=arg1&arg2;
    machine.n(result<0);
    machine.z(result==0);
    machine.c(false);
    machine.v(false);
    return true;
}

bool Machine::pushExecutor(Machine &machine, Instruction &instruction)
{
    int16_t arg1;
    if(!machine.fetchArgument(instruction.getType1(), instruction.getValue1(), instruction.getSecondWord(), arg1))
    {
        return false;
    }
    return machine.push((uint16_t)arg1);
}

bool Machine::popExecutor(Machine &machine, Instruction &instruction)
{
    uint16_t value;
    if(!machine.pop(value)) return false;
    return machine.storeResult(instruction.getType1(), instruction.getValue1(), instruction.getSecondWord(), (int16_t)value);
}

bool Machine::callExecutor(Machine &machine, Instruction &instruction)
{
    int16_t arg1;
    if(!machine.fetchArgument(instruction.getType1(), instruction.getValue1(), instruction.getSecondWord(), arg1))
    {
        return false;
    }
    if(!machine.push(machine.registers[PC_REGISTER])) return false;
    machine.registers[PC_REGISTER]=(uint16_t)arg1;
    return true;
}

bool Machine::iretExecutor(Machine &machine, Instruction &instruction)
{
    if(!machine.pop(machine.registers[PSW_REGISTER])) return false;
    return machine.pop(machine.registers[PC_REGISTER]);
}

bool Machine::movExecutor(Machine &machine, Instruction &instruction)
{
    int16_t arg2;
    if(!machine.fetchArgument(instruction.getType2(), instruction.getValue2(), instruction.getSecondWord(), arg2))
    {
        return false;
    }
    return machine.storeResult(instruction.getType1(), instruction.getValue1(), instruction.getSecondWord(), arg2);
}

bool Machine::shlExecutor(Machine &machine, Instruction &instruction)
{
    int16_t arg1;
    int16_t arg2;
    if(!machine.fetchArgument(instruction.getType1(), instruction.getValue1(), instruction.getSecondWord(), arg1))
    {
        return false;
    }
    if(!machine.fetchArgument(instruction.getType2(), instruction.getValue2(), instruction.getSecondWord(), arg2))
    {
        return false;
    }
    int16_t result=arg1<<arg2;
    return machine.storeResult(instruction.getType1(), instruction.getValue1(), instruction.getSecondWord(), result);
}

bool Machine::shrExecutor(Machine &machine, Instruction &instruction)
{
    int16_t arg1;
    int16_t arg2;
    if(!machine.fetchArgument(instruction.getType1(), instruction.getValue1(), instruction.getSecondWord(), arg1))
    {
        return false;
    }
    if(!machine.fetchArgument(instruction.getType2(), instruction.getValue2(), instruction.getSecondWord(), arg2))
    {
        return false;
    }
    int16_t result=arg1>>arg2;
    return machine.storeResult(instruction.getType1(), instruction.getValue1(), instruction.getSecondWord(), result);
}

bool Machine::n()
{
    return registers[PSW_REGISTER]&8;
}
bool Machine::z()
{
    return registers[PSW_REGISTER]&1;
}
bool Machine::c()
{
    return registers[PSW_REGISTER]&4;
}
bool Machine::v()
{
    return registers[PSW_REGISTER]&2;
}
void Machine::n(bool value)
{
    if(value)
    {
        registers[PSW_REGISTER]|=8;
    }
    else
    {
        registers[PSW_REGISTER]&=~8;
    }
}
void Machine::z(bool value)
{
    if(value)
    {
        registers[PSW_REGISTER]|=1;
    }
    else
    {
        registers[PSW_REGISTER]&=~1;
    }
}
void Machine::c(bool value)
{
    if(value)
    {
        registers[PSW_REGISTER]|=4;
    }
    else
    {
        registers[PSW_REGISTER]&=~4;
    }
}
void Machine::v(bool value)
{
    if(value)
    {
        registers[PSW_REGISTER]|=2;
    }
    else
    {
        registers[PSW_REGISTER]&=~2;
    }
}

bool Machine::testConditions(Instruction::Condition cnd)
{
    switch (cnd)
    {
        case Instruction::Condition::EQ:
            return z();
        case Instruction::Condition::AL:
            return true;
        case Instruction::Condition::GT:
            return !n()&&!z();
        case Instruction::Condition::NE:
            return !z();
    }
}

bool Machine::pop(uint16_t &value)
{
    if(!memory.read(registers[SP_REGISTER], value)) return false;
    registers[SP_REGISTER]+=2;
    return true;
}

bool Machine::push(uint16_t value)
{
    registers[SP_REGISTER]-=2;
    return memWrite(registers[SP_REGISTER], value);

}

void Machine::periodicInterrupt(Machine *machine)
{
    while(true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        if(!machine->running) return;

        {
            std::lock_guard<std::recursive_mutex> lck(machine->mtx);
            if(machine->registers[PSW_REGISTER]&(1<<13))
            {

                machine->interrupt(1);
            }
        }
    }
}

bool Machine::interrupt(int id)
{
    if(id>15) return false;
    if(registers[PSW_REGISTER]&(1<<15))
    {
        push(registers[PC_REGISTER]);
        push(registers[PSW_REGISTER]);
        registers[PSW_REGISTER]&=~(1<<15);
        uint16_t addr;
        memory.read(id*2, addr);
        registers[PC_REGISTER]=addr;
        return true;
    }
    return false;
}

bool Machine::memWrite(uint16_t address, uint16_t value)
{
    if(address<IO_SEGMENT_START)
    {
        return memory.write(address, value);
    }
    else
    {
        if(address==SCREEN_OUT)
        {
            char chr=value;
            if(value==16)
            {
                chr='\n';
            }
            std::cout<<chr;
            std::cout.flush();
        }
        return true;
    }
}

void Machine::inputReader(Machine *machine)
{
    while(true)
    {
        char val = getchar();
        {
            machine->mtx.lock();
            while (!machine->interrupt(3))
            {
                machine->mtx.unlock();
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                machine->mtx.lock();
            }
            machine->memory.write(KBD_IN, (uint8_t)val);
            machine->mtx.unlock();
        }
    }
}
