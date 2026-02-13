#include "instruction_set.h"

#include <future>

void nop(Cpu* cpu, InstructionRuntime* instr)
{

}

void di(Cpu* cpu, InstructionRuntime* instr)
{
   cpu->ime = false;
}

void ei(Cpu* cpu, InstructionRuntime* instr)
{
   cpu->ime = true;
}

// Flow Control
void jp_a16(Cpu* cpu, InstructionRuntime* instr)
{
   cpu->reg.PC = instr->imm.u16;
}

void jp_r16(Cpu* cpu, InstructionRuntime* instr)
{
   uint16_t* reg = cpu->reg.Reg16(instr->def->op1);
   cpu->reg.PC = *reg;
}

void jp_z_a16(Cpu* cpu, InstructionRuntime* instr)
{
   if (cpu->reg.GetFlag(FlagType::FLAG_Z))
   {
      cpu->reg.PC = instr->imm.u16;
      instr->cycles = instr->def->alt_cycles;
   }
}

void jp_nz_a16(Cpu* cpu, InstructionRuntime* instr)
{
   if (!cpu->reg.GetFlag(FlagType::FLAG_Z))
   {
      cpu->reg.PC = instr->imm.u16;
      instr->cycles = instr->def->alt_cycles;
   }
}

void jr_s8(Cpu* cpu, InstructionRuntime* instr)
{
   const int8_t disp = instr->imm.s8;

   cpu->reg.PC += disp;
}

void jr_z_s8(Cpu* cpu, InstructionRuntime* instr)
{
   const int8_t disp = instr->imm.s8;
   if (cpu->reg.GetFlag(FlagType::FLAG_Z))
   {
      cpu->reg.PC += disp;
      instr->cycles = instr->def->alt_cycles;
   }
}

void jr_nz_s8(Cpu* cpu, InstructionRuntime* instr)
{
   const int8_t disp = instr->imm.s8;
   if (!cpu->reg.GetFlag(FlagType::FLAG_Z))
   {
      cpu->reg.PC += disp;
      instr->cycles = instr->def->alt_cycles;
   }
}

void jr_c_s8(Cpu* cpu, InstructionRuntime* instr)
{
   const int8_t disp = instr->imm.s8;
   if (cpu->reg.GetFlag(FlagType::FLAG_C))
   {
      cpu->reg.PC += disp;
      instr->cycles = instr->def->alt_cycles;
   }
}


void call_a16(Cpu* cpu, InstructionRuntime* instr)
{
   cpu->Push16(cpu->reg.PC);
   cpu->reg.PC = instr->imm.u16;
}

void ret(Cpu* cpu, InstructionRuntime* instr)
{
   cpu->reg.PC = cpu->Pop16();
}

void reti(Cpu* cpu, InstructionRuntime* instr)
{
   cpu->reg.PC = cpu->Pop16();
   cpu->ime = true;
}

void ret_nc(Cpu* cpu, InstructionRuntime* instr)
{
   if (!cpu->reg.GetFlag(FlagType::FLAG_C))
   {
      cpu->reg.PC = cpu->Pop16();
      instr->cycles = instr->def->alt_cycles;
   }
}

void ret_z(Cpu* cpu, InstructionRuntime* instr)
{
   if (cpu->reg.GetFlag(FlagType::FLAG_Z))
   {
      cpu->reg.PC = cpu->Pop16();
      instr->cycles = instr->def->alt_cycles;
   }
}


void ret_nz(Cpu* cpu, InstructionRuntime* instr)
{
   if (!cpu->reg.GetFlag(FlagType::FLAG_Z))
   {
      cpu->reg.PC = cpu->Pop16();
      instr->cycles = instr->def->alt_cycles;
   }
}

void rst(Cpu* cpu, InstructionRuntime* instr)
{
   cpu->Push16(cpu->reg.PC);
   cpu->reg.PC = instr->def->param1;
}

// Stack

void push_r16(Cpu* cpu, InstructionRuntime* instr)
{
   uint16_t* reg = cpu->reg.Reg16(instr->def->op1);
   cpu->Push16(*reg);
}

void pop_r16(Cpu* cpu, InstructionRuntime* instr)
{
   uint16_t* reg = cpu->reg.Reg16(instr->def->op1);
   *reg = cpu->Pop16();
}

// Load
void ld_r8_r8(Cpu* cpu, InstructionRuntime* instr)
{
   uint8_t* reg1 = cpu->reg.Reg8(instr->def->op1);
   uint8_t* reg2 = cpu->reg.Reg8(instr->def->op2);

   *reg1 = *reg2;

}

void ld_r16_d16(Cpu* cpu, InstructionRuntime* instr)
{
   cpu->reg.Write16(instr->def->op1, instr->imm.u16);
}

void ld_m16_r8(Cpu* cpu, InstructionRuntime* instr)
{
   uint16_t* reg1 = cpu->reg.Reg16(instr->def->op1);
   uint8_t* reg2 = cpu->reg.Reg8(instr->def->op2);

   cpu->memory->Write8(*reg1, *reg2);
}

void ld_m16_d8(Cpu* cpu, InstructionRuntime* instr)
{
   uint16_t* reg1 = cpu->reg.Reg16(instr->def->op1);

   cpu->memory->Write8(*reg1, instr->imm.u8);
}

void ldi_r8_m16(Cpu* cpu, InstructionRuntime* instr)
{
   uint8_t* reg1 = cpu->reg.Reg8(instr->def->op1);
   uint16_t* reg2 = cpu->reg.Reg16(instr->def->op2);

   *reg1 = cpu->memory->Read8(*reg2);

   *reg2 += 1;
}

void ldi_m16_r8(Cpu* cpu, InstructionRuntime* instr)
{
   uint16_t* reg1 = cpu->reg.Reg16(instr->def->op1);
   uint8_t* reg2 = cpu->reg.Reg8(instr->def->op2);

   cpu->memory->Write8(*reg1, *reg2);

   *reg1 += 1;
}

void ldd_m16_r8(Cpu* cpu, InstructionRuntime* instr)
{
   uint16_t* reg1 = cpu->reg.Reg16(instr->def->op1);
   uint8_t* reg2 = cpu->reg.Reg8(instr->def->op2);

   cpu->memory->Write8(*reg1, *reg2);

   *reg1 -= 1;
}

void ld_m8_r8(Cpu* cpu, InstructionRuntime* instr)
{
   uint8_t* reg1 = cpu->reg.Reg8(instr->def->op1);
   uint8_t* reg2 = cpu->reg.Reg8(instr->def->op2);

   cpu->memory->Write8(0xFF00 | *reg1, *reg2);
}

