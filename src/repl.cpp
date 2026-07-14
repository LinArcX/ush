#include "repl.h"

#include <print>
#include <stdio.h>
#include <fstream>
#include <unistd.h>
#include <filesystem>
#include <string_view>

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
  enableRawMode();

  // SIGINIT is disabled in ush main process, and just child process are allowed to have SIGINIT.
  // When it happens in a child-process, we exit from it and we just go to next line ready for another command in ush.
  std::signal(SIGINT, ush::Repl::SIGINTHandler);
}

ush::Repl::~Repl()
{
  disableRawMode();
}

int ush::Repl::loop(void)
{
  std::array<char, charsForLine> chars {};
  std::array<char[charsForArg], maxArgs> args {};

  while(true) {
    // reset arrays
    chars = {};
    args = {};

    Error e = handleEventsAndPopulateChars(chars);
    if (e != Error::eSuccess) {
      continue;
    }

    e = parseCharsAndPopulateCommandsArgs(chars, args);
    if (e != Error::eSuccess) {
      continue;
    }
 
    e = execute(args);
    if (Error::eExit == e) {
      return 0;
    }
  }
}

ush::Error ush::Repl::handleEventsAndPopulateChars(std::array<char, charsForLine>& chars)
{
  //// detect keys in raw mode
  //int c = 0U;
  //while (read(STDIN_FILENO, &c, 1) == 1) {
  //  printf("char: %c, decimal: %d hex: %02X\n",
  //      c,
  //      (unsigned char)c,
  //      (unsigned char)c);
  //}

	char c;
	uint32_t charPosition = 0U;
	uint32_t cursorPosition = 0U;
  write(STDOUT_FILENO, " > ", 3);
	
  while (read(STDIN_FILENO, &c, 1) == 1) {
    // Ctrl-l
    if (c == 12) {
      return clearScreen();
    }

    // Ctrl-u
    if (c == 21) { 
      return clearLine();
    }

    // BakcSpace ('\b' or 127)
    if (c == 127) {
      if (charPosition > 0) {
        --cursorPosition;
        write(STDOUT_FILENO, "\b \b", 3);

        --charPosition;
        chars[charPosition] = '\0';
      }
      continue;
    }

    if (c == 27 ) { // ESC or \x1b
      char seq[2];

      if (read(STDIN_FILENO, &seq[0], 1) != 1)
        continue;

      if (read(STDIN_FILENO, &seq[1], 1) != 1)
        continue;

      if (seq[0] == '[') {
        // Up - previous history
        if (seq[1] == 'A') {
          continue;
        }

        // Down - next history
        if (seq[1] == 'B') {
          continue;
        }

        // Right - next char
        if (seq[1] == 'C') {
          if (cursorPosition < charPosition) {
            write(STDOUT_FILENO, "\x1b[C", 3);
            cursorPosition++;
          }
          continue;
        }

        // Left - previous char
        if (seq[1] == 'D') {
          if (cursorPosition > 0) {
            write(STDOUT_FILENO, "\x1b[D", 3);
            cursorPosition--;
          }
          continue;
        }
        if (seq[1] == '1') {
          // Extended sequence: read more bytes here
          char extSeq[3];

          if (read(STDIN_FILENO, &extSeq[0], 1) != 1)
            continue;

          if (read(STDIN_FILENO, &extSeq[1], 1) != 1)
            continue;

          if (read(STDIN_FILENO, &extSeq[2], 1) != 1)
            continue;

          // Ctrl+Up
          if (extSeq[2] == 'A') {
            write(STDOUT_FILENO, "u", 1);
            continue;
          }

          // Ctrl+Down
          if (extSeq[2] == 'B') {
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
          if (extSeq[2] == 'C') {
            // Scenario 1
            if (chars[cursorPosition] != 32) {  // 32 is SPACE
              moveForwardToFirstSpaceAfterCurrentWord(cursorPosition, charPosition, chars);
            }
            // Scenario 2
            else {
              moveForwardToFirstNonSpaceChar(cursorPosition, charPosition, chars);
              moveForwardToFirstSpaceAfterCurrentWord(cursorPosition, charPosition, chars);
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
          if (extSeq[2] == 'D') {
            // Scenario 1
            if (chars[cursorPosition] != 32) {  // 32 is SPACE
              moveBackToFirstCharOfWord(cursorPosition, chars);
            }

            // Scenario 2
            else {
              moveBackToFirstNonSpaceChar(cursorPosition, chars);
              moveBackToFirstCharOfWord(cursorPosition, chars);
            }
            continue;
          }
        }
      }
    }

		// If we hit EOF, replace it with a null character and return.
		if (c == '\r' || c == '\n') {
		  cursorPosition = 0;

			chars[charPosition] = '\0';
			write(STDOUT_FILENO, "\r\n", 2);
			return Error::eSuccess;
		} else {
		  // this is when you start to move cursor back and foth to put space/chars
		  if (cursorPosition < charPosition) {
        for (std::size_t i = charPosition; i > cursorPosition; --i) {
            chars[i] = chars[i - 1];
        }
        chars[cursorPosition] = c;
        ++cursorPosition;
        ++charPosition;
        chars[charPosition] = '\0';
        write(STDOUT_FILENO, "\r", 1);
        write(STDOUT_FILENO, "\x1b[2K", 4); // Clear line
        write(STDOUT_FILENO, " > ", 3);
        write(STDOUT_FILENO, chars.data(), charPosition);

        char buf[32];
        int n = std::snprintf(buf, sizeof(buf), "\r\x1b[%uC",
                      static_cast<unsigned>(3 + cursorPosition)); // 3 = prompt length
        write(STDOUT_FILENO, buf, n);
		  }
		  // this is the normal path, as you type, you move forward. it include chars and SPACE
		  else {
		    cursorPosition++;

			  chars[charPosition] = c;
		    charPosition++;
		    write(STDOUT_FILENO, &c, 1);
		  }
		}

		// If we have exceeded the buffer, we just clear the line
		if (charPosition >= charsForLine) {
		  return clearLine();
		}
  }
  // unknow error
  return clearLine();
}

ush::Error ush::Repl::parseCharsAndPopulateCommandsArgs(const std::array<char, charsForLine>& chars,
  std::array<char[charsForArg], maxArgs>& args)
{
  bool seenChar = false;
  uint32_t currentArg = 0U;

  for (size_t i = 0, j = 0; i < charsForArg; i++) {
    char currentChar = chars[i];

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
    else if (isalnum(currentChar) || currentChar == '-') {
      seenChar = true;
      args[currentArg][j++] = currentChar;
    }
  }
  return Error::eError;
}

ush::Error ush::Repl::execute(std::array<char[charsForArg], maxArgs>& args)
{
  // search in builtin commands first
	if (std::string_view(args[0]) == std::string_view("clear")) {
	  return clearScreen();
	}
	if (std::string_view(args[0]) == std::string_view("help")) {
	  return help();
	}
	if (std::string_view(args[0]) == std::string_view("exit")) {
	  return exit();
	}
	
  // it's not a builtint. let's launch it as a separate process.
	return launchBinary(args);
}

ush::Error ush::Repl::launchBinary(std::array<char[charsForArg], maxArgs>& args)
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
	int status;
	pid_t pid;

  std::array<char*, maxArgs + 1> argv{};

  std::size_t argc = 0;

  while (argc < maxArgs && args[argc][0] != '\0') {
    argv[argc] = args[argc];
    ++argc;
  }
  argv[argc] = nullptr;

  disableRawMode();
	pid = fork();
	if (pid == 0) {
		// TODO: enable Ctrl-C to exit from child process
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
      // TODO 2: save directories in directory history located at: $HOME/.config/ush/history/dirs
      std::string command;
      for (size_t i = 0; i < argc; i++) {
        command += argv[i];
        command += " ";
      }
      saveFile("commands", command);

		} while (!WIFEXITED(status) && !WIFSIGNALED(status));
	}
#endif
	return Error::eSuccess;
}

ush::Error ush::Repl::clearScreen(void)
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
  //std::print("\033[3J\033[2J\033[H");
  //std::fflush(stdout);
  constexpr char clear_seq[] = "\x1b[3J\x1b[2J\x1b[H";
  write(STDOUT_FILENO, clear_seq, sizeof(clear_seq) - 1);
#endif
  return Error::eClearScreen;
}
 
