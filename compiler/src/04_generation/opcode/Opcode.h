#pragma once

#include "Register.h"

namespace nugdev::compiler::generation {

class Opcode {};

namespace Code {

// Rx = Ry
class Move : public Opcode {

  private:
    RegisterTag rx;
    RegisterTag ry;
};

// Rx = null
class LoadNull : public Opcode {

  private:
    RegisterTag rx;
};

// Rx = immediate integer
class LoadInt : public Opcode {

  private:
    RegisterTag rx;
    RegisterData value;
};

// Rx = immediate float
class LoadFloat : public Opcode {

  private:
    RegisterTag rx;
    RegisterData value;
};

// Rx = Memory[Ry]
class Load : public Opcode {

  private:
    RegisterTag rx;
    RegisterTag ry;
};

// Memory[Rx] = Ry
class Store : public Opcode {

  private:
    RegisterTag rx;
    RegisterTag ry;
};

// Rx = alloc(Ry) - Ry는 크기
class Allocate : public Opcode {

  private:
    RegisterTag rx;
    RegisterTag ry;
};

// free(Rx)
class Free : public Opcode {

  private:
    RegisterTag rx;
};

// Rx = Rx + Ry
class Add : public Opcode {

  private:
    RegisterTag rx;
    RegisterTag ry;
};

// Rx = Rx - Ry
class Sub : public Opcode {

  private:
    RegisterTag rx;
    RegisterTag ry;
};

// Rx = Rx * Ry
class Mul : public Opcode {

  private:
    RegisterTag rx;
    RegisterTag ry;
};

// Rx = Rx / Ry
class Div : public Opcode {

  private:
    RegisterTag rx;
    RegisterTag ry;
};

// Rx = Rx % Ry
class Mod : public Opcode {

  private:
    RegisterTag rx;
    RegisterTag ry;
};

// Rx = Rx & Ry
class And : public Opcode {

  private:
    RegisterTag rx;
    RegisterTag ry;
};

// Rx = Rx ^ Ry
class Xor : public Opcode {

  private:
    RegisterTag rx;
    RegisterTag ry;
};

// Rx = ~Rx
class Not : public Opcode {

  private:
    RegisterTag rx;
};

// Rx = Rx == Ry
class Eq : public Opcode {

  private:
    RegisterTag rx;
    RegisterTag ry;
};

// Rx = Rx != Ry
class Neq : public Opcode {

  private:
    RegisterTag rx;
    RegisterTag ry;
};

// Rx > Ry
class Gt : public Opcode {

  private:
    RegisterTag rx;
    RegisterTag ry;
};

// Rx >= Ry
class Gte : public Opcode {

  private:
    RegisterTag rx;
    RegisterTag ry;
};

// Rx < Ry
class Lte : public Opcode {

  private:
    RegisterTag rx;
    RegisterTag ry;
};

// pc = pc + Ry
class Jump : public Opcode {

  private:
    RegisterTag ry;
};

// if Rx == 0: PC = address
class Jumpz : public Opcode {

  private:
    RegisterTag rx;
};

// if Rx != 0: PC = address
class Jumpnz : public Opcode {

  private:
    RegisterTag rx;
};

// Call function at address
class Call : public Opcode {

  private:
    RegisterTag ry;
};

// Return from function
class Return : public Opcode {

  private:
    RegisterTag ry;
};

// Halt execution
class Halt : public Opcode {};
} // namespace Code

} // namespace nugdev::compiler::generation