void ld_r8_imm8(Cpu* cpu, InstructionRuntime* instr)
{
   uint8_t* reg1 = cpu->reg.Reg8(instr->def->op1);

   *reg1 = cpu->memory->Read8(0xFF00 | instr->imm.u8);
}

void ld_r8_imm16(Cpu* cpu, InstructionRuntime* instr)
{
   uint8_t* reg = cpu->reg.Reg8(instr->def->op1);

   *reg = cpu->memory->Read8(instr->imm.u16);
}

void ld_r8_m16(Cpu* cpu, InstructionRuntime* instr)
{
   uint8_t* reg1 = cpu->reg.Reg8(instr->def->op1);
   uint16_t* reg2 = cpu->reg.Reg16(instr->def->op2);

   *reg1 = cpu->memory->Read8(*reg2);
}


void ld_imm8_r8(Cpu* cpu, InstructionRuntime* instr)
{
   const uint8_t* reg = cpu->reg.Reg8(instr->def->op1);

   cpu->memory->Write8(0xFF00 | instr->imm.u8, *reg);
}

void ld_imm16_r8(Cpu* cpu, InstructionRuntime* instr)
{
   const uint8_t* reg = cpu->reg.Reg8(instr->def->op1);

   cpu->memory->Write8(instr->imm.u16, *reg);
}

void ld_imm16_r16(Cpu* cpu, InstructionRuntime* instr)
{
   const uint16_t* reg = cpu->reg.Reg16(instr->def->op1);

   cpu->memory->Write8(instr->imm.u16, *reg);
}

void ld_r8_d8(Cpu* cpu, InstructionRuntime* instr)
{
   uint8_t* reg = cpu->reg.Reg8(instr->def->op1);
   *reg = instr->imm.u8;
}

// Math
void add_r8_r8(Cpu* cpu, InstructionRuntime* instr)
{
   uint8_t* reg1 = cpu->reg.Reg8(instr->def->op1);
   uint8_t* reg2 = cpu->reg.Reg8(instr->def->op2);

   const uint16_t result16 = *reg1 + *reg2;
   const uint8_t result8 = result16 & 0xFF;
   const uint8_t half_carry = HALF_CARRY_ADD(*reg1, *reg2);
   const uint8_t carry = result16 > U8_MASK;

   *reg1 = result8;

   cpu->reg.SetFlag(FlagType::FLAG_Z, result8 == 0);
   cpu->reg.SetFlag(FlagType::FLAG_N, 0);
   cpu->reg.SetFlag(FlagType::FLAG_H, half_carry);
   cpu->reg.SetFlag(FlagType::FLAG_C, carry);
}

void add_r8_d8(Cpu* cpu, InstructionRuntime* instr)
{
   uint8_t* reg1 = cpu->reg.Reg8(instr->def->op1);
   uint8_t imm = instr->imm.u8;

   const uint16_t result16 = *reg1 + imm;
   const uint8_t result8 = result16 & 0xFF;
   const uint8_t half_carry = HALF_CARRY_ADD(*reg1, imm);
   const uint8_t carry = result16 > U8_MASK;

   *reg1 = result8;

   cpu->reg.SetFlag(FlagType::FLAG_Z, result8 == 0);
   cpu->reg.SetFlag(FlagType::FLAG_N, 0);
   cpu->reg.SetFlag(FlagType::FLAG_H, half_carry);
   cpu->reg.SetFlag(FlagType::FLAG_C, carry);
}


void add_r16_r16(Cpu* cpu, InstructionRuntime* instr)
{
   uint16_t* reg1 = cpu->reg.Reg16(instr->def->op1);
   uint16_t* reg2 = cpu->reg.Reg16(instr->def->op2);

   const uint32_t result32 = *reg1 + *reg2;
   const uint16_t result16 = result32 & 0xFFFF;
   const uint16_t half_carry = HALF_CARRY_ADD_16(*reg1, *reg2);
   const uint16_t carry = result32 > U16_MASK;

   *reg1 = result16;

   cpu->reg.SetFlag(FlagType::FLAG_N, 0);
   cpu->reg.SetFlag(FlagType::FLAG_H, half_carry);
   cpu->reg.SetFlag(FlagType::FLAG_C, carry);
}

void add_r16_s8(Cpu* cpu, InstructionRuntime* instr)
{
   uint16_t* reg = cpu->reg.Reg16(instr->def->op1);

   const uint32_t result32 = *reg + instr->imm.s8;
   const uint16_t result16 = result32 & 0xFFFF;
   const uint8_t half_carry = HALF_CARRY_ADD(*reg  & 0xFF, instr->imm.s8 & 0xFF);
   const uint8_t carry = ((*reg  & 0xFF) + static_cast<uint8_t>(instr->imm.s8)) > 0xFF;

   *reg = result16;

   cpu->reg.SetFlag(FlagType::FLAG_Z, 0);
   cpu->reg.SetFlag(FlagType::FLAG_N, 0);
   cpu->reg.SetFlag(FlagType::FLAG_H, half_carry);
   cpu->reg.SetFlag(FlagType::FLAG_C, carry);
}

void add_r8_m16(Cpu* cpu, InstructionRuntime* instr)
{
   uint8_t* reg1 = cpu->reg.Reg8(instr->def->op1);
   uint16_t* reg2 = cpu->reg.Reg16(instr->def->op2);
   uint8_t memory_value = cpu->memory->Read8(*reg2);

   const uint16_t result16 = *reg1 + memory_value;
   const uint8_t result8 = result16 & 0xFF;
   const uint8_t half_carry = HALF_CARRY_ADD(*reg1, memory_value);
   const uint8_t carry = result16 > U8_MASK;

   *reg1 = result8;

   cpu->reg.SetFlag(FlagType::FLAG_Z, result8 == 0);
   cpu->reg.SetFlag(FlagType::FLAG_N, 0);
   cpu->reg.SetFlag(FlagType::FLAG_H, half_carry);
   cpu->reg.SetFlag(FlagType::FLAG_C, carry);
}

void sub_r8_r8(Cpu* cpu, InstructionRuntime* instr)
{
   uint8_t* reg1 = cpu->reg.Reg8(instr->def->op1);
   uint8_t* reg2 = cpu->reg.Reg8(instr->def->op2);

   const uint8_t result = *reg1 - *reg2;
   const uint8_t half_carry = HALF_CARRY_SUB(*reg1, *reg2);
   const uint8_t carry = *reg1 < *reg2;

   *reg1 = result;

   cpu->reg.SetFlag(FlagType::FLAG_Z, result == 0);
   cpu->reg.SetFlag(FlagType::FLAG_N, 1);
   cpu->reg.SetFlag(FlagType::FLAG_H, half_carry);
   cpu->reg.SetFlag(FlagType::FLAG_C, carry);
}

