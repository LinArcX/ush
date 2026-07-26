#ifndef USH_TERMINAL_H
#define USH_TERMINAL_H

#include "error.h"
#include <cstddef>
#include <sys/ioctl.h>
#include <termios.h>

namespace ush
{
  class Terminal
  {
    public:
      enum class EColorAttr : uint32_t {
        eBackground,
        eForeground
      };

      static Error requestGetTerminalWindowSize();

      static winsize& getTerminalWindowSize() { return m_ws; }

      static Error enableRawMode();

      static Error disableRawMode();

      static void startColor(EColorAttr attr, uint32_t r, uint32_t g, uint32_t b);

      static void endColor();

      static void writeSpace();

      static void writeNewLine();

      static void writeIcon(const char8_t* iconName);

      static void writeText(const char* name, size_t size);

    private:
      Terminal() = delete;

      ~Terminal() = delete;

      static winsize m_ws;

      static termios m_original;
  };
}
 
#endif // USH_TERMINAL_H
