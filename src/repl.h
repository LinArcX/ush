#ifndef USH_REPL_H
#define USH_REPL_H

#include <array>
#include <termios.h>
#include <string_view>

#include "error.h"

constexpr uint32_t maxArgs = 64;
constexpr uint32_t charsForLine = 1024;
constexpr uint32_t charsForArg = 64;
constexpr uint32_t builtinCommands = 8;

namespace ush
{
  class Repl
  {
    public:
      Repl();
      ~Repl();

      int loop(void);
      Error readLine(std::array<char, charsForLine>& line);
      Error splitArgs(const std::array<char, charsForLine>& chars,
        std::array<char[charsForArg], maxArgs>& args);
      Error execute(std::array<char[charsForArg], maxArgs>& args);
      Error launch(std::array<char[charsForArg], maxArgs>& args);

      Error cd(std::string_view arg);
      Error pwd(std::string_view arg);
      Error clearScreen(void);
      Error clearLine(void);
      Error help(void);
      Error exit(void);

      void enableRawMode();
      void disableRawMode();

    private:
      termios original;
      termios raw;
  };
}
#endif // USH_REPL_H
