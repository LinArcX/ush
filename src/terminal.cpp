#include "terminal.h"

#include <unistd.h>
#include <format>
#include <string>
#include <string_view>

winsize ush::Terminal::m_ws = {};
termios ush::Terminal::m_original = {};

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

void ush::Terminal::startColor(EColorAttr attr,
  uint32_t r, uint32_t g, uint32_t b)
{
  std::string str;
  if (attr == EColorAttr::eForeground) {
    str = std::format("\033[38;2;{};{};{}m", r, g, b);
  }
  else {
    str = std::format("\033[48;2;{};{};{}m", r, g, b);
  }
  writeText(str.data(), str.size());
  //write(STDOUT_FILENO, str.data(), str.size());
}

void ush::Terminal::endColor()
{
  write(STDOUT_FILENO, "\033[0m", 4);
}

void ush::Terminal::makeNewLine()
{
  write(STDOUT_FILENO, "\n", 1);
}

void ush::Terminal::goTostartOfLine()
{
  write(STDOUT_FILENO, "\r", 1);
}

void ush::Terminal::writeSpace()
{
  write(STDOUT_FILENO, " ", 1);
}

void ush::Terminal::writeIcon(const char8_t* iconName)
{
  write(STDOUT_FILENO, reinterpret_cast<const char*>(iconName),
      std::char_traits<char8_t>::length(iconName));
}

void ush::Terminal::writeChar(const char* ch)
{
  write(STDOUT_FILENO, ch, 1);
}

void ush::Terminal::writeText(const char* name, size_t size)
{
  write(STDOUT_FILENO, name, size);
}

void ush::Terminal::eraseEntireLine()
{
  //constexpr char clear_seq[] = "\x1b[3J\x1b[2J\x1b[H";
  //constexpr char clear_seq[] = "\x1b[2k";
  //write(STDOUT_FILENO, clear_seq, sizeof(clear_seq) - 1);
  std::string str = "\x1b[2K";
  writeText(str.data(), str.size());
}

void ush::Terminal::moveCursorToLineColumn(uint32_t row, uint32_t col)
{
  std::string str =
    "\033[" +
    std::to_string(row) +
    ";" +
    std::to_string(col) +
    "H";

  writeText(str.data(), str.size());
  //write(STDOUT_FILENO, str.data(), str.size())
}
