#ifndef USH_REPL_H
#define USH_REPL_H

#include <array>
#include <termios.h>
#include <filesystem>
#include <string_view>
#include <vector>
#include <sys/ioctl.h>

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

      void clearScreen(void);
      void clearLine(void);
      void resetLineVarsShowPrompt(void);
      [[nodiscard]] Error cd(void);
      [[nodiscard]] Error help(void);
      [[nodiscard]] Error exit(void);

    private:
      char c;
      uint32_t m_charPosition = 0U;
      uint32_t m_cursorPosition = 0U;

      winsize m_ws{};
      termios m_raw;
      termios m_original;
 
      std::vector<std::string> m_dirsHistory;
      std::vector<std::string> m_commandsHistory;
      std::array<char, charsForLine> m_chars {};
      std::array<char[charsForArg], maxArgs> m_args {};

      bool m_inCommandHistoryTravelMode = true;
	    uint32_t m_inCommandHistoryLastIndexVisited = 0U;
	    bool m_inDirHistoryTravelMode = true;
	    uint32_t m_inDirHistoryLastIndexVisited = 0U;

      void enableRawMode();
      void disableRawMode();

      void moveBackToFirstCharOfWord();

      void moveBackToFirstNonSpaceChar();

      void moveForwardToFirstNonSpaceChar();

      void moveForwardToFirstSpaceAfterCurrentWord();

      void showElns(std::string path);
 
      bool lineIsEmpty();
      bool saveFile(std::filesystem::path path,
          std::string_view file,
          std::string_view text);
      void saveCommandHistory(std::string str);
      void saveDirectoryHistory(std::string str);

      bool readFile(const std::filesystem::path& path,
        std::vector<std::string>& vec);
      void readCommandHistory();
      void readDirectoryHistory();

      static void SIGINTHandler(int signal);
  };
}
#endif // USH_REPL_H
