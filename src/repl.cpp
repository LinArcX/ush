#include "repl.h"
#include "icons.h"
#include "file.h"

#include <print>
#include <stdio.h>
#include <unistd.h>
#include <algorithm>
#include <magic.h>

#ifdef __linux__
#include <csignal>
#include <sys/wait.h>
#include <sys/types.h>
#elif __windows__ || defined (WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#include <Windows.h>
#endif

// static
void ush::Repl::SIGINTHandler(int signal)
{
  Terminal::goTostartOfLine();
  Terminal::makeNewLine();
  //write(STDOUT_FILENO, "\r\n", 2);
}

// public
ush::Error ush::Repl::init()
{
  if (Error::eSuccess != Terminal::enableRawMode()) {
    return Error::eError;
  }

  // SIGINIT is disabled in ush main process, and just child process are allowed to have SIGINIT.
  // When it happens in a child-process, we exit from it and we just go to next line ready for another command in ush.
  std::signal(SIGINT, ush::Repl::SIGINTHandler);

  File::readCommandHistory();
  File::readDirectoryHistory();

  // 1 = top border, 1 = bottom border, 1 = content
  if (Error::eSuccess != m_pwdBox.drawBorder(1, 1, Terminal::getTerminalWindowSize().ws_col, 3)) {
    return Error::eError;
  } 

  //if (Error::eSuccess !=  m_replBox.draw(1, m_pwdBox.m_height, Terminal::getTerminalWindowSize().ws_col, 
  //    Terminal::getTerminalWindowSize().ws_row - (m_pwdBox.m_height + m_gitBox.m_height))) {
  //  return Error::eError;
  //}

  //if (Error::eSuccess != m_gitBox.draw(1, m_replBox.m_height, Terminal::getTerminalWindowSize().ws_col, 3)) {
  //  return Error::eError;
  //}

  return Error::eSuccess;
}

ush::Repl::~Repl()
{
  Terminal::disableRawMode();
}

int ush::Repl::loop(void)
{
  //m_replBox.moveCursor(m_replBox.m_col, m_replBox.m_row);
  //if (clearRepl() != Error::eSuccess) {
  //  return -1;
  //}
  //showElns(std::filesystem::current_path());

  //while(true) {
  //  // reset arrays
  //  m_chars = {};
  //  m_args = {};

  //  Error e = handleEventsAndPopulateChars();
  //  if (e != Error::eSuccess) {
  //    continue;
  //  }

  //  e = parseCharsAndPopulateCommandsArgs();
  //  if (e != Error::eSuccess) {
  //    continue;
  //  }
 
  //  e = execute();
  //  if (Error::eExit == e) {
  //    return 0;
  //  }
  //}
  return 0;
}