void xor_r8_r8(Cpu* cpu, InstructionRuntime* instr)
{
   uint8_t* reg1 = cpu->reg.Reg8(instr->def->op1);
   uint8_t* reg2 = cpu->reg.Reg8(instr->def->op2);
   *reg1 ^= *reg2;

   cpu->reg.SetFlag(FlagType::FLAG_Z, *reg1 == 0);
   cpu->reg.SetFlag(FlagType::FLAG_N, 0);
   cpu->reg.SetFlag(FlagType::FLAG_H, 0);
   cpu->reg.SetFlag(FlagType::FLAG_C, 0);
}

void or_r8_r8(Cpu* cpu, InstructionRuntime* instr)
{
   uint8_t* reg1 = cpu->reg.Reg8(instr->def->op1);
   uint8_t* reg2 = cpu->reg.Reg8(instr->def->op2);
   *reg1 |= *reg2;

   cpu->reg.SetFlag(FlagType::FLAG_Z, *reg1 == 0);
   cpu->reg.SetFlag(FlagType::FLAG_N, 0);
   cpu->reg.SetFlag(FlagType::FLAG_H, 0);
   cpu->reg.SetFlag(FlagType::FLAG_C, 0);
}

void and_r8_r8(Cpu* cpu, InstructionRuntime* instr)
{
   uint8_t* reg1 = cpu->reg.Reg8(instr->def->op1);
   uint8_t* reg2 = cpu->reg.Reg8(instr->def->op2);
   *reg1 &= *reg2;

   cpu->reg.SetFlag(FlagType::FLAG_Z, *reg1 == 0);
   cpu->reg.SetFlag(FlagType::FLAG_N, 0);
   cpu->reg.SetFlag(FlagType::FLAG_H, 1);
   cpu->reg.SetFlag(FlagType::FLAG_C, 0);
}

void and_r8_d8(Cpu* cpu, InstructionRuntime* instr)
{
   uint8_t* reg = cpu->reg.Reg8(instr->def->op1);
   *reg &= instr->imm.u8;

   cpu->reg.SetFlag(FlagType::FLAG_Z, *reg == 0);
   cpu->reg.SetFlag(FlagType::FLAG_N, 0);
   cpu->reg.SetFlag(FlagType::FLAG_H, 1);
   cpu->reg.SetFlag(FlagType::FLAG_C, 0);
}

void inc_r8(Cpu* cpu, InstructionRuntime* instr)
{
   uint8_t* reg = cpu->reg.Reg8(instr->def->op1);
   const uint8_t half_carry = HALF_CARRY_ADD(*reg, 1);

   *reg += 1;

   cpu->reg.SetFlag(FlagType::FLAG_Z, *reg == 0);
   cpu->reg.SetFlag(FlagType::FLAG_N, 0);
   cpu->reg.SetFlag(FlagType::FLAG_H, half_carry);
}

void inc_r16(Cpu* cpu, InstructionRuntime* instr)
{
   uint16_t* reg = cpu->reg.Reg16(instr->def->op1);
   *reg += 1;
}

void inc_m16(Cpu* cpu, InstructionRuntime* instr)
{
   uint16_t* reg = cpu->reg.Reg16(instr->def->op1);
   const uint8_t memory_value = cpu->memory->Read8(*reg);

   const uint8_t half_carry = HALF_CARRY_ADD(memory_value, 1);

   cpu->memory->Write8(*reg, memory_value + 1); // TODO replace with memory ptrs

   cpu->reg.SetFlag(FlagType::FLAG_Z, *reg == 0);
   cpu->reg.SetFlag(FlagType::FLAG_N, 0);
   cpu->reg.SetFlag(FlagType::FLAG_H, half_carry);
}

void dec_r8(Cpu* cpu, InstructionRuntime* instr)
{
   uint8_t* reg = cpu->reg.Reg8(instr->def->op1);
   const uint8_t half_carry = HALF_CARRY_SUB(*reg, 1);

   *reg -= 1;

   cpu->reg.SetFlag(FlagType::FLAG_Z, *reg == 0);
   cpu->reg.SetFlag(FlagType::FLAG_N, 1);
   cpu->reg.SetFlag(FlagType::FLAG_H, half_carry);
}

void dec_r16(Cpu* cpu, InstructionRuntime* instr)
{
   uint16_t* reg = cpu->reg.Reg16(instr->def->op1);
   *reg -= 1;
}

void dec_m16(Cpu* cpu, InstructionRuntime* instr)
{
   const uint16_t* reg = cpu->reg.Reg16(instr->def->op1);
   const uint8_t memory_value = cpu->memory->Read8(*reg);

   const uint8_t half_carry = HALF_CARRY_SUB(memory_value, 1);

   cpu->memory->Write8(*reg, memory_value - 1);

   cpu->reg.SetFlag(FlagType::FLAG_Z, *reg == 0);
   cpu->reg.SetFlag(FlagType::FLAG_N, 1);
   cpu->reg.SetFlag(FlagType::FLAG_H, half_carry);
}

void cp_r8_d8(Cpu* cpu, InstructionRuntime* instr)
{
   const uint8_t* reg = cpu->reg.Reg8(instr->def->op1);

   const uint8_t result = *reg - instr->imm.u8;
   const uint8_t half_carry = HALF_CARRY_SUB(*reg, instr->imm.u8);
   const uint8_t carry = *reg < instr->imm.u8;

   cpu->reg.SetFlag(FlagType::FLAG_Z, result == 0);
   cpu->reg.SetFlag(FlagType::FLAG_N, 1);
   cpu->reg.SetFlag(FlagType::FLAG_H, half_carry);
   cpu->reg.SetFlag(FlagType::FLAG_C, carry);
}

void cp_r8_m16(Cpu* cpu, InstructionRuntime* instr)
{
   const uint8_t* reg1 = cpu->reg.Reg8(instr->def->op1);
   const uint16_t* reg2 = cpu->reg.Reg16(instr->def->op2);
   uint8_t memory_value = cpu->memory->Read8(*reg2);

   const uint8_t result = *reg1 - memory_value;
   const uint8_t half_carry = HALF_CARRY_SUB(*reg1, memory_value);
   const uint8_t carry = *reg1 < memory_value;

   cpu->reg.SetFlag(FlagType::FLAG_Z, result == 0);
   cpu->reg.SetFlag(FlagType::FLAG_N, 1);
   cpu->reg.SetFlag(FlagType::FLAG_H, half_carry);
   cpu->reg.SetFlag(FlagType::FLAG_C, carry);
}

