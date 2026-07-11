#ifndef USH_REPL_H
#define USH_REPL_H

#include <array>
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
      int loop(void);

      Error readLine(std::array<char, charsForLine>& line);
      Error splitArgs(const std::array<char, charsForLine>& chars,
        std::array<char[charsForArg], maxArgs>& args);
        //std::array<char[charsForArg], maxArgs> args);
      Error execute(std::array<char[charsForArg], maxArgs>& args);
      Error launch(std::array<char[charsForArg], maxArgs>& args);

      Error cd(std::string_view arg);
      Error pwd(std::string_view arg);
      Error clear(void);
      Error help(void);
      Error exit(void);

    private:

  };
}
#endif // USH_REPL_H
