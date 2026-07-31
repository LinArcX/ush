#ifndef USH_REPL_H
#define USH_REPL_H

#include "box.h"
#include "error.h"
#include "terminal.h"

#include <array>
#include <string>

constexpr uint32_t maxArgs = 64;
constexpr uint32_t charsForLine = 1024;
constexpr uint32_t charsForArg = 64;
constexpr uint32_t builtinCommands = 8;

namespace ush
{
  class Repl
  {
    public:
      Error init();

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
       * @return Error::eSuccess when press Enter
       */
      [[nodiscard]] Error handleEventsAndPopulateChars(void);

      /**
       * @brief parse characters, extract commands and args from it
       *
       * @example 
       *                      ls     -l     
       *        - any space before first charachter should be discard.
       *        - any space between first word and next word should be discard also.
       *          - but we need to increment arguments by one.
       * @return Error::eSuccess if parsing is ok 
       */
      [[nodiscard]] Error parseCharsAndPopulateCommandsArgs(void);

      [[nodiscard]] Error execute(void);

      [[nodiscard]] Error launchBinary(void);

      Error clearRepl(void);

      Error clearLine(void);

      void resetLineVarsShowPrompt(void);

      [[nodiscard]] Error cd(void);

      [[nodiscard]] Error help(void);

      [[nodiscard]] Error exit(void);

    private:
      char c;

      uint32_t m_elnNumber = 1U;

      uint32_t m_charPosition = 1U;

      uint32_t m_cursorPosition = 1U;
 
      Box m_pwdBox;

      Box m_replBox;

      Box m_gitBox;
 
      std::array<char, charsForLine> m_chars {};

      std::array<char[charsForArg], maxArgs> m_args {};

	    bool m_inDirHistoryTravelMode = true;

      bool m_inCommandHistoryTravelMode = true;

      // eln
      void showElns(std::string path);

      void drawElnNode(const char* name,
          size_t size,
          const char8_t* iconName,
          Terminal::EColorAttr attr,
          uint32_t r, uint32_t g, uint32_t b);

      // repl
      static void SIGINTHandler(int signal);

      bool lineIsEmpty();

      void moveBackToFirstCharOfWord();

      void moveBackToFirstNonSpaceChar();

      void moveForwardToFirstNonSpaceChar();

      void moveForwardToFirstSpaceAfterCurrentWord();
  };
}
#endif // USH_REPL_H