void rla(Cpu* cpu, InstructionRuntime* instr)
{
   uint8_t* reg = cpu->reg.Reg8(RegisterType::REG_A);

   const uint8_t carry_bit = (*reg >> 7) & 1;
   *reg = (*reg << 1) | cpu->reg.GetFlag(FlagType::FLAG_C);

   cpu->reg.SetFlag(FlagType::FLAG_Z, 0);
   cpu->reg.SetFlag(FlagType::FLAG_N, 0);
   cpu->reg.SetFlag(FlagType::FLAG_H, 0);
   cpu->reg.SetFlag(FlagType::FLAG_C, carry_bit);
}

void rrca(Cpu* cpu, InstructionRuntime* instr)
{
   uint8_t* reg = cpu->reg.Reg8(RegisterType::REG_A);

   const uint8_t carry_bit = *reg & 1;
   *reg = (*reg >> 1) | carry_bit << 7;

   cpu->reg.SetFlag(FlagType::FLAG_Z, 0);
   cpu->reg.SetFlag(FlagType::FLAG_N, 0);
   cpu->reg.SetFlag(FlagType::FLAG_H, 0);
   cpu->reg.SetFlag(FlagType::FLAG_C, carry_bit);
}

void cpl(Cpu* cpu, InstructionRuntime* instr)
{
   uint8_t* reg = cpu->reg.Reg8(RegisterType::REG_A);
   *reg = ~(*reg);

   cpu->reg.SetFlag(FlagType::FLAG_N, 1);
   cpu->reg.SetFlag(FlagType::FLAG_H, 1);
}

// CB
void bit_imm_r8(Cpu* cpu, InstructionRuntime* instr)
{
   uint8_t bit = instr->def->param1;
   uint8_t* reg = cpu->reg.Reg8(instr->def->op1);

   cpu->reg.SetFlag(FlagType::FLAG_Z, (*reg & (1 << bit)) == 0);
   cpu->reg.SetFlag(FlagType::FLAG_N, 0);
   cpu->reg.SetFlag(FlagType::FLAG_H, 1);
}

void bit_imm_m16(Cpu* cpu, InstructionRuntime* instr)
{
   uint8_t bit = instr->def->param1;
   uint16_t* reg = cpu->reg.Reg16(instr->def->op1);
   uint8_t memory_value = cpu->memory->Read8(*reg);

   cpu->reg.SetFlag(FlagType::FLAG_Z, (memory_value & (1 << bit)) == 0);
   cpu->reg.SetFlag(FlagType::FLAG_N, 0);
   cpu->reg.SetFlag(FlagType::FLAG_H, 1);
}

void rl_r8(Cpu* cpu, InstructionRuntime* instr)
{
   uint8_t* reg = cpu->reg.Reg8(instr->def->op1);

   const uint8_t carry_bit = (*reg >> 7) & 1;
   *reg = (*reg << 1) | cpu->reg.GetFlag(FlagType::FLAG_C);

   cpu->reg.SetFlag(FlagType::FLAG_Z, *reg == 0);
   cpu->reg.SetFlag(FlagType::FLAG_N, 0);
   cpu->reg.SetFlag(FlagType::FLAG_H, 0);
   cpu->reg.SetFlag(FlagType::FLAG_C, carry_bit);
}

void sla_r8(Cpu* cpu, InstructionRuntime* instr)
{
   uint8_t* reg = cpu->reg.Reg8(instr->def->op1);

   const uint8_t carry_bit = (*reg >> 7) & 1;
   *reg <<= 1;

   cpu->reg.SetFlag(FlagType::FLAG_Z, 0);
   cpu->reg.SetFlag(FlagType::FLAG_N, 0);
   cpu->reg.SetFlag(FlagType::FLAG_H, 0);
   cpu->reg.SetFlag(FlagType::FLAG_C, carry_bit);
}

void swap_r8(Cpu* cpu, InstructionRuntime* instr)
{
   uint8_t* reg = cpu->reg.Reg8(instr->def->op1);
   *reg = (*reg & 0b1111) << 4 | (*reg >> 4);

   cpu->reg.SetFlag(FlagType::FLAG_Z, *reg == 0);
   cpu->reg.SetFlag(FlagType::FLAG_N, 0);
   cpu->reg.SetFlag(FlagType::FLAG_H, 0);
   cpu->reg.SetFlag(FlagType::FLAG_C, 0);
}

void res_bit_r8(Cpu* cpu, InstructionRuntime* instr)
{
   uint8_t* reg = cpu->reg.Reg8(instr->def->op1);
   *reg &= ~(1 << instr->def->param1);
}

