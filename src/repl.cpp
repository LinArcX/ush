#include "repl.h"

#include <print>
#include <stdio.h>
#include <fstream>
#include <unistd.h>
#include <string_view>
#include <algorithm>
#include <magic.h>

#ifdef __linux__
#include <csignal>
#include <sys/wait.h>
#include <sys/types.h>
#elif __windows__ || defined (WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#include <Windows.h>
#endif

void ush::Repl::SIGINTHandler(int signal)
{
  write(STDOUT_FILENO, "\r\n", 2);
}

ush::Repl::Repl()
{
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &m_ws) == -1) {
    perror("ioctl");
  }
  enableRawMode();

  // SIGINIT is disabled in ush main process, and just child process are allowed to have SIGINIT.
  // When it happens in a child-process, we exit from it and we just go to next line ready for another command in ush.
  std::signal(SIGINT, ush::Repl::SIGINTHandler);

  readCommandHistory();
  readDirectoryHistory();
}

ush::Repl::~Repl()
{
  disableRawMode();
}

int ush::Repl::loop(void)
{
  clearScreen();
  showElns(std::filesystem::current_path());

  while(true) {
    // reset arrays
    m_chars = {};
    m_args = {};

    Error e = handleEventsAndPopulateChars();
    if (e != Error::eSuccess) {
      continue;
    }

    e = parseCharsAndPopulateCommandsArgs();
    if (e != Error::eSuccess) {
      continue;
    }
 
    e = execute();
    if (Error::eExit == e) {
      return 0;
    }
  }
}