ush::Error ush::Repl::handleEventsAndPopulateChars()
{
	resetLineVarsShowPrompt();
  while (read(STDIN_FILENO, &c, 1) == 1) {
    // Ctrl-a: beginning of line
    if (c == 1) {
      while (m_cursorPosition > 0) {
        Terminal::moveCursorLeftOneChar();
        //write(STDOUT_FILENO, "\x1b[D", 3);
        m_cursorPosition--;
      }
      continue;
    }

    // Ctrl-e: end of line
    if (c == 5) {
      while (m_cursorPosition < m_charPosition) {
        Terminal::moveCursorRightOneChar();
        //write(STDOUT_FILENO, "\x1b[C", 3);
        m_cursorPosition++;
      }
      continue;
    }

    // Ctrl-l
    if (c == 12) {
      clearRepl();
      showElns(std::filesystem::current_path());
      resetLineVarsShowPrompt();
      continue;
    }

    // Ctrl-u
    if (c == 21) { 
      clearLine();
      continue;
    }

    // BakcSpace ('\b' or 127)
    if (c == 127) {
      if (m_charPosition > 0) {
        --m_cursorPosition;
        Terminal::removePrevCharAndMoveCursorToLeft();

        --m_charPosition;
        m_chars[m_charPosition] = '\0';

        if (lineIsEmpty()) {
			    m_chars[m_charPosition] = '\0';
          m_inDirHistoryTravelMode = true;
          m_inCommandHistoryTravelMode = true;
		      continue;
		    }
      }
      continue;
    }

    if (c == 27 ) { // ESC or \x1b
      char seq[1];

      if (read(STDIN_FILENO, &seq[0], 1) != 1)
        continue;

      // Alt+j - next dir history
      if (seq[0] == 'j') {
        if (m_inDirHistoryTravelMode == true) {
          if (File::m_inDirHistoryLastIndexVisited < File::m_dirsHistory.size()) {
            std::string item = File::m_dirsHistory[++File::m_inDirHistoryLastIndexVisited];
            size_t itemSize = item.size();
            if (itemSize > 0) {
              clearLine();
            }
            for (size_t i = 0; i < itemSize; i++) {
              Terminal::writeChar(&item[i]);
              //write(STDOUT_FILENO, &item[i], 1);
              m_chars[m_cursorPosition] = item[i];
              m_cursorPosition++;
              m_charPosition++;
            }
          }
        }
        continue;
      }

      // Alt+k - previous dir history
      if (seq[0] == 'k') {
        if (m_inDirHistoryTravelMode == true) {
          if (File::m_inDirHistoryLastIndexVisited > 0) {
            std::string item = File::m_dirsHistory[--File::m_inDirHistoryLastIndexVisited];
            size_t itemSize = item.size();
            if (itemSize > 0) {
              clearLine();
            }
            for (size_t i = 0; i < itemSize; i++) {
              Terminal::writeChar(&item[i]);
              //write(STDOUT_FILENO, &item[i], 1);
              m_chars[m_cursorPosition] = item[i];
              m_cursorPosition++;
              m_charPosition++;
            }
          }
        }
        continue;
      }

      if (seq[0] == '[') {
        // Extended sequence: read more bytes here
        char extSeq1[1];

        if (read(STDIN_FILENO, &extSeq1[0], 1) != 1)
          continue;
 
        // Up - previous history
        if (extSeq1[0] == 'A') {
          if (m_inCommandHistoryTravelMode == true) {
            if (File::m_inCommandHistoryLastIndexVisited > 0) {
              std::string item = File::m_commandsHistory[--File::m_inCommandHistoryLastIndexVisited];
              size_t itemSize = item.size();
              if (itemSize > 0) {
                clearLine();
              }
              for (size_t i = 0; i < item.size(); i++) {
                Terminal::writeChar(&item[i]);
                //write(STDOUT_FILENO, &item[i], 1);
                m_chars[m_cursorPosition] = item[i];
                m_cursorPosition++;
                m_charPosition++;
              }
            }
          }
          continue;
        }

        // Down - next history
        if (extSeq1[0] == 'B') {
          if (m_inCommandHistoryTravelMode == true) {
            if (File::m_inCommandHistoryLastIndexVisited < File::m_commandsHistory.size()) {
              std::string item = File::m_commandsHistory[++File::m_inCommandHistoryLastIndexVisited];
              size_t itemSize = item.size();
              if (itemSize > 0) {
                clearLine();
              }
              for (size_t i = 0; i < itemSize; i++) {
                Terminal::writeChar(&item[i]);
                //write(STDOUT_FILENO, &item[i], 1);
                m_chars[m_cursorPosition] = item[i];
                m_cursorPosition++;
                m_charPosition++;
              }
            }
            //std::string item = m_commandsHistory.front();
            //for (size_t i = 0; i < item.size(); i++) {
            //  write(STDOUT_FILENO, &item[i], 1);
            //  m_cursorPosition++;
            //  m_charPosition++;
            //}
          }
          continue;
        }

        // Right - next char
        if (extSeq1[0] == 'C') {
          if (m_cursorPosition < m_charPosition) {
            Terminal::moveCursorRightOneChar();
            //write(STDOUT_FILENO, "\x1b[C", 3);
            m_cursorPosition++;
            m_inCommandHistoryTravelMode = false;
            m_inDirHistoryTravelMode = false;
          }
          continue;
        }

        // Left - previous char
        if (extSeq1[0] == 'D') {
          if (m_cursorPosition > 0) {
            Terminal::moveCursorRightOneChar();
            m_cursorPosition--;
            m_inCommandHistoryTravelMode = false;
            m_inDirHistoryTravelMode = false;
          }
          continue;
        }
        if (extSeq1[0] == '1') {
          // Extended sequence: read more bytes here
          char extSeq2[3];

          if (read(STDIN_FILENO, &extSeq2[0], 1) != 1)
            continue;

          if (read(STDIN_FILENO, &extSeq2[1], 1) != 1)
            continue;

          if (read(STDIN_FILENO, &extSeq2[2], 1) != 1)
            continue;

          // Ctrl+Up
          if (extSeq2[2] == 'A') {
            if (m_inDirHistoryTravelMode == true) {
              Terminal::writeChar("u");
              //write(STDOUT_FILENO, "u", 1);
            }
 
            continue;
          }

          // Ctrl+Down
          if (extSeq2[2] == 'B') {
            Terminal::writeChar("d");
            //write(STDOUT_FILENO, "d", 1);
            continue;
          }
 
          // Scenario 1:
          //     this          is    the       best       world
          //                          ^
          //     this          is    the       best       world
          //                            ^
          // Scenario 2:
          //     this          is    the       best       world
          //                               ^
          //     this          is    the       best       world
          //                                       ^
          //  Note: when press Ctrl+right, ^ moves to the first space after current word or
          //        if ^ it's on a space position, it moves to the first space character after next word
          // Ctrl+Right - next word
          if (extSeq2[2] == 'C') {
            // Scenario 1
            if (m_chars[m_cursorPosition] != 32) {  // 32 is SPACE
              moveForwardToFirstSpaceAfterCurrentWord();
            }
            // Scenario 2
            else {
              moveForwardToFirstNonSpaceChar();
              moveForwardToFirstSpaceAfterCurrentWord();
            }
            continue;
          }
 
          // Scenario 1:
          //     this          is    the       best       world
          //                          ^
          //     this          is    the       best       world
          //                         ^
          // Scenario 2:
          //     this          is    the       best       world
          //                      ^
          //     this          is    the       best       world
          //                   ^
          //  Note: when press Ctrl+left, ^ moves to the first charachter of current word or
          //
          //        if ^ it's on a space position, it moves to the first character of previous word
          // Ctrl+Left - previous word
          if (extSeq2[2] == 'D') {
            // Scenario 1
            if (m_chars[m_cursorPosition] != 32) {  // 32 is SPACE
              moveBackToFirstCharOfWord();
            }

            // Scenario 2
            else {
              moveBackToFirstNonSpaceChar();
              moveBackToFirstCharOfWord();
            }
            continue;
          }
        }
      }
    }

		// If we hit EOF, replace it with a null character and return.
		if (c == '\r' || c == '\n') {
      Terminal::goTostartOfLine();
      Terminal::makeNewLine();
			//write(STDOUT_FILENO, "\r\n", 2);
		  if (lineIsEmpty()) {
		    resetLineVarsShowPrompt();
		    continue;
		  }

			m_chars[m_charPosition] = '\0';
		  m_cursorPosition = 0;
      m_inDirHistoryTravelMode = true;
      m_inCommandHistoryTravelMode = true;

      File::readCommandHistory();
      File::readDirectoryHistory();
			return Error::eSuccess;
		} else {
		  // this is when you start to move cursor back and foth to put space/chars
		  if (m_cursorPosition < m_charPosition) {
        for (std::size_t i = m_charPosition; i > m_cursorPosition; --i) {
            m_chars[i] = m_chars[i - 1];
        }
        m_chars[m_cursorPosition] = c;
        ++m_cursorPosition;
        ++m_charPosition;
        m_chars[m_charPosition] = '\0';
        Terminal::goTostartOfLine();
        //write(STDOUT_FILENO, "\r", 1);
        Terminal::eraseEntireLine();
        //write(STDOUT_FILENO, "\x1b[2K", 4); // Clear line
        std::string str = " > ";
        Terminal::writeText(str.data(), str.size());
        //write(STDOUT_FILENO, " > ", 3);
        Terminal::writeText(m_chars.data(), m_charPosition);
        //write(STDOUT_FILENO, m_chars.data(), m_charPosition);

        char buf[32];
        int n = std::snprintf(buf, sizeof(buf), "\r\x1b[%uC",
                      static_cast<unsigned>(3 + m_cursorPosition)); // 3 = prompt length
        Terminal::writeText(buf, n);
        //write(STDOUT_FILENO, buf, n);
		  }
		  // this is the normal path, as you type, you move forward. it include chars and SPACE
		  else {
		    m_cursorPosition++;

			  m_chars[m_charPosition] = c;
		    m_charPosition++;
        Terminal::writeChar(&c);
		    //write(STDOUT_FILENO, &c, 1);
		  }
      m_inCommandHistoryTravelMode = false;
      m_inDirHistoryTravelMode = false; 
		}

		// If we have exceeded the buffer, we just clear the line
		if (m_charPosition >= charsForLine) {
		  clearLine();
		  return Error::eError;
		}
  }
  // unknow error
  clearLine();
  return Error::eUnknown;
}

