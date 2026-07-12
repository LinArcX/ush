#ifndef USH_REPL_H
#define USH_REPL_H

#include <array>
#include <termios.h>

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

      [[nodiscard]] int loop(void);
      [[nodiscard]] Error readLine(std::array<char, charsForLine>& line);
      [[nodiscard]] Error splitArgs(const std::array<char, charsForLine>& chars,
        std::array<char[charsForArg], maxArgs>& args);
      [[nodiscard]] Error execute(std::array<char[charsForArg], maxArgs>& args);
      [[nodiscard]] Error launch(std::array<char[charsForArg], maxArgs>& args);

      [[nodiscard]] Error clearScreen(void);
      [[nodiscard]] Error clearLine(void);
      [[nodiscard]] Error help(void);
      [[nodiscard]] Error exit(void);

      void enableRawMode();
      void disableRawMode();

    private:
      termios original;
      termios raw;
      static void SIGINTHandler(int signal);
  };
}
#endif // USH_REPL_H
