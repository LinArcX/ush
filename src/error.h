#include <cstdint>

enum class Error : uint32_t {
  eSuccess,
  eClear,
  eError,
  eExit,
};
