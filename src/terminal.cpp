#include "terminal.h"
#include <unistd.h>

ush::Error ush::Terminal::Terminal::requestGetTerminalWindowSize()
{
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &m_ws) != -1 ) {
    return Error::eSuccess;
  }
  return Error::eError;
}

ush::Error ush::Terminal::enableRawMode()
{
  termios m_raw;
  tcgetattr(STDIN_FILENO, &m_original);

  m_raw = m_original;
  //raw.c_lflag &= ~(ICANON | ECHO | ECHOCTL); // IEXTEN | ISIG
  m_raw.c_lflag &= ~(ICANON | ECHO | IEXTEN | ISIG);
  //raw.c_iflag &= ~(IXON | ICRNL); // BRKINT | INPCK | ISTRIP
  m_raw.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
  // raw.c_oflag &= ~(OPOST);
  m_raw.c_cflag |= CS8;

  m_raw.c_cc[VMIN] = 1;
  m_raw.c_cc[VTIME] = 0;
  if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &m_raw) == 0) {
    return Error::eSuccess;
  }
  return Error::eError;
}

ush::Error ush::Terminal::disableRawMode()
{
  if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &m_original) == 0) {
    return Error::eSuccess;
  }
  return Error::eError;
}
