#ifndef USH_ERROR_H
#define USH_ERROR_H

#include <cstdint>

namespace ush
{
  enum class Error : uint32_t
  {
    eUnknown = 13,
    eError = 21,
    eSuccess = 0,
    eExit = 89,
    eClearScreen = 1,
    eClearLine = 2,
  };
}

#endif // USH_ERROR_H
