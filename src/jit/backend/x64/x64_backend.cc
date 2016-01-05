#include <iomanip>
#include <sstream>
#include <beaengine/BeaEngine.h>
#include <xbyak/xbyak.h>
#include "core/core.h"
#include "emu/profiler.h"
#include "jit/backend/x64/x64_backend.h"
#include "sys/exception_handler.h"

using namespace dreavm;
using namespace dreavm::hw;
using namespace dreavm::jit;
using namespace dreavm::jit::backend;
using namespace dreavm::jit::backend::x64;
using namespace dreavm::jit::ir;
using namespace dreavm::sys;

namespace dreavm {
namespace jit {
namespace backend {
namespace x64 {
const Register x64_registers[] = {
    {"rbx", ir::VALUE_INT_MASK},     {"rbp", ir::VALUE_INT_MASK},
    {"r12", ir::VALUE_INT_MASK},     {"r13", ir::VALUE_INT_MASK},
    {"r14", ir::VALUE_INT_MASK},     {"r15", ir::VALUE_INT_MASK},
    {"xmm6", ir::VALUE_FLOAT_MASK},  {"xmm7", ir::VALUE_FLOAT_MASK},
    {"xmm8", ir::VALUE_FLOAT_MASK},  {"xmm9", ir::VALUE_FLOAT_MASK},
    {"xmm10", ir::VALUE_FLOAT_MASK}, {"xmm11", ir::VALUE_FLOAT_MASK}};

const int x64_num_registers = sizeof(x64_registers) / sizeof(Register);
}
}
}
}

X64Backend::X64Backend(Memory &memory)
    : Backend(memory), emitter_(memory, 1024 * 1024 * 8) {}

X64Backend::~X64Backend() {}

const Register *X64Backend::registers() const { return x64_registers; }

int X64Backend::num_registers() const {
  return sizeof(x64_registers) / sizeof(Register);
}

void X64Backend::Reset() { emitter_.Reset(); }

BlockPointer X64Backend::AssembleBlock(ir::IRBuilder &builder,
                                       void *guest_ctx) {
  // try to generate the x64 code. if the code buffer overflows let the backend
  // know so it can reset the cache and try again
  X64Fn fn;
  try {
    fn = emitter_.Emit(builder, guest_ctx);
  } catch (const Xbyak::Error &e) {
    if (e == Xbyak::ERR_CODE_IS_TOO_BIG) {
      return nullptr;
    }
    LOG_FATAL("X64 codegen failure, %s", e.what());
  }
  return reinterpret_cast<BlockPointer>(fn);
}

void X64Backend::DumpBlock(BlockPointer block) {
  DISASM dsm;
  memset(&dsm, 0, sizeof(dsm));
  dsm.Archi = 64;
  dsm.EIP = (uintptr_t)block;
  dsm.SecurityBlock = 0;
  dsm.Options = NasmSyntax | PrefixedNumeral;

  while (true) {
    int len = Disasm(&dsm);
    if (len == OUT_OF_BLOCK) {
      LOG_INFO("Disasm engine is not allowed to read more memory");
      break;
    } else if (len == UNKNOWN_OPCODE) {
      LOG_INFO("Unknown opcode");
      break;
    }

    // format instruction binary
    static const int MAX_INSTR_LENGTH = 15;
    std::stringstream instr;
    for (int i = 0; i < MAX_INSTR_LENGTH; i++) {
      uint32_t v =
          i < len ? (uint32_t) * reinterpret_cast<uint8_t *>(dsm.EIP + i) : 0;
      instr << std::hex << std::setw(2) << std::setfill('0') << v;
    }

    // print out binary / mnemonic
    LOG_INFO("%s %s", instr.str().c_str(), dsm.CompleteInstr);

    if (dsm.Instruction.BranchType == RetType) {
      break;
    }

    dsm.EIP = dsm.EIP + len;
  }
}

bool X64Backend::HandleException(Exception &ex) {
  size_t original_size = emitter_.getSize();
  size_t offset =
      ex.thread_state.rip - reinterpret_cast<uint64_t>(emitter_.getCode());
  emitter_.setSize(offset);

  // nop out the mov
  uint8_t *ptr = reinterpret_cast<uint8_t *>(ex.thread_state.rip);
  while (*ptr != 0xeb) {
    emitter_.nop();
    ptr++;
  }

  // nop out the near jmp
  emitter_.nop();
  emitter_.nop();

  emitter_.setSize(original_size);

  return true;
}
