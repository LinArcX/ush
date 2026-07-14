#ifndef USH_ERROR_H
#define USH_ERROR_H

#include <cstdint>

namespace ush
{
  enum class Error : uint32_t
  {
    eSuccess,
    eError,
    eExit,
    eUnknown,
    eClearScreen,
    eClearLine,
  };
}

#endif // USH_ERROR_H
