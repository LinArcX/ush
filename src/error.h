#ifndef USH_ERROR_H
#define USH_ERROR_H

#include <cstdint>

namespace ush
{
  enum class Error : uint32_t
  {
    eSuccess,
    eClear,
    eError,
    eExit,
  };
}

#endif // USH_ERROR_H
