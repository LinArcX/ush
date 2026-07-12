#include "repl.h"

#include <print>
#include <stdio.h>
#include <unistd.h>
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
    if (Error::eClear == e) {
      continue;
    }
    e = parseCharsAndPopulateCommandsArgs(chars, args);
    e = execute(args);
    if (Error::eExit == e) {
      return 0;
    }
  }
}

ush::Error ush::Repl::handleEventsAndPopulateChars(std::array<char, charsForLine>& chars)
{
	char c ;
	uint32_t position = 0U;
  write(STDOUT_FILENO, " > ", 3);
	
  while (read(STDIN_FILENO, &c, 1) == 1) {
    // Space
    if (c == 32) {
      write(STDOUT_FILENO, " ", 1);
      continue;
    }

    // BakcSpace c == '\b'
    if (c == 127) {
      if (position > 0) {
        --position;
        chars[position] = '\0';
        write(STDOUT_FILENO, "\b \b", 3);
      }
      continue;
    }

    // Ctrl-l
    if (c == 12) {
      return clearScreen();
    }

    // Ctrl-u
    if (c == 21) { 
      position = 0;
      chars.fill('\0');
      return clearLine();
    }

		// If we hit EOF, replace it with a null character and return.
		if (c == '\r' || c == '\n') {
			write(STDOUT_FILENO, "\r\n", 2);
			chars[position] = '\0';
			return Error::eSuccess;
		} else {
			chars[position] = c;
		}
		position++;
		write(STDOUT_FILENO, &c, 1);

		// If we have exceeded the buffer, return error
		if (position >= charsForLine) {
		  return Error::eError;
		}
  }
  return Error::eSuccess;
}

ush::Error ush::Repl::parseCharsAndPopulateCommandsArgs(const std::array<char, charsForLine>& chars,
  std::array<char[charsForArg], maxArgs>& args)
{
  uint32_t j = 0U;
  uint32_t currentArg = 0U;
  for (size_t i = 0; i < charsForArg; i++) {
    char currentChar = chars[i];
    if (currentChar == '\0') {
      return Error::eSuccess;
    }
    if (isspace(currentChar)) {
      currentArg++;
      j = 0;
    }
    else if (isalnum(currentChar) || currentChar == '-') {
      args[currentArg][j++] = currentChar;
    }
  }
  return Error::eSuccess;
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
  return Error::eClear;
}
 
ush::Error ush::Repl::clearLine(void)
{   
  write(STDOUT_FILENO, "\r", 1);
  write(STDOUT_FILENO, "\x1b[2K", 4);
  //write(STDOUT_FILENO, " > ", 3);
  return Error::eClear;
}

ush::Error ush::Repl::help(void)
{
  std::print("Welcome to unified shell(ush) ***");
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