ush::Error ush::Repl::clearLine(void)
{   
  write(STDOUT_FILENO, "\r", 1);
  write(STDOUT_FILENO, "\x1b[2K", 4);
  //write(STDOUT_FILENO, " > ", 3);
  return Error::eClearLine;
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
  tcgetattr(STDIN_FILENO, &original);

  raw = original;
  //raw.c_lflag &= ~(ICANON | ECHO | ECHOCTL); // IEXTEN | ISIG
  raw.c_lflag &= ~(ICANON | ECHO | IEXTEN | ISIG);
  //raw.c_iflag &= ~(IXON | ICRNL); // BRKINT | INPCK | ISTRIP
  raw.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
  // raw.c_oflag &= ~(OPOST);
  raw.c_cflag |= CS8;

  raw.c_cc[VMIN] = 1;
  raw.c_cc[VTIME] = 0;
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void ush::Repl::disableRawMode()
{
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &original);
}

void ush::Repl::moveBackToFirstCharOfWord(uint32_t& cursorPosition,
  std::array<char, charsForLine>& chars)
{
  while(true) {
    if (cursorPosition > 0) {
      if (chars[cursorPosition] != 32 && chars[cursorPosition - 1] == 32) {
        write(STDOUT_FILENO, "\x1b[D", 3);
        cursorPosition--;
        break;
      }

      if (chars[cursorPosition - 1] != 32) {
        write(STDOUT_FILENO, "\x1b[D", 3);
        cursorPosition--;
        continue;
      }
      break;
    }
    break;
  }
}

