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
      enum class EElnAttr : uint32_t {
        eBackground,
        eForeground
      };

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
      // nerd-fonts
      const char8_t* m_text = u8"\uf15c";
      const char8_t* m_json = u8"\ueb0f";
      const char8_t* m_html = u8"\ue60e";
      const char8_t* m_css = u8"\ue6b8";
      const char8_t* m_js = u8"\ue781";
      const char8_t* m_csv = u8"\ueefc";
      const char8_t* m_xml = u8"\ue8ea";
      const char8_t* m_cpp = u8"\ue61d";
      const char8_t* m_c = u8"\ue61e";
      const char8_t* m_python = u8"\ue73c";
      const char8_t* m_ruby = u8"\ue739";
      const char8_t* m_bash = u8"\ue760";
      const char8_t* m_font = u8"\uf031";
      const char8_t* m_zip = u8"\ue6aa";
      const char8_t* m_rust = u8"\ue7a8";
      const char8_t* m_nim = u8"\ue841";
      const char8_t* m_go = u8"\ue627";
      const char8_t* m_zig = u8"\ue8ef";
      const char8_t* m_java = u8"\ue738";
      const char8_t* m_yaml = u8"\ue8eb";
      const char8_t* m_toml = u8"\ue6b2";
      const char8_t* m_markdown = u8"\uf48a";
      const char8_t* m_audio = u8"\uec1b";
      const char8_t* m_video = u8"\uf1c8";
      const char8_t* m_image = u8"\uf03e";
      const char8_t* m_pdf = u8"\uf1c1";
      const char8_t* m_word = u8"\ue6a5";
      const char8_t* m_excel = u8"\uf1c3";
      const char8_t* m_powerpoint = u8"\uf1c4";
      const char8_t* m_application = u8"\uf0be";
      const char8_t* m_binary = u8"\ueae8";
      const char8_t* m_library = u8"\ueb9c";
      const char8_t* m_file = u8"\uf15b";
      const char8_t* m_folder = u8"\uf07b";
      const char8_t* m_debug = u8"\uead8";
      
      
        
      char c;
      uint32_t m_elnNumber = 1U;
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

      void drawElnNode(const char* name,
          size_t size,
          const char8_t* iconName,
          EElnAttr attr,
          uint32_t r, uint32_t g, uint32_t b);

      static void SIGINTHandler(int signal);

      /**
       * @brief detect the type of a file
       *
       * @param fileName IN
       * @param type OUT argument which contains one of these:
       *      0 -> unknown
       *      1 -> image,
       *      2 -> video
       *      3 -> audio
       *      4 -> text
       *      5 -> application
       * @return true if it can detect the file type, otherwise false.
       */
      //bool getFileType(const char * const fileName,
      //                int8_t* type);
      /**
       * @brief detect the extension of a file
       *
       * @param fileName IN
       * @param extName OUT
       * @param extNameSize IN
       * @return true if it can detect the file extension, otherwise false.
       */
      //bool getExtensionOfFile(const char * const fileName,
      //                        char extName[],
      //                        uint32_t extNameSize);
  };
}
#endif // USH_REPL_H