InstructionDef* InstructionSet::instructions[INSTRUCTION_SET_SIZE] = {

   new InstructionDef("NOP", 0x00, nop, 1, 1, 1),
   new InstructionDef("LD BC, d16", 0x01, ld_r16_d16, 3, 3, 3, RegisterType::REG_BC),
   new InstructionDef("INC BC", 0x03, inc_r16, 1, 2, 2, RegisterType::REG_BC),
   new InstructionDef("INC B", 0x04, inc_r8, 1, 1, 1, RegisterType::REG_B),
   new InstructionDef("DEC B", 0x05, dec_r8, 1, 1, 1, RegisterType::REG_B),
   new InstructionDef("LD B, d8", 0x06, ld_r8_d8, 2, 2, 2, RegisterType::REG_B),
   new InstructionDef("LD (a16), SP", 0x08, ld_imm16_r16, 3, 5, 5, RegisterType::REG_SP),
   new InstructionDef("ADD HL, BC", 0x09, add_r16_r16, 1, 2, 2, RegisterType::REG_HL, RegisterType::REG_BC),
   new InstructionDef("LD A, (BC)", 0x0A, ld_r8_m16, 1, 2, 2, RegisterType::REG_A, RegisterType::REG_BC),
   new InstructionDef("DEC BC", 0x0B, dec_r16, 1, 2, 2, RegisterType::REG_BC),
   new InstructionDef("INC C", 0x0C, inc_r8, 1, 1, 1, RegisterType::REG_C),
   new InstructionDef("DEC C", 0x0D, dec_r8, 1, 1, 1, RegisterType::REG_C),
   new InstructionDef("LD C, d8", 0x0E, ld_r8_d8, 2, 2, 2, RegisterType::REG_C),
   new InstructionDef("RRCA", 0x0F, rrca, 1, 1, 1),

   new InstructionDef("LD DE, d16", 0x11, ld_r16_d16, 3, 3, 3, RegisterType::REG_DE),
   new InstructionDef("LD (DE), A", 0x12, ld_m16_r8, 1, 2, 2, RegisterType::REG_DE, RegisterType::REG_A),
   new InstructionDef("INC DE", 0x13, inc_r16, 1, 2, 2, RegisterType::REG_DE),
   new InstructionDef("DEC D", 0x15, dec_r8, 1, 1, 1, RegisterType::REG_D),
   new InstructionDef("LD D, d8", 0x16, ld_r8_d8, 2, 2, 2, RegisterType::REG_D),
   new InstructionDef("RLA", 0x17, rla, 1, 1, 1),
   new InstructionDef("JR s8", 0x18, jr_s8, 2, 3, 3),
   new InstructionDef("ADD HL, DE", 0x19, add_r16_r16, 1, 2, 2, RegisterType::REG_HL, RegisterType::REG_DE),
   new InstructionDef("LD A, (DE)", 0x1A, ld_r8_m16, 1, 2, 2, RegisterType::REG_A, RegisterType::REG_DE),
   new InstructionDef("INC E", 0x1C, inc_r8, 1, 1, 1, RegisterType::REG_E),
   new InstructionDef("DEC E", 0x1D, dec_r8, 1, 1, 1, RegisterType::REG_E),
   new InstructionDef("LD E, d8", 0x1E, ld_r8_d8, 2, 2, 2, RegisterType::REG_E),

   new InstructionDef("JR NZ, s8", 0x20, jr_nz_s8, 2, 3, 2),
   new InstructionDef("LD HL, d16", 0x21, ld_r16_d16, 3, 3, 3, RegisterType::REG_HL),
   new InstructionDef("LD (HL+), A", 0x22, ldi_m16_r8, 1, 2, 2, RegisterType::REG_HL, RegisterType::REG_A),
   new InstructionDef("INC HL", 0x23, inc_r16, 1, 2, 2, RegisterType::REG_HL),
   new InstructionDef("INC H", 0x24, inc_r8, 1, 1, 1, RegisterType::REG_H),
   new InstructionDef("JR Z, s8", 0x28, jr_z_s8, 2, 3, 2),
   new InstructionDef("LD A, (HL+)", 0x2A, ldi_r8_m16, 1, 2, 2, RegisterType::REG_A, RegisterType::REG_HL),
   new InstructionDef("INC L", 0x2C, inc_r8, 1, 1, 1, RegisterType::REG_L),
   new InstructionDef("DEC L", 0x2D, dec_r8, 1, 1, 1, RegisterType::REG_L),
   new InstructionDef("LD L, d8", 0x2E, ld_r8_d8, 2, 2, 2, RegisterType::REG_L),
   new InstructionDef("CPL", 0x2F, cpl, 1, 1, 1),

   new InstructionDef("LD SP, d16", 0x31, ld_r16_d16, 3, 3, 3, RegisterType::REG_SP),
   new InstructionDef("LD (HL-), A", 0x32, ldd_m16_r8, 1, 2, 2, RegisterType::REG_HL, RegisterType::REG_A),
   new InstructionDef("INC (HL)", 0x34, inc_m16, 1, 3, 3, RegisterType::REG_HL),
   new InstructionDef("DEC (HL)", 0x35, dec_m16, 1, 3, 3, RegisterType::REG_HL),
   new InstructionDef("LD (HL), d8", 0x36, ld_m16_d8, 2, 3, 3, RegisterType::REG_HL),
   new InstructionDef("JR C, s8", 0x38, jr_c_s8, 2, 3, 2),
   new InstructionDef("ADD HL, SP", 0x39, add_r16_r16, 1, 2, 2, RegisterType::REG_HL, RegisterType::REG_SP),
   new InstructionDef("INC A", 0x3C, inc_r8, 1, 1, 1, RegisterType::REG_A),
   new InstructionDef("DEC A", 0x3D, dec_r8, 1, 1, 1, RegisterType::REG_A),
   new InstructionDef("LD A, d8", 0x3E, ld_r8_d8, 2, 2, 2, RegisterType::REG_A),

   new InstructionDef("LD B, B", 0x40, ld_r8_r8, 1, 1, 1, RegisterType::REG_B, RegisterType::REG_B),
   new InstructionDef("LD B, (HL)", 0x46, ld_r8_m16, 1, 2, 2, RegisterType::REG_B, RegisterType::REG_HL),
   new InstructionDef("LD B, A", 0x47, ld_r8_r8, 1, 1, 1, RegisterType::REG_B, RegisterType::REG_A),
   new InstructionDef("LD C, A", 0x4F, ld_r8_r8, 1, 1, 1, RegisterType::REG_C, RegisterType::REG_A),
   new InstructionDef("LD C, (HL)", 0x4E, ld_r8_m16, 1, 2, 2, RegisterType::REG_C, RegisterType::REG_HL),

   new InstructionDef("LD D, (HL)", 0x56, ld_r8_m16, 1, 2, 2, RegisterType::REG_D, RegisterType::REG_HL),
   new InstructionDef("LD D, A", 0x57, ld_r8_r8, 1, 1, 1, RegisterType::REG_D, RegisterType::REG_A),
   new InstructionDef("LD E, (HL)", 0x5E, ld_r8_m16, 1, 2, 2, RegisterType::REG_E, RegisterType::REG_HL),
   new InstructionDef("LD E, A", 0x5F, ld_r8_r8, 1, 1, 1, RegisterType::REG_E, RegisterType::REG_A),
   new InstructionDef("LD H, B", 0x60, ld_r8_r8, 1, 1, 1, RegisterType::REG_H, RegisterType::REG_B),
   new InstructionDef("LD H, A", 0x67, ld_r8_r8, 1, 1, 1, RegisterType::REG_H, RegisterType::REG_A),
   new InstructionDef("LD L, C", 0x69, ld_r8_r8, 1, 1, 1, RegisterType::REG_L, RegisterType::REG_C),
   new InstructionDef("LD L, H", 0x6C, ld_r8_r8, 1, 1, 1, RegisterType::REG_L, RegisterType::REG_H),
   new InstructionDef("LD L, A", 0x6F, ld_r8_r8, 1, 1, 1, RegisterType::REG_L, RegisterType::REG_A),

   new InstructionDef("LD (HL), A", 0x77, ld_m16_r8, 1, 2, 2, RegisterType::REG_HL, RegisterType::REG_A),
   new InstructionDef("LD A, B", 0x78, ld_r8_r8, 1, 1, 1, RegisterType::REG_A, RegisterType::REG_B),
   new InstructionDef("LD A, C", 0x79, ld_r8_r8, 1, 1, 1, RegisterType::REG_A, RegisterType::REG_C),
   new InstructionDef("LD A, E", 0x7B, ld_r8_r8, 1, 1, 1, RegisterType::REG_A, RegisterType::REG_E),
   new InstructionDef("LD A, H", 0x7C, ld_r8_r8, 1, 1, 1, RegisterType::REG_A, RegisterType::REG_H),
   new InstructionDef("LD A, L", 0x7D, ld_r8_r8, 1, 1, 1, RegisterType::REG_A, RegisterType::REG_L),
   new InstructionDef("LD A, (HL)", 0x7E, ld_r8_m16, 1, 2, 2, RegisterType::REG_A, RegisterType::REG_HL),
   new InstructionDef("ADD A, B", 0x80, add_r8_r8, 1, 1, 1, RegisterType::REG_A, RegisterType::REG_B),
   new InstructionDef("ADD A, L", 0x85, add_r8_r8, 1, 1, 1, RegisterType::REG_A, RegisterType::REG_L),
   new InstructionDef("ADD A, (HL)", 0x86, add_r8_m16, 1, 2, 2, RegisterType::REG_A, RegisterType::REG_HL),
   new InstructionDef("ADD A, A", 0x87, add_r8_r8, 1, 1, 1, RegisterType::REG_A, RegisterType::REG_A),
   new InstructionDef("SUB A, B", 0x90, sub_r8_r8, 1, 1, 1, RegisterType::REG_A, RegisterType::REG_B),
   new InstructionDef("AND A, C", 0xA1, and_r8_r8, 1, 1, 1, RegisterType::REG_A, RegisterType::REG_C),
   new InstructionDef("AND A, A", 0xA7, and_r8_r8, 1, 1, 1, RegisterType::REG_A, RegisterType::REG_A),
   new InstructionDef("XOR A, C", 0xA9, xor_r8_r8, 1, 1, 1, RegisterType::REG_A, RegisterType::REG_C),
   new InstructionDef("XOR A, A", 0xAF, xor_r8_r8, 1, 1, 1, RegisterType::REG_A, RegisterType::REG_A),
   new InstructionDef("OR A, B", 0xB0, or_r8_r8, 1, 1, 1, RegisterType::REG_A, RegisterType::REG_B),
   new InstructionDef("OR A, C", 0xB1, or_r8_r8, 1, 1, 1, RegisterType::REG_A, RegisterType::REG_C),
   new InstructionDef("CP A, (HL))", 0xBE, cp_r8_m16, 1, 2, 2, RegisterType::REG_A, RegisterType::REG_HL),

   new InstructionDef("RET NZ", 0xC0, ret_nz, 1, 5, 2),
   new InstructionDef("POP BC", 0xC1, pop_r16, 1, 3, 3, RegisterType::REG_BC),
   new InstructionDef("JP NZ, a16", 0xC2, jp_nz_a16, 3, 4, 3),
   new InstructionDef("JP a16", 0xC3, jp_a16, 3, 4, 4),
   new InstructionDef("PUSH BC", 0xC5, push_r16, 1, 4, 4, RegisterType::REG_BC),
   new InstructionDef("ADD A, d8", 0xC6, add_r8_d8, 2, 2, 2, RegisterType::REG_A),
   new InstructionDef("RET Z", 0xC8, ret_z, 1, 5, 2),
   new InstructionDef("RET", 0xC9, ret, 1, 4, 4),
   new InstructionDef("JP Z, a16", 0xCA, jp_z_a16, 3, 4, 3),
   new InstructionDef("CB", 0xCB, nullptr, 0, 0, 0),
   new InstructionDef("CALL a16", 0xCD, call_a16, 3, 6, 6),

   new InstructionDef("RET NC", 0xD0, ret_nc, 1, 5, 2),
   new InstructionDef("POP DE", 0xD1, pop_r16, 1, 3, 3, RegisterType::REG_DE),
   new InstructionDef("PUSH DE", 0xD5, push_r16, 1, 4, 4, RegisterType::REG_DE),
   new InstructionDef("RETI", 0xD9, reti, 1, 4, 4),
   new InstructionDef("LD (a8), A", 0xE0, ld_imm8_r8, 2, 3, 3, RegisterType::REG_A),
   new InstructionDef("POP HL", 0xE1, pop_r16, 1, 3, 3, RegisterType::REG_HL),
   new InstructionDef("LD (C), A", 0xE2, ld_m8_r8, 1, 2, 2, RegisterType::REG_C, RegisterType::REG_A),
   new InstructionDef("PUSH HL", 0xE5, push_r16, 1, 4, 4, RegisterType::REG_HL),
   new InstructionDef("AND A, d8", 0xE6, and_r8_d8, 2, 2, 2, RegisterType::REG_A),
   new InstructionDef("ADD SP, s8", 0xE8, add_r16_s8, 2, 4, 4, RegisterType::REG_SP),
   new InstructionDef("JP HL", 0xE9, jp_r16, 1, 1, 1, RegisterType::REG_HL),
   new InstructionDef("LD (a16), A", 0xEA, ld_imm16_r8, 3, 4, 4, RegisterType::REG_A),
   new InstructionDef("RST 5", 0xEF, rst, 1, 4, 4, RegisterType::REG_NONE, RegisterType::REG_NONE, 0x28),
   new InstructionDef("LD A, (a8)", 0xF0, ld_r8_imm8, 2, 3, 3, RegisterType::REG_A),
   new InstructionDef("POP AF", 0xF1, pop_r16, 1, 3, 3, RegisterType::REG_AF),
   new InstructionDef("DI", 0xF3, di, 1, 1, 1),
   new InstructionDef("PUSH AF", 0xF5, push_r16, 1, 4, 4, RegisterType::REG_AF),
   new InstructionDef("LD A, (a16)", 0xFA, ld_r8_imm16, 3, 4, 4, RegisterType::REG_A),
   new InstructionDef("EI", 0xFB, ei, 1, 1, 1),
   new InstructionDef("CP A, d8", 0xFE, cp_r8_d8, 2, 2, 2, RegisterType::REG_A),
   new InstructionDef("RST 7", 0xFF, rst, 1, 4, 4, RegisterType::REG_NONE, RegisterType::REG_NONE, 0x38),
};