ush::Error ush::Repl::handleEventsAndPopulateChars()
{
	resetLineVarsShowPrompt();
  while (read(STDIN_FILENO, &c, 1) == 1) {
    // Ctrl-a: beginning of line
    if (c == 1) {
      while (m_cursorPosition > 0) {
        write(STDOUT_FILENO, "\x1b[D", 3);
        m_cursorPosition--;
      }
      continue;
    }

    // Ctrl-e: end of line
    if (c == 5) {
      while (m_cursorPosition < m_charPosition) {
        write(STDOUT_FILENO, "\x1b[C", 3);
        m_cursorPosition++;
      }
      continue;
    }

    // Ctrl-l
    if (c == 12) {
      clearScreen();
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
        write(STDOUT_FILENO, "\b \b", 3);

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
          if (m_inDirHistoryLastIndexVisited < m_dirsHistory.size()) {
            std::string item = m_dirsHistory[++m_inDirHistoryLastIndexVisited];
            size_t itemSize = item.size();
            if (itemSize > 0) {
              clearLine();
            }
            for (size_t i = 0; i < itemSize; i++) {
              write(STDOUT_FILENO, &item[i], 1);
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
          if (m_inDirHistoryLastIndexVisited > 0) {
            std::string item = m_dirsHistory[--m_inDirHistoryLastIndexVisited];
            size_t itemSize = item.size();
            if (itemSize > 0) {
              clearLine();
            }
            for (size_t i = 0; i < itemSize; i++) {
              write(STDOUT_FILENO, &item[i], 1);
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
            if (m_inCommandHistoryLastIndexVisited > 0) {
              std::string item = m_commandsHistory[--m_inCommandHistoryLastIndexVisited];
              size_t itemSize = item.size();
              if (itemSize > 0) {
                clearLine();
              }
              for (size_t i = 0; i < item.size(); i++) {
                write(STDOUT_FILENO, &item[i], 1);
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
            if (m_inCommandHistoryLastIndexVisited < m_commandsHistory.size()) {
              std::string item = m_commandsHistory[++m_inCommandHistoryLastIndexVisited];
              size_t itemSize = item.size();
              if (itemSize > 0) {
                clearLine();
              }
              for (size_t i = 0; i < itemSize; i++) {
                write(STDOUT_FILENO, &item[i], 1);
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
            write(STDOUT_FILENO, "\x1b[C", 3);
            m_cursorPosition++;
            m_inCommandHistoryTravelMode = false;
            m_inDirHistoryTravelMode = false;
          }
          continue;
        }

        // Left - previous char
        if (extSeq1[0] == 'D') {
          if (m_cursorPosition > 0) {
            write(STDOUT_FILENO, "\x1b[D", 3);
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
              write(STDOUT_FILENO, "u", 1);
            }
 
            continue;
          }

          // Ctrl+Down
          if (extSeq2[2] == 'B') {
            write(STDOUT_FILENO, "d", 1);
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
			write(STDOUT_FILENO, "\r\n", 2);
		  if (lineIsEmpty()) {
		    resetLineVarsShowPrompt();
		    continue;
		  }

			m_chars[m_charPosition] = '\0';
		  m_cursorPosition = 0;
      m_inDirHistoryTravelMode = true;
      m_inCommandHistoryTravelMode = true;

      readCommandHistory();
      readDirectoryHistory();
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
        write(STDOUT_FILENO, "\r", 1);
        write(STDOUT_FILENO, "\x1b[2K", 4); // Clear line
        write(STDOUT_FILENO, " > ", 3);
        write(STDOUT_FILENO, m_chars.data(), m_charPosition);

        char buf[32];
        int n = std::snprintf(buf, sizeof(buf), "\r\x1b[%uC",
                      static_cast<unsigned>(3 + m_cursorPosition)); // 3 = prompt length
        write(STDOUT_FILENO, buf, n);
		  }
		  // this is the normal path, as you type, you move forward. it include chars and SPACE
		  else {
		    m_cursorPosition++;

			  m_chars[m_charPosition] = c;
		    m_charPosition++;
		    write(STDOUT_FILENO, &c, 1);
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
	  clearScreen();
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
    saveDirectoryHistory(argv[1]);
  }
  else {
	  int status;
	  pid_t pid;
    disableRawMode();
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
	      enableRawMode();
        std::string command;
        for (size_t i = 0; i < argc; i++) {
          command += argv[i];
          command += " ";
        }
        saveCommandHistory(command);
	  	} while (!WIFEXITED(status) && !WIFSIGNALED(status));
	  }
  }
#endif
	return Error::eSuccess;
}

void ush::Repl::clearScreen(void)
{
#if __windows__ || defined (WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
  HANDLE hStdout;
  CHAR_INFO fill;
  COORD scrollTarget;
  SMALL_RECT scrollRect;
  CONSOLE_SCREEN_BUFFER_INFO csbi;

  hStdout = GetStdHandle(STD_OUTPUT_HANDLE);

  // Get the number of character cells in the current buffer.
  if (!GetConsoleScreenBufferInfo(hStdout, &csbi)) {
    return Error::eError;
  }

  // Scroll the rectangle of the entire buffer.
  scrollRect.Left = 0;
  scrollRect.Top = 0;
  scrollRect.Right = csbi.dwSize.X;
  scrollRect.Bottom = csbi.dwSize.Y;

  // Scroll it upwards off the top of the buffer with a magnitude of the entire height.
  scrollTarget.X = 0;
  scrollTarget.Y = (SHORT)(0 - csbi.dwSize.Y);

  // Fill with empty spaces with the buffer's default text attribute.
  fill.Char.UnicodeChar = TEXT(' ');
  fill.Attributes = csbi.wAttributes;

  // Do the scroll
  ScrollConsoleScreenBuffer(hStdout, &scrollRect, NULL, scrollTarget, &fill);

  // Move the cursor to the top left corner too.
  csbi.dwCursorPosition.X = 0;
  csbi.dwCursorPosition.Y = 0;

  SetConsoleCursorPosition(hStdout, csbi.dwCursorPosition);
#elif __linux__
  // clear screen
  constexpr char clear_seq[] = "\x1b[3J\x1b[2J\x1b[H";
  write(STDOUT_FILENO, clear_seq, sizeof(clear_seq) - 1);

  m_charPosition = 0U;
  m_cursorPosition = 0U;
#endif
}
 
void ush::Repl::clearLine(void)
{
  // clear line
  write(STDOUT_FILENO, "\r", 1);
  write(STDOUT_FILENO, "\x1b[2K", 4);

  m_inDirHistoryTravelMode = true;
  m_inCommandHistoryTravelMode = true;
  resetLineVarsShowPrompt();
}

void ush::Repl::resetLineVarsShowPrompt()
{
  // reset variables and show prompt again
  m_charPosition = 0U;
  m_cursorPosition = 0U;
  write(STDOUT_FILENO, " > ", 3);
}

ush::Error ush::Repl::cd()
{
  if (m_args[1] == std::string("~") || m_args[1] == std::string(" ")) {
    chdir(std::getenv("HOME"));
  }
  else {
    chdir(m_args[1]);
  }
  saveDirectoryHistory(std::string("cd ") + m_args[1]);

  clearScreen();
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

void ush::Repl::enableRawMode()
{
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
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &m_raw);
}

void ush::Repl::disableRawMode()
{
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &m_original);
}

void ush::Repl::moveBackToFirstCharOfWord()
{
  while(true) {
    if (m_cursorPosition > 0) {
      if (m_chars[m_cursorPosition] != 32 && m_chars[m_cursorPosition - 1] == 32) {
        write(STDOUT_FILENO, "\x1b[D", 3);
        m_cursorPosition--;
        break;
      }

      if (m_chars[m_cursorPosition - 1] != 32) {
        write(STDOUT_FILENO, "\x1b[D", 3);
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
        write(STDOUT_FILENO, "\x1b[D", 3);
        m_cursorPosition--;
        continue;
      }
      write(STDOUT_FILENO, "\x1b[D", 3);
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
        write(STDOUT_FILENO, "\x1b[C", 3);
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
        write(STDOUT_FILENO, "\x1b[C", 3);
        m_cursorPosition++;
        continue;
      }
      write(STDOUT_FILENO, "\x1b[C", 3);
      m_cursorPosition++;
      break;
    }
    break;
  }
}

bool ush::Repl::readFile(const std::filesystem::path& path,
  std::vector<std::string>& vec)
{
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    return false;
  }

  std::string line;
  while (std::getline(file, line)) {
    vec.push_back(std::move(line));
  }

  return !file.bad(); // true unless an I/O error occurred
}

void ush::Repl::readCommandHistory()
{
  std::filesystem::path path 
    = std::filesystem::path(std::getenv("HOME"))
                                       / ".config"
                                       / "ush"
                                       / "history"
                                       / "commands";
  readFile(path, m_commandsHistory);
  std::erase(m_commandsHistory, "");
  m_inCommandHistoryLastIndexVisited = m_commandsHistory.size();
}

void ush::Repl::readDirectoryHistory()
{
  std::filesystem::path path 
    = std::filesystem::path(std::getenv("HOME"))
                                       / ".config"
                                       / "ush"
                                       / "history"
                                       / "dirs";
  readFile(path, m_dirsHistory);
  std::erase(m_dirsHistory, "");
  m_inDirHistoryLastIndexVisited = m_dirsHistory.size() - 1;
}

bool ush::Repl::saveFile(std::filesystem::path path,
  std::string_view file,
  std::string_view text)
{
    // Create parent directories if needed.
    std::error_code ec;
    std::filesystem::create_directories(path, ec);
    if (ec) {
        return false;
    }

    std::filesystem::path fullPath = path / file;

    std::ofstream osfile(fullPath, std::ios::binary | std::ios::app);

    if (!osfile) {
        return false;
    }

    osfile.write(text.data(), static_cast<std::streamsize>(text.size()));
    osfile.put('\n');

    return osfile.good();
}

void ush::Repl::saveCommandHistory(std::string str)
{
  std::filesystem::path dir 
    = std::filesystem::path(std::getenv("HOME")) 
                                       / ".config" 
                                       / "ush"
                                       / "history";
  saveFile(dir, "commands", str);
}

void ush::Repl::saveDirectoryHistory(std::string str)
{
  std::filesystem::path dir = 
    std::filesystem::path(std::getenv("HOME")) 
                                     / ".config" 
                                     / "ush"
                                     / "history";
  saveFile(dir, "dirs", str);
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
  char buf[16];
  std::vector<std::filesystem::directory_entry> entries;

  for (const auto& entry : std::filesystem::directory_iterator(path)) {
    entries.push_back(entry);
  }

  std::ranges::sort(entries, {}, [](const auto& e) {
    return e.path().filename().string();
  });

  for (const auto& entry : entries) {
    auto [ptr, ec] = std::to_chars(buf, buf + sizeof(buf), m_elnNumber);
    if (ec == std::errc{}) {
      std::string name = entry.path().filename().string();

      write(STDOUT_FILENO, "\033[38;2;109;229;210m", 20);
      write(STDOUT_FILENO, buf, ptr - buf);
      write(STDOUT_FILENO, "\033[0m", 4);
      write(STDOUT_FILENO, " ", 1);

      if (entry.is_directory()) {
        drawElnNode(name.data(), name.size(), m_folder, EElnAttr::eForeground, 102, 153, 204);
        continue;
      } 
      else if (entry.is_regular_file()) {
        magic_t m = magic_open(MAGIC_MIME_TYPE);
        magic_load(m, nullptr);
        const char* type = magic_file(m, name.data());
        if(std::string(type).compare("image/jpeg") == 0) {
          drawElnNode(name.data(), name.size(), m_image, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("video/mp4") == 0) {
          drawElnNode(name.data(), name.size(), m_video, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("text/plain") == 0) {
          drawElnNode(name.data(), name.size(), m_text, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("text/html") == 0) {
          drawElnNode(name.data(), name.size(), m_html, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("text/css") == 0) {
          drawElnNode(name.data(), name.size(), m_css, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("text/javascript") == 0) {
          drawElnNode(name.data(), name.size(), m_js, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("text/markdown") == 0) {
          drawElnNode(name.data(), name.size(), m_markdown, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("text/csv") == 0) {
          drawElnNode(name.data(), name.size(), m_csv, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("text/xml") == 0) {
          drawElnNode(name.data(), name.size(), m_xml, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("text/x-c") == 0) {
          drawElnNode(name.data(), name.size(), m_c, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("text/x-c++") == 0) {
          drawElnNode(name.data(), name.size(), m_cpp, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("text/x-python") == 0) {
          drawElnNode(name.data(), name.size(), m_python, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("text/x-rust") == 0) {
          drawElnNode(name.data(), name.size(), m_rust, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("text/x-go") == 0) {
          drawElnNode(name.data(), name.size(), m_go, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("text/x-zig") == 0) {
          drawElnNode(name.data(), name.size(), m_zig, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("text/x-java") == 0) {
          drawElnNode(name.data(), name.size(), m_java, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("text/x-shellscript") == 0) {
          drawElnNode(name.data(), name.size(), m_bash, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("application/json") == 0) {
          drawElnNode(name.data(), name.size(), m_json, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("application/xml") == 0) {
          drawElnNode(name.data(), name.size(), m_xml, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("application/yaml") == 0) {
          drawElnNode(name.data(), name.size(), m_yaml, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("application/toml") == 0) {
          drawElnNode(name.data(), name.size(), m_toml, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("image/png") == 0) {
          drawElnNode(name.data(), name.size(), m_image, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("image/gif") == 0) {
          drawElnNode(name.data(), name.size(), m_image, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("image/webp") == 0) {
          drawElnNode(name.data(), name.size(), m_image, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("image/svg+xml") == 0) {
          drawElnNode(name.data(), name.size(), m_image, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("image/bmp") == 0) {
          drawElnNode(name.data(), name.size(), m_image, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("image/tiff") == 0) {
          drawElnNode(name.data(), name.size(), m_image, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("image/x-icon") == 0) {
          drawElnNode(name.data(), name.size(), m_image, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("audio/mpeg") == 0) {
          drawElnNode(name.data(), name.size(), m_audio, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("audio/flac") == 0) {
          drawElnNode(name.data(), name.size(), m_audio, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("audio/wav") == 0) {
          drawElnNode(name.data(), name.size(), m_audio, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("audio/ogg") == 0) {
          drawElnNode(name.data(), name.size(), m_audio, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("audio/aac") == 0) {
          drawElnNode(name.data(), name.size(), m_audio, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("audio/mp4") == 0) {
          drawElnNode(name.data(), name.size(), m_audio, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("audio/webm") == 0) {
          drawElnNode(name.data(), name.size(), m_audio, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("audio/midi") == 0) {
          drawElnNode(name.data(), name.size(), m_audio, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("video/x-matroska") == 0) {
          drawElnNode(name.data(), name.size(), m_video, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("video/webm") == 0) {
          drawElnNode(name.data(), name.size(), m_video, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("video/x-msvideo") == 0) {
          drawElnNode(name.data(), name.size(), m_video, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("video/quicktime") == 0) {
          drawElnNode(name.data(), name.size(), m_video, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("video/mpeg") == 0) {
          drawElnNode(name.data(), name.size(), m_video, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("video/ogg") == 0) {
          drawElnNode(name.data(), name.size(), m_video, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("application/pdf") == 0) {
          drawElnNode(name.data(), name.size(), m_pdf, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("application/rtf") == 0) {
          drawElnNode(name.data(), name.size(), m_word, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("application/msword") == 0) {
          drawElnNode(name.data(), name.size(), m_word, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("application/vnd.openxmlformats-officedocument.wordprocessingml.document") == 0) {
          drawElnNode(name.data(), name.size(), m_word, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("application/vnd.ms-excel") == 0) {
          drawElnNode(name.data(), name.size(), m_excel, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("application/vnd.openxmlformats-officedocument.spreadsheetml.sheet") == 0) {
          drawElnNode(name.data(), name.size(), m_excel, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("application/vnd.ms-powerpoint") == 0) {
          drawElnNode(name.data(), name.size(), m_powerpoint, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("application/vnd.openxmlformats-officedocument.presentationml.presentation") == 0) {
          drawElnNode(name.data(), name.size(), m_powerpoint, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("application/zip") == 0) {
          drawElnNode(name.data(), name.size(), m_zip, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("application/x-tar") == 0) {
          drawElnNode(name.data(), name.size(), m_zip, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("application/gzip") == 0) {
          drawElnNode(name.data(), name.size(), m_zip, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("application/x-bzip2") == 0) {
          drawElnNode(name.data(), name.size(), m_zip, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("application/x-xz") == 0) {
          drawElnNode(name.data(), name.size(), m_zip, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("application/x-7z-compressed") == 0) {
          drawElnNode(name.data(), name.size(), m_zip, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("application/vnd.rar") == 0) {
          drawElnNode(name.data(), name.size(), m_zip, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("application/x-zstd") == 0) {
          drawElnNode(name.data(), name.size(), m_zip, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("application/x-executable") == 0) {
          drawElnNode(name.data(), name.size(), m_binary, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("application/x-sharedlib") == 0) {
          drawElnNode(name.data(), name.size(), m_library, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("application/x-pie-executable") == 0) {
          drawElnNode(name.data(), name.size(), m_binary, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("application/x-object") == 0) {
          drawElnNode(name.data(), name.size(), m_binary, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("application/x-mach-binary") == 0) {
          drawElnNode(name.data(), name.size(), m_binary, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("application/x-dosexec") == 0) {
          drawElnNode(name.data(), name.size(), m_binary, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("font/ttf") == 0) {
          drawElnNode(name.data(), name.size(), m_font, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("font/otf") == 0) {
          drawElnNode(name.data(), name.size(), m_font, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("font/woff") == 0) {
          drawElnNode(name.data(), name.size(), m_font, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("font/woff2") == 0) {
          drawElnNode(name.data(), name.size(), m_font, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("application/octet-stream") == 0) {
          drawElnNode(name.data(), name.size(), m_binary, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("application/x-ole-storage") == 0) {
          drawElnNode(name.data(), name.size(), m_binary, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        if(std::string(type).compare("application/x-ms-pdb") == 0) {
          drawElnNode(name.data(), name.size(), m_debug, EElnAttr::eForeground, 169, 218, 169);
          continue;
        }
        drawElnNode(name.data(), name.size(), m_file, EElnAttr::eForeground, 169, 218, 169);
        continue;
      } 
      else {
        drawElnNode(name.data(), name.size(), m_file, EElnAttr::eForeground, 102, 153, 204);
        continue;
      }
    }
  }

  for (size_t i = 0; i < m_ws.ws_col; i++) {
    write(STDOUT_FILENO, "\033[38;2;179;179;179m", 20);
    write(STDOUT_FILENO, "-", 1);
    write(STDOUT_FILENO, "\033[0m", 4);
  }
  write(STDOUT_FILENO, "\r\n", 2);
}

void ush::Repl::drawElnNode(const char* name, 
    size_t size,
    const char8_t* iconName,
    EElnAttr attr,
    uint32_t r, uint32_t g, uint32_t b)
{
  std::string str;
  if (attr == EElnAttr::eForeground) {
    // color-start
    str = std::format("\033[38;2;{};{};{}m", r, g, b);
    //write(STDOUT_FILENO, "\033[38;2;169;218;169m", 20);
  }
  else {
    // color-start
    str = std::format("\033[48;2;{};{};{}m", r, g, b);
    //write(STDOUT_FILENO, "\033[48;2;169;218;169m", 20);
  }
  write(STDOUT_FILENO, str.data(), str.size());

  // icon
  write(STDOUT_FILENO, reinterpret_cast<const char*>(iconName),
      std::char_traits<char8_t>::length(iconName));
  // extra space after icon
  write(STDOUT_FILENO, " ", 1);
  // text
  write(STDOUT_FILENO, name, size);
  // color-end
  write(STDOUT_FILENO, "\033[0m", 4);

  write(STDOUT_FILENO, "\r\n", 2);
  m_elnNumber++;
}
