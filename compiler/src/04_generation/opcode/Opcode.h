#pragma once

namespace nugdev::compiler::generation {

class Register;

class Opcode {};

// Rx = Ry
class Move : public Opcode {};

// Rx = null
class LoadNull : public Opcode {};

// Rx = immediate integer
class LoadInt : public Opcode {};

// Rx = immediate float
class LoadFloat : public Opcode {};

// Rx = Memory[Ry]
class Load : public Opcode {};

// Memory[Rx] = Ry
class Store : public Opcode {};

// Rx = alloc(Ry) - Ry는 크기
class Allocate : public Opcode {};

// free(Rx)
class Free : public Opcode {};

// Rx = Rx + Ry
class Add : public Opcode {};

// Rx = Rx - Ry
class Sub : public Opcode {};

// Rx = Rx * Ry
class Mul : public Opcode {};

// Rx = Rx / Ry
class Div : public Opcode {};

// Rx = Rx % Ry
class Mod : public Opcode {};

// Rx = Rx & Ry
class And : public Opcode {};

// Rx = Rx | Ry
class Or : public Opcode {};

// Rx = Rx ^ Ry
class Xor : public Opcode {};

// Rx = ~Rx
class Not : public Opcode {};

// Rx = Rx == Ry
class Eq : public Opcode {};

// Rx = Rx != Ry
class Neq : public Opcode {};

// Rx > Ry
class Gt : public Opcode {};

// Rx >= Ry
class Gte : public Opcode {};

// Rx < Ry
class Lt : public Opcode {};

// Rx <= Ry
class Lte : public Opcode {};

// pc = pc + Ry
class Jump : public Opcode {};

// if Rx == 0: PC = address
class Jumpz : public Opcode {};

// if Rx != 0: PC = address
class Jumpnz : public Opcode {};

// Call function at address
class Call : public Opcode {};

// Return from function
class Return : public Opcode {};

// Halt execution
class Halt : public Opcode {};

} // namespace nugdev::compiler::generation
