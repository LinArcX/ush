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

      /**
       * @brief REPL of ush. during this loop, you can:
       * 1. send keyboard events
       * 2. type a command and execute it
       * 3. repeate this process
       *
       * @return 0 if Error::eExit fires
       */
      [[nodiscard]] int loop(void);

      /**
       * @brief 1. handle keyboard events like Ctrl-l (clear screen), Ctrl-u (clear line)
       *        2. populate input characters and make them ready for prepareCommandAndArgs()
       *
       * @param chars
       * @return Error::eSuccess when press Enter
       */
      [[nodiscard]] Error handleEventsAndPopulateChars(std::array<char, charsForLine>& chars);

      /**
       * @brief parse characters, extract commands and args from it
       *
       * @example 
       *                      ls     -l     
       *        - any space before first charachter should be discard.
       *        - any space between first word and next word should be discard also.
       *          - but we need to increment arguments by one.
       * @param chars an array of all input characters comes from handleEventsAndPopulateChars()
       * @param args an array contains command and args
       * @return Error::eSuccess if parsing is ok 
       */
      [[nodiscard]] Error parseCharsAndPopulateCommandsArgs(const std::array<char, charsForLine>& chars,
        std::array<char[charsForArg], maxArgs>& args);

      [[nodiscard]] Error execute(std::array<char[charsForArg], maxArgs>& args);
      [[nodiscard]] Error launchBinary(std::array<char[charsForArg], maxArgs>& args);

      [[nodiscard]] Error clearScreen(void);
      [[nodiscard]] Error clearLine(void);
      [[nodiscard]] Error help(void);
      [[nodiscard]] Error exit(void);

    private:
      termios original;
      termios raw;

      void enableRawMode();
      void disableRawMode();

      static void SIGINTHandler(int signal);
  };
}
#endif // USH_REPL_H