InstructionDef* InstructionSet::prefix_instructions[INSTRUCTION_SET_SIZE] = {
   new InstructionDef("RL C", 0x11, rl_r8, 2, 2, 2, RegisterType::REG_C),
   new InstructionDef("SLA A", 0x27, sla_r8, 2, 2, 2, RegisterType::REG_A),
   new InstructionDef("SWAP A", 0x37, swap_r8, 2, 2, 2, RegisterType::REG_A),

   // BIT 0
   new InstructionDef("BIT 0, B", 0x40, bit_imm_r8, 2, 2, 2, RegisterType::REG_B, RegisterType::REG_NONE, 0),
   new InstructionDef("BIT 0, C", 0x41, bit_imm_r8, 2, 2, 2, RegisterType::REG_C, RegisterType::REG_NONE, 0),
   new InstructionDef("BIT 0, D", 0x42, bit_imm_r8, 2, 2, 2, RegisterType::REG_D, RegisterType::REG_NONE, 0),
   new InstructionDef("BIT 0, E", 0x43, bit_imm_r8, 2, 2, 2, RegisterType::REG_E, RegisterType::REG_NONE, 0),
   new InstructionDef("BIT 0, H", 0x44, bit_imm_r8, 2, 2, 2, RegisterType::REG_H, RegisterType::REG_NONE, 0),
   new InstructionDef("BIT 0, L", 0x45, bit_imm_r8, 2, 2, 2, RegisterType::REG_L, RegisterType::REG_NONE, 0),
   new InstructionDef("BIT 0, (HL)", 0x46, bit_imm_m16, 2, 3, 3, RegisterType::REG_HL, RegisterType::REG_NONE, 0),
   new InstructionDef("BIT 0, A", 0x47, bit_imm_r8, 2, 2, 2, RegisterType::REG_A, RegisterType::REG_NONE, 0),

   // BIT 1
   new InstructionDef("BIT 1, B", 0x48, bit_imm_r8, 2, 2, 2, RegisterType::REG_B, RegisterType::REG_NONE, 1),
   new InstructionDef("BIT 1, C", 0x49, bit_imm_r8, 2, 2, 2, RegisterType::REG_C, RegisterType::REG_NONE, 1),
   new InstructionDef("BIT 1, D", 0x4A, bit_imm_r8, 2, 2, 2, RegisterType::REG_D, RegisterType::REG_NONE, 1),
   new InstructionDef("BIT 1, E", 0x4B, bit_imm_r8, 2, 2, 2, RegisterType::REG_E, RegisterType::REG_NONE, 1),
   new InstructionDef("BIT 1, H", 0x4C, bit_imm_r8, 2, 2, 2, RegisterType::REG_H, RegisterType::REG_NONE, 1),
   new InstructionDef("BIT 1, L", 0x4D, bit_imm_r8, 2, 2, 2, RegisterType::REG_L, RegisterType::REG_NONE, 1),
   new InstructionDef("BIT 1, (HL)", 0x4E, bit_imm_m16, 2, 3, 3, RegisterType::REG_HL, RegisterType::REG_NONE, 1),
   new InstructionDef("BIT 1, A", 0x4F, bit_imm_r8, 2, 2, 2, RegisterType::REG_A, RegisterType::REG_NONE, 1),

   // BIT 2
   new InstructionDef("BIT 2, B", 0x50, bit_imm_r8, 2, 2, 2, RegisterType::REG_B, RegisterType::REG_NONE, 2),
   new InstructionDef("BIT 2, C", 0x51, bit_imm_r8, 2, 2, 2, RegisterType::REG_C, RegisterType::REG_NONE, 2),
   new InstructionDef("BIT 2, D", 0x52, bit_imm_r8, 2, 2, 2, RegisterType::REG_D, RegisterType::REG_NONE, 2),
   new InstructionDef("BIT 2, E", 0x53, bit_imm_r8, 2, 2, 2, RegisterType::REG_E, RegisterType::REG_NONE, 2),
   new InstructionDef("BIT 2, H", 0x54, bit_imm_r8, 2, 2, 2, RegisterType::REG_H, RegisterType::REG_NONE, 2),
   new InstructionDef("BIT 2, L", 0x55, bit_imm_r8, 2, 2, 2, RegisterType::REG_L, RegisterType::REG_NONE, 2),
   new InstructionDef("BIT 2, (HL)", 0x56, bit_imm_m16, 2, 3, 3, RegisterType::REG_HL, RegisterType::REG_NONE, 2),
   new InstructionDef("BIT 2, A", 0x57, bit_imm_r8, 2, 2, 2, RegisterType::REG_A, RegisterType::REG_NONE, 2),

   // BIT 3
   new InstructionDef("BIT 3, B", 0x58, bit_imm_r8, 2, 2, 2, RegisterType::REG_B, RegisterType::REG_NONE, 3),
   new InstructionDef("BIT 3, C", 0x59, bit_imm_r8, 2, 2, 2, RegisterType::REG_C, RegisterType::REG_NONE, 3),
   new InstructionDef("BIT 3, D", 0x5A, bit_imm_r8, 2, 2, 2, RegisterType::REG_D, RegisterType::REG_NONE, 3),
   new InstructionDef("BIT 3, E", 0x5B, bit_imm_r8, 2, 2, 2, RegisterType::REG_E, RegisterType::REG_NONE, 3),
   new InstructionDef("BIT 3, H", 0x5C, bit_imm_r8, 2, 2, 2, RegisterType::REG_H, RegisterType::REG_NONE, 3),
   new InstructionDef("BIT 3, L", 0x5D, bit_imm_r8, 2, 2, 2, RegisterType::REG_L, RegisterType::REG_NONE, 3),
   new InstructionDef("BIT 3, (HL)", 0x5E, bit_imm_m16, 2, 3, 3, RegisterType::REG_HL, RegisterType::REG_NONE, 3),
   new InstructionDef("BIT 3, A", 0x5F, bit_imm_r8, 2, 2, 2, RegisterType::REG_A, RegisterType::REG_NONE, 3),

   // BIT 4
   new InstructionDef("BIT 4, B", 0x60, bit_imm_r8, 2, 2, 2, RegisterType::REG_B, RegisterType::REG_NONE, 4),
   new InstructionDef("BIT 4, C", 0x61, bit_imm_r8, 2, 2, 2, RegisterType::REG_C, RegisterType::REG_NONE, 4),
   new InstructionDef("BIT 4, D", 0x62, bit_imm_r8, 2, 2, 2, RegisterType::REG_D, RegisterType::REG_NONE, 4),
   new InstructionDef("BIT 4, E", 0x63, bit_imm_r8, 2, 2, 2, RegisterType::REG_E, RegisterType::REG_NONE, 4),
   new InstructionDef("BIT 4, H", 0x64, bit_imm_r8, 2, 2, 2, RegisterType::REG_H, RegisterType::REG_NONE, 4),
   new InstructionDef("BIT 4, L", 0x65, bit_imm_r8, 2, 2, 2, RegisterType::REG_L, RegisterType::REG_NONE, 4),
   new InstructionDef("BIT 4, (HL)", 0x66, bit_imm_m16, 2, 3, 3, RegisterType::REG_HL, RegisterType::REG_NONE, 4),
   new InstructionDef("BIT 4, A", 0x67, bit_imm_r8, 2, 2, 2, RegisterType::REG_A, RegisterType::REG_NONE, 4),

   // BIT 5
   new InstructionDef("BIT 5, B", 0x68, bit_imm_r8, 2, 2, 2, RegisterType::REG_B, RegisterType::REG_NONE, 5),
   new InstructionDef("BIT 5, C", 0x69, bit_imm_r8, 2, 2, 2, RegisterType::REG_C, RegisterType::REG_NONE, 5),
   new InstructionDef("BIT 5, D", 0x6A, bit_imm_r8, 2, 2, 2, RegisterType::REG_D, RegisterType::REG_NONE, 5),
   new InstructionDef("BIT 5, E", 0x6B, bit_imm_r8, 2, 2, 2, RegisterType::REG_E, RegisterType::REG_NONE, 5),
   new InstructionDef("BIT 5, H", 0x6C, bit_imm_r8, 2, 2, 2, RegisterType::REG_H, RegisterType::REG_NONE, 5),
   new InstructionDef("BIT 5, L", 0x6D, bit_imm_r8, 2, 2, 2, RegisterType::REG_L, RegisterType::REG_NONE, 5),
   new InstructionDef("BIT 5, (HL)", 0x6E, bit_imm_m16, 2, 3, 3, RegisterType::REG_HL, RegisterType::REG_NONE, 5),
   new InstructionDef("BIT 5, A", 0x6F, bit_imm_r8, 2, 2, 2, RegisterType::REG_A, RegisterType::REG_NONE, 5),

   // BIT 6
   new InstructionDef("BIT 6, B", 0x70, bit_imm_r8, 2, 2, 2, RegisterType::REG_B, RegisterType::REG_NONE, 6),
   new InstructionDef("BIT 6, C", 0x71, bit_imm_r8, 2, 2, 2, RegisterType::REG_C, RegisterType::REG_NONE, 6),
   new InstructionDef("BIT 6, D", 0x72, bit_imm_r8, 2, 2, 2, RegisterType::REG_D, RegisterType::REG_NONE, 6),
   new InstructionDef("BIT 6, E", 0x73, bit_imm_r8, 2, 2, 2, RegisterType::REG_E, RegisterType::REG_NONE, 6),
   new InstructionDef("BIT 6, H", 0x74, bit_imm_r8, 2, 2, 2, RegisterType::REG_H, RegisterType::REG_NONE, 6),
   new InstructionDef("BIT 6, L", 0x75, bit_imm_r8, 2, 2, 2, RegisterType::REG_L, RegisterType::REG_NONE, 6),
   new InstructionDef("BIT 6, (HL)", 0x76, bit_imm_m16, 2, 3, 3, RegisterType::REG_HL, RegisterType::REG_NONE, 6),
   new InstructionDef("BIT 6, A", 0x77, bit_imm_r8, 2, 2, 2, RegisterType::REG_A, RegisterType::REG_NONE, 6),

   // BIT 7
   new InstructionDef("BIT 7, B", 0x78, bit_imm_r8, 2, 2, 2, RegisterType::REG_B, RegisterType::REG_NONE, 7),
   new InstructionDef("BIT 7, C", 0x79, bit_imm_r8, 2, 2, 2, RegisterType::REG_C, RegisterType::REG_NONE, 7),
   new InstructionDef("BIT 7, D", 0x7A, bit_imm_r8, 2, 2, 2, RegisterType::REG_D, RegisterType::REG_NONE, 7),
   new InstructionDef("BIT 7, E", 0x7B, bit_imm_r8, 2, 2, 2, RegisterType::REG_E, RegisterType::REG_NONE, 7),
   new InstructionDef("BIT 7, H", 0x7C, bit_imm_r8, 2, 2, 2, RegisterType::REG_H, RegisterType::REG_NONE, 7),
   new InstructionDef("BIT 7, L", 0x7D, bit_imm_r8, 2, 2, 2, RegisterType::REG_L, RegisterType::REG_NONE, 7),
   new InstructionDef("BIT 7, (HL)", 0x7E, bit_imm_m16, 2, 3, 3, RegisterType::REG_HL, RegisterType::REG_NONE, 7),
   new InstructionDef("BIT 7, A", 0x7F, bit_imm_r8, 2, 2, 2, RegisterType::REG_A, RegisterType::REG_NONE, 7),


   new InstructionDef("RES 0, A", 0x87, res_bit_r8, 2, 2, 2, RegisterType::REG_A, RegisterType::REG_NONE, 0),
};

