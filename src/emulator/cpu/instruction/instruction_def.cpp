#include "instruction_def.h"

void InstructionRuntime::Execute(CPU* cpu)
{
    this->def->func(cpu, this);
}
