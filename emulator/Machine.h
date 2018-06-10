//
// Created by nidzo on 3.6.18..
//

#ifndef SS_MACHINE_H
#define SS_MACHINE_H


#include <mutex>
#include <cstdint>
#include <unordered_map>
#include <semaphore.h>
#include "Memory.h"
#include "../common/machine_params.h"
#include "Instruction.h"

class Machine
{
public:
    Machine();
    bool setRegister(uint16_t reg, uint16_t val);
    bool getRegister(uint16_t reg, uint16_t &val);
    bool step();
    bool run();

    Memory &getMemory();

protected:
    Memory memory;
    uint16_t registers[NO_OF_REGISTERS+1];
    std::recursive_mutex mtx;
    bool fetch(Instruction&);

    bool storeResult(Instruction::OperandType type, unsigned value, uint16_t secondWord, int32_t result);
    bool storeResult(Instruction::OperandType type, unsigned value, uint16_t secondWord, int16_t result);
    bool fetchArgument(Instruction::OperandType type, unsigned value, uint16_t secondWord, int16_t &argument, bool useEffectiveAddress=false);
    bool interrupt(int id);
    bool memWrite(uint16_t address, uint16_t value);
    volatile bool running;
    bool interruptSignals[16];
    bool notifyInterrupt(int id);
    void handleInterrupts();

    bool pop(uint16_t& value);
    bool push(uint16_t value);
    static void inputReader(Machine *machine);
    static void periodicInterrupt(Machine *machine);

    bool n();
    bool z();
    bool c();
    bool v();

    void n(bool);
    void z(bool);
    void c(bool);
    void v(bool);

    bool testConditions(Instruction::Condition cnd);

    static std::unordered_map<unsigned, std::function<bool(Machine&, Instruction&)> > instructionExecutors;
    static bool addExecutor(Machine &machine, Instruction &instruction);
    static bool subExecutor(Machine &machine, Instruction &instruction);
    static bool mulExecutor(Machine &machine, Instruction &instruction);
    static bool divExecutor(Machine &machine, Instruction &instruction);
    static bool cmpExecutor(Machine &machine, Instruction &instruction);
    static bool andExecutor(Machine &machine, Instruction &instruction);
    static bool orExecutor(Machine &machine, Instruction &instruction);
    static bool notExecutor(Machine &machine, Instruction &instruction);
    static bool testExecutor(Machine &machine, Instruction &instruction);
    static bool pushExecutor(Machine &machine, Instruction &instruction);
    static bool popExecutor(Machine &machine, Instruction &instruction);
    static bool callExecutor(Machine &machine, Instruction &instruction);
    static bool iretExecutor(Machine &machine, Instruction &instruction);
    static bool movExecutor(Machine &machine, Instruction &instruction);
    static bool shlExecutor(Machine &machine, Instruction &instruction);
    static bool shrExecutor(Machine &machine, Instruction &instruction);

};


#endif //SS_MACHINE_H