void ush::Repl::moveBackToFirstNonSpaceChar(uint32_t& cursorPosition,
  std::array<char, charsForLine>& chars)
{
  while(true) {
    if (cursorPosition > 0) {
      if (chars[cursorPosition - 1] == 32) {
        write(STDOUT_FILENO, "\x1b[D", 3);
        cursorPosition--;
        continue;
      }
      write(STDOUT_FILENO, "\x1b[D", 3);
      cursorPosition--;
      break;
    }
    break;
  }
}

void ush::Repl::moveForwardToFirstNonSpaceChar(uint32_t& cursorPosition,
  uint32_t& charPosition,
  std::array<char, charsForLine>& chars)
{
  while(true) {
    if (cursorPosition < charPosition) {
      if (chars[cursorPosition + 1] == 32) {
        write(STDOUT_FILENO, "\x1b[C", 3);
        cursorPosition++;
        continue;
      }
      break;
    }
    break;
  }
}

void ush::Repl::moveForwardToFirstSpaceAfterCurrentWord(uint32_t& cursorPosition,
  uint32_t& charPosition,
  std::array<char, charsForLine>& chars)
{
  while(true) {
    if (cursorPosition < charPosition) {
      if (chars[cursorPosition + 1] != 32) {
        write(STDOUT_FILENO, "\x1b[C", 3);
        cursorPosition++;
        continue;
      }
      write(STDOUT_FILENO, "\x1b[C", 3);
      cursorPosition++;
      break;
    }
    break;
  }
}

bool ush::Repl::saveFile(std::string_view fileName,
  std::string_view text)
{
    std::filesystem::path dir =
      std::filesystem::path(std::getenv("HOME")) 
      / ".config" 
      / "ush"
      / "history";

    // Create parent directories if needed.
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);

    if (ec) {
        return false;
    }

    std::filesystem::path fullPath = dir / fileName;

    std::ofstream file(fullPath, std::ios::binary | std::ios::app);

    if (!file) {
        return false;
    }

    file.write(text.data(), static_cast<std::streamsize>(text.size()));
    file.put('\n');

    return file.good();
}