ush::Error ush::Repl::parseCharsAndPopulateCommandsArgs()
{
  bool seenChar = false;
  uint32_t currentArg = 0U;

  for (size_t i = 0, j = 0; i < charsForArg; i++) {
    char currentChar = m_chars[i];

    if (currentChar == '\0') {
      return Error::eSuccess;
    }
    if (isspace(currentChar)) {
      if (seenChar == true) {
        currentArg++;
        j = 0;
        seenChar = false;
      }
    }
    else if (isalnum(currentChar) 
        || currentChar == '-' 
        || currentChar == '~' 
        || currentChar == '.'
        || currentChar == '/') {
      seenChar = true;
      m_args[currentArg][j++] = currentChar;
    }
  }
  return Error::eError;
}

ush::Error ush::Repl::execute()
{
  // search in builtin commands first
  if (std::string_view(m_args[0]) == std::string_view("clear")) {
	  clearRepl();
  }
  else if (std::string_view(m_args[0]) == std::string_view("cd")) {
	  return cd();
  }
  else if (std::string_view(m_args[0]) == std::string_view("help")) {
    return help();
  }
  else if (std::string_view(m_args[0]) == std::string_view("exit")) {
    return exit();
  }
  else {
    // it's not a builtint. let's launch it as a separate process.
    return launchBinary();
  }
  return Error::eError;
}

