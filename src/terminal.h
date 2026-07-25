#ifndef USH_TERMINAL_H
#define USH_TERMINAL_H

#include "error.h"
#include <sys/ioctl.h>
#include <termios.h>

namespace ush
{
  class Terminal
  {
    public:
      Error requestGetTerminalWindowSize();
      winsize& getTerminalWindowSize() { return m_ws; }

      Error enableRawMode();
      Error disableRawMode();

    private:
      winsize m_ws;
      termios m_original;
  };
}
 
#endif // USH_TERMINAL_H