// TODO switch to a map
int InstructionSet::instruction_mappings[INSTRUCTION_SET_SIZE] = {};
int InstructionSet::prefix_instruction_mappings[INSTRUCTION_SET_SIZE] = {};

void InstructionSet::Initialize()
{
   std::fill_n(instruction_mappings, INSTRUCTION_SET_SIZE, -1);
   std::fill_n(prefix_instruction_mappings, INSTRUCTION_SET_SIZE, -1);

   int valid_instr = 0;
   for (int i = 0; i < INSTRUCTION_SET_SIZE; i++)
   {
      if (instructions[i] != nullptr)
      {
         valid_instr++;
         instruction_mappings[instructions[i]->opcode] = i;
      }
   }

   for (int i = 0; i < INSTRUCTION_SET_SIZE; i++)
   {
      if (prefix_instructions[i] != nullptr)
      {
         prefix_instruction_mappings[prefix_instructions[i]->opcode] = i;
      }
   }
}

InstructionDef* InstructionSet::Get(uint8_t opcode)
{
   int index = instruction_mappings[opcode];
   return index == -1 ? nullptr : instructions[index];
}

InstructionDef* InstructionSet::GetPrefixed(uint8_t opcode)
{
   int index = prefix_instruction_mappings[opcode];
   return index == -1 ? nullptr : prefix_instructions[index];
}