ush::Error ush::Repl::launchBinary()
{
#if __windows__ || defined (WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
  STARTUPINFO si;
  PROCESS_INFORMATION pi;

  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  ZeroMemory(&pi, sizeof(pi));

  if(!CreateProcess(NULL, args[0], NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
    switch(GetLastError()) {
      case ERROR_INVALID_PARAMETER:
        std::print( "CreateProcess failed (%d).\n", GetLastError() );
        break;
      case ERROR_FILE_NOT_FOUND:
        std::print("Unknown internal or external command.\n");
        break;
    }
    return Error::eError;
  }

  WaitForSingleObject( pi.hProcess, INFINITE );

	CloseHandle( pi.hProcess );
	CloseHandle( pi.hThread );
#elif __linux__
  std::array<char*, maxArgs + 1> argv{};
  std::size_t argc = 0;
  while (argc < maxArgs && m_args[argc][0] != '\0') {
    argv[argc] = m_args[argc];
    ++argc;
  }
  argv[argc] = nullptr;

  if (argv[0] == std::string("cd")) {
    chdir(argv[1]);
    File::saveDirectoryHistory(argv[1]);
  }
  else {
	  int status;
	  pid_t pid;
    Terminal::disableRawMode();
	  pid = fork();
	  if (pid == 0) {
	  	// Child process
	  	if(execvp(argv[0], argv.data()) == -1) {
        std::print("\"{0}\" not found\n", argv[0]);
        // exit from failed child process
        _exit(127);
	  	}
	  } else if (pid < 0) {
      std::print("ush::launch() --> pid: {0}, error in forking\n", pid);
      return Error::eError;
	  } else {
	  	// Parent process
	  	do {
	  		waitpid(pid, &status, WUNTRACED);
	      Terminal::enableRawMode();
        std::string command;
        for (size_t i = 0; i < argc; i++) {
          command += argv[i];
          command += " ";
        }
        File::saveCommandHistory(command);
	  	} while (!WIFEXITED(status) && !WIFSIGNALED(status));
	  }
  }
#endif
	return Error::eSuccess;
}

void ush::Repl::resetLineVarsShowPrompt()
{
  // reset variables and show prompt again
  m_replBox.moveCursorInContentArea(m_replBox.m_contentPosition.m_col, m_replBox.m_contentPosition.m_row);
  m_charPosition = m_replBox.m_contentPosition.m_col;
  m_cursorPosition = m_replBox.m_contentPosition.m_col;

  Terminal::writeIcon(Icons::hollowRightPointingSmallTriangle);
  //write(STDOUT_FILENO, reinterpret_cast<const char*>(Icons::hollowRightPointingSmallTriangle),
  //  std::char_traits<char8_t>::length(Icons::hollowRightPointingSmallTriangle));
  Terminal::writeSpace();
  //write(STDOUT_FILENO, " ", 1);
}

ush::Error ush::Repl::cd()
{
  if (m_args[1] == std::string("~") || m_args[1] == std::string(" ")) {
    chdir(std::getenv("HOME"));
  }
  else {
    chdir(m_args[1]);
  }
  File::saveDirectoryHistory(std::string("cd ") + m_args[1]);

  clearRepl();
  showElns(std::filesystem::current_path());
  //resetLineVarsShowPrompt();

  return Error::eSuccess;
}

ush::Error ush::Repl::help(void)
{
  std::print("Welcome to universal shell(ush)");
  return Error::eSuccess;
}

ush::Error ush::Repl::exit(void)
{
  return Error::eExit;
}

void ush::Repl::moveBackToFirstCharOfWord()
{
  while(true) {
    if (m_cursorPosition > 0) {
      if (m_chars[m_cursorPosition] != 32 && m_chars[m_cursorPosition - 1] == 32) {
        Terminal::moveCursorLeftOneChar();
        //write(STDOUT_FILENO, "\x1b[D", 3);
        m_cursorPosition--;
        break;
      }

      if (m_chars[m_cursorPosition - 1] != 32) {
        Terminal::moveCursorLeftOneChar();
        //write(STDOUT_FILENO, "\x1b[D", 3);
        m_cursorPosition--;
        continue;
      }
      break;
    }
    break;
  }
}

void ush::Repl::moveBackToFirstNonSpaceChar()
{
  while(true) {
    if (m_cursorPosition > 0) {
      if (m_chars[m_cursorPosition - 1] == 32) {
        Terminal::moveCursorLeftOneChar();
        //write(STDOUT_FILENO, "\x1b[D", 3);
        m_cursorPosition--;
        continue;
      }
      Terminal::moveCursorLeftOneChar();
      //write(STDOUT_FILENO, "\x1b[D", 3);
      m_cursorPosition--;
      break;
    }
    break;
  }
}

void ush::Repl::moveForwardToFirstNonSpaceChar()
{
  while(true) {
    if (m_cursorPosition < m_charPosition) {
      if (m_chars[m_cursorPosition + 1] == 32) {
        Terminal::moveCursorRightOneChar();
        //write(STDOUT_FILENO, "\x1b[C", 3);
        m_cursorPosition++;
        continue;
      }
      break;
    }
    break;
  }
}

void ush::Repl::moveForwardToFirstSpaceAfterCurrentWord()
{
  while(true) {
    if (m_cursorPosition < m_charPosition) {
      if (m_chars[m_cursorPosition + 1] != 32) {
        Terminal::moveCursorRightOneChar();
        //write(STDOUT_FILENO, "\x1b[C", 3);
        m_cursorPosition++;
        continue;
      }
      Terminal::moveCursorRightOneChar();
      //write(STDOUT_FILENO, "\x1b[C", 3);
      m_cursorPosition++;
      break;
    }
    break;
  }
}

bool ush::Repl::lineIsEmpty()
{
  if(m_charPosition == 0 && m_cursorPosition == 0) {
    return true;
  }
  return false;
}

void ush::Repl::showElns(std::string path)
{
  m_elnNumber = 1U;
  std::vector<std::filesystem::directory_entry> entries;

  for (const auto& entry : std::filesystem::directory_iterator(path)) {
    entries.push_back(entry);
  }

  std::ranges::sort(entries, {}, [](const auto& e) {
    return e.path().filename().string();
  });

  char elnString[16];
  for (const auto& entry : entries) {
    auto [ptr, ec] = std::to_chars(elnString, elnString + sizeof(elnString), m_elnNumber);
    if (ec == std::errc{}) {
      std::string name = entry.path().filename().string();

      Terminal::startColor(Terminal::EColorAttr::eForeground, 109, 229, 210);
      Terminal::writeText(elnString, ptr - elnString);
      Terminal::endColor();
      Terminal::writeSpace();

      //write(STDOUT_FILENO, "\033[38;2;109;229;210m", 20);
      //write(STDOUT_FILENO, elnString, ptr - elnString);
      //write(STDOUT_FILENO, "\033[0m", 4);
      //write(STDOUT_FILENO, " ", 1);

      if (entry.is_directory()) {
        drawElnNode(name.data(), name.size(), Icons::folder, Terminal::EColorAttr::eForeground, 102, 153, 204);
        continue;
      } 
      else if (entry.is_regular_file()) {
        magic_t m = magic_open(MAGIC_MIME_TYPE);
        magic_load(m, nullptr);
        const char* type = magic_file(m, name.data());
        if(std::string(type).compare("image/jpeg") == 0) {
          drawElnNode(name.data(), name.size(), Icons::image, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("video/mp4") == 0) {
          drawElnNode(name.data(), name.size(), Icons::video, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("text/plain") == 0) {
          drawElnNode(name.data(), name.size(), Icons::text, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("text/html") == 0) {
          drawElnNode(name.data(), name.size(), Icons::html, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("text/css") == 0) {
          drawElnNode(name.data(), name.size(), Icons::css, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("text/javascript") == 0) {
          drawElnNode(name.data(), name.size(), Icons::js, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("text/markdown") == 0) {
          drawElnNode(name.data(), name.size(), Icons::markdown, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("text/csv") == 0) {
          drawElnNode(name.data(), name.size(), Icons::csv, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("text/xml") == 0) {
          drawElnNode(name.data(), name.size(), Icons::xml, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("text/x-c") == 0) {
          drawElnNode(name.data(), name.size(), Icons::c, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("text/x-c++") == 0) {
          drawElnNode(name.data(), name.size(), Icons::cpp, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("text/x-python") == 0) {
          drawElnNode(name.data(), name.size(), Icons::python, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("text/x-rust") == 0) {
          drawElnNode(name.data(), name.size(), Icons::rust, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("text/x-go") == 0) {
          drawElnNode(name.data(), name.size(), Icons::go, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("text/x-zig") == 0) {
          drawElnNode(name.data(), name.size(), Icons::zig, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("text/x-java") == 0) {
          drawElnNode(name.data(), name.size(), Icons::java, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("text/x-shellscript") == 0) {
          drawElnNode(name.data(), name.size(), Icons::bash, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("application/json") == 0) {
          drawElnNode(name.data(), name.size(), Icons::json, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("application/xml") == 0) {
          drawElnNode(name.data(), name.size(), Icons::xml, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("application/yaml") == 0) {
          drawElnNode(name.data(), name.size(), Icons::yaml, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("application/toml") == 0) {
          drawElnNode(name.data(), name.size(), Icons::toml, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("image/png") == 0) {
          drawElnNode(name.data(), name.size(), Icons::image, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("image/gif") == 0) {
          drawElnNode(name.data(), name.size(), Icons::image, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("image/webp") == 0) {
          drawElnNode(name.data(), name.size(), Icons::image, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("image/svg+xml") == 0) {
          drawElnNode(name.data(), name.size(), Icons::image, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("image/bmp") == 0) {
          drawElnNode(name.data(), name.size(), Icons::image, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("image/tiff") == 0) {
          drawElnNode(name.data(), name.size(), Icons::image, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("image/x-icon") == 0) {
          drawElnNode(name.data(), name.size(), Icons::image, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("audio/mpeg") == 0) {
          drawElnNode(name.data(), name.size(), Icons::audio, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("audio/flac") == 0) {
          drawElnNode(name.data(), name.size(), Icons::audio, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("audio/wav") == 0) {
          drawElnNode(name.data(), name.size(), Icons::audio, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("audio/ogg") == 0) {
          drawElnNode(name.data(), name.size(), Icons::audio, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("audio/aac") == 0) {
          drawElnNode(name.data(), name.size(), Icons::audio, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("audio/mp4") == 0) {
          drawElnNode(name.data(), name.size(), Icons::audio, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("audio/webm") == 0) {
          drawElnNode(name.data(), name.size(), Icons::audio, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("audio/midi") == 0) {
          drawElnNode(name.data(), name.size(), Icons::audio, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("video/x-matroska") == 0) {
          drawElnNode(name.data(), name.size(), Icons::video, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("video/webm") == 0) {
          drawElnNode(name.data(), name.size(), Icons::video, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("video/x-msvideo") == 0) {
          drawElnNode(name.data(), name.size(), Icons::video, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("video/quicktime") == 0) {
          drawElnNode(name.data(), name.size(), Icons::video, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("video/mpeg") == 0) {
          drawElnNode(name.data(), name.size(), Icons::video, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("video/ogg") == 0) {
          drawElnNode(name.data(), name.size(), Icons::video, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("application/pdf") == 0) {
          drawElnNode(name.data(), name.size(), Icons::pdf, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("application/rtf") == 0) {
          drawElnNode(name.data(), name.size(), Icons::word, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("application/msword") == 0) {
          drawElnNode(name.data(), name.size(), Icons::word, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("application/vnd.openxmlformats-officedocument.wordprocessingml.document") == 0) {
          drawElnNode(name.data(), name.size(), Icons::word, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("application/vnd.ms-excel") == 0) {
          drawElnNode(name.data(), name.size(), Icons::excel, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("application/vnd.openxmlformats-officedocument.spreadsheetml.sheet") == 0) {
          drawElnNode(name.data(), name.size(), Icons::excel, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("application/vnd.ms-powerpoint") == 0) {
          drawElnNode(name.data(), name.size(), Icons::powerpoint, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("application/vnd.openxmlformats-officedocument.presentationml.presentation") == 0) {
          drawElnNode(name.data(), name.size(), Icons::powerpoint, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("application/zip") == 0) {
          drawElnNode(name.data(), name.size(), Icons::zip, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("application/x-tar") == 0) {
          drawElnNode(name.data(), name.size(), Icons::zip, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("application/gzip") == 0) {
          drawElnNode(name.data(), name.size(), Icons::zip, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("application/x-bzip2") == 0) {
          drawElnNode(name.data(), name.size(), Icons::zip, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("application/x-xz") == 0) {
          drawElnNode(name.data(), name.size(), Icons::zip, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("application/x-7z-compressed") == 0) {
          drawElnNode(name.data(), name.size(), Icons::zip, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("application/vnd.rar") == 0) {
          drawElnNode(name.data(), name.size(), Icons::zip, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("application/x-zstd") == 0) {
          drawElnNode(name.data(), name.size(), Icons::zip, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("application/x-executable") == 0) {
          drawElnNode(name.data(), name.size(), Icons::binary, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("application/x-sharedlib") == 0) {
          drawElnNode(name.data(), name.size(), Icons::library, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("application/x-pie-executable") == 0) {
          drawElnNode(name.data(), name.size(), Icons::binary, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("application/x-object") == 0) {
          drawElnNode(name.data(), name.size(), Icons::binary, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("application/x-mach-binary") == 0) {
          drawElnNode(name.data(), name.size(), Icons::binary, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("application/x-dosexec") == 0) {
          drawElnNode(name.data(), name.size(), Icons::binary, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("font/ttf") == 0) {
          drawElnNode(name.data(), name.size(), Icons::font, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("font/otf") == 0) {
          drawElnNode(name.data(), name.size(), Icons::font, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("font/woff") == 0) {
          drawElnNode(name.data(), name.size(), Icons::font, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("font/woff2") == 0) {
          drawElnNode(name.data(), name.size(), Icons::font, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("application/octet-stream") == 0) {
          drawElnNode(name.data(), name.size(), Icons::binary, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("application/x-ole-storage") == 0) {
          drawElnNode(name.data(), name.size(), Icons::binary, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("application/x-ms-pdb") == 0) {
          drawElnNode(name.data(), name.size(), Icons::debug, Terminal::EColorAttr::eForeground, 169, 218, 169);
          continue;
        }
        drawElnNode(name.data(), name.size(), Icons::file, Terminal::EColorAttr::eForeground, 169, 218, 169);
        continue;
      } 
      else {
        drawElnNode(name.data(), name.size(), Icons::file, Terminal::EColorAttr::eForeground, 102, 153, 204);
        continue;
      }
    }
  }

  for (size_t i = 0; i < Terminal::getTerminalWindowSize().ws_col; i++) {
    //write(STDOUT_FILENO, "\033[38;2;179;179;179m", 20);
    Terminal::startColor(Terminal::EColorAttr::eForeground, 179, 179, 179);
    Terminal::writeChar("-");
    //write(STDOUT_FILENO, "-", 1);
    Terminal::endColor();
    //write(STDOUT_FILENO, "\033[0m", 4);
  }
  Terminal::goTostartOfLine();
  Terminal::makeNewLine();
  //write(STDOUT_FILENO, "\r\n", 2);
}

void ush::Repl::drawElnNode(const char* name,
  size_t size,
  const char8_t* iconName,
  Terminal::EColorAttr attr,
  uint32_t r, uint32_t g, uint32_t b)
{
  Terminal::startColor(attr, r, g, b);

  Terminal::writeIcon(iconName);
  Terminal::writeSpace();
  Terminal::writeText(name, size);

  Terminal::endColor();

  Terminal::goTostartOfLine();
  Terminal::makeNewLine();
  m_elnNumber++;
}

ush::Error ush::Repl::clearRepl(void)
{
  if (m_replBox.clearBoxContent() != Error::eSuccess) {
    return Error::eError;
  }

  m_charPosition = 1U;
  m_cursorPosition = 1U;
  return Error::eSuccess;
}
 
ush::Error ush::Repl::clearLine(void)
{
  if (m_replBox.clearLine(m_replBox.m_contentPosition.m_row, m_replBox.m_contentPosition.m_col) != Error::eSuccess) {
    return Error::eError;
  }

  m_inDirHistoryTravelMode = true;
  m_inCommandHistoryTravelMode = true;
  resetLineVarsShowPrompt();
  return Error::eSuccess;
}
