#ifndef PROFILER_H
#define PROFILER_H

#include <microprofile.h>
#include "renderer/backend.h"
#include "sys/keycode.h"

#define PROFILER_SCOPE(group, name) \
  MICROPROFILE_SCOPEI(group, name, dreavm::emu::Profiler::ScopeColor(name))

#define PROFILER_GPU(name) \
  MICROPROFILE_SCOPEI("gpu", name, dreavm::emu::Profiler::ScopeColor(name))

#define PROFILER_RUNTIME(name) \
  MICROPROFILE_SCOPEI("runtime", name, dreavm::emu::Profiler::ScopeColor(name))

#define PROFILER_COUNT(name, count) MICROPROFILE_COUNTER_SET(name, count)

namespace dreavm {
namespace emu {

class Profiler {
 public:
  static Profiler &instance();
  static uint32_t ScopeColor(const char *name);

  Profiler();

  bool HandleInput(sys::Keycode key, int16_t value);
  bool HandleMouseMove(int x, int y);
  void Render(renderer::Backend *backend);
};
}
}

#endif
