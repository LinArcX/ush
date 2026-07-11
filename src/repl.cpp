#include "repl.h"

#include <print>
#include <stdio.h>
#include <unistd.h>

#ifdef __linux__
#include <sys/wait.h>
#include <sys/types.h>
#elif __windows__ || defined (WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#include <Windows.h>
#endif

void ush::Repl::enableRawMode()
{
  tcgetattr(STDIN_FILENO, &original);

  raw = original;
  raw.c_lflag &= ~(ICANON);

  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void ush::Repl::disableRawMode()
{
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &original);
}

int ush::Repl::loop(void)
{
  std::array<char, charsForLine> chars {};
  std::array<char[charsForArg], maxArgs> args {};

  while(true) {
    // reset array content
    chars = {};
    args = {};

    Error e = readLine(chars);
    if (Error::eClear == e) {
      continue;
    }
    //if (Error::eError == e) {
    //  std::print("readLine() error");
    //  return -1;
    //}

    e = splitArgs(chars, args);
    //if (Error::eError == e) {
    //  std::print("splitArgs() error");
    //  return -1;
    //}

    e = execute(args);
    if (Error::eExit == e) {
      return 0;
    }
    //if (Error::eError == e) {
    //  //std::print("execute() error\n");
    //  return -1;
    //}
  }
}

Error ush::Repl::readLine(std::array<char, charsForLine>& chars)
{
	int c = 0U;
	uint32_t position = 0U;

  write(STDOUT_FILENO, " > ", 3);
	
  constexpr char CTRL_U = 'U' & 0x1F;
  constexpr char CTRL_L = 'L' & 0x1F;

  while (read(STDIN_FILENO, &c, 1) == 1) {
    if (c == CTRL_L) {
      return clearScreen();
    }

    if (c == CTRL_U) {
      position = 0;
      chars.fill('\0');

      return clearLine();
    }

    if (c == 127 || c == '\b') {
      if (position > 0) {
        --position;
        chars[position] = '\0';
      }
      return backSpace();
    }

		// If we hit EOF, replace it with a null character and return.
		if (c == '\n') {
			chars[position] = '\0';
			return Error::eSuccess;
		} else {
			chars[position] = c;
		}
		position++;

		// If we have exceeded the buffer, return error
		if (position >= charsForLine) {
		  return Error::eError;
		}
  }
  return Error::eSuccess;
}

Error ush::Repl::splitArgs(const std::array<char, charsForLine>& chars,
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

Error ush::Repl::execute(std::array<char[charsForArg], maxArgs>& args)
{
  // search in builtin commands first
  if (std::string_view(args[0]) == std::string_view("cd")) {
	  return cd(args[1]);
	}
  if (std::string_view(args[0]) == std::string_view("pwd")) {
	  return pwd(args[1]);
	}
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
	return launch(args);
}

Error ush::Repl::launch(std::array<char[charsForArg], maxArgs>& args)
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

  std::size_t argc = 1; /* number of arguments */

  for (std::size_t i = 0; i < argc; ++i)
    argv[i] = args[i];

  argv[argc] = nullptr;

	pid = fork();
	if (pid == 0) {
		// Child process
		if(execvp(argv[0], argv.data()) == -1) {
      std::print("\"{0}\" not found\n", argv[0]);
      // this exit from failed child process
      _exit(127);
		}
	} else if (pid < 0) {
    std::print("ush::launch() --> pid: {0}, error in forking\n", pid);
    return Error::eError;
	} else {
		// Parent process
		do {
			waitpid(pid, &status, WUNTRACED);
		} while (!WIFEXITED(status) && !WIFSIGNALED(status));
	}
#endif
	return Error::eSuccess;
}

Error ush::Repl::cd(std::string_view arg)
{
#if __windows__ || defined (WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
  if(!SetCurrentDirectory(args)) {
    std::print("cd failed (%d)\n", GetLastError());
		return Error::eError;
  }
#elif __linux__
  if (chdir(arg.data()) != 0) {
    std::print("ush");
		  return Error::eError;
  }
#endif
	return Error::eSuccess;
}

Error ush::Repl::pwd(std::string_view arg)
{
#if __windows__ || defined (WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
  TCHAR path[MAX_PATH];
  GetCurrentDirectory(MAX_PATH, path);
  std::cout << path << std::endl;
#elif __linux__

#endif
  return Error::eSuccess;
}

Error ush::Repl::clearScreen(void)
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
 
Error ush::Repl::clearLine(void)
{   
  write(STDOUT_FILENO, "\r", 1);
  write(STDOUT_FILENO, "\x1b[2K", 4);
  //write(STDOUT_FILENO, " > ", 3);
  return Error::eClear;
}

Error ush::Repl::backSpace(void)
{   
  char buff[] = "\b \b";
  write(STDOUT_FILENO, buff, sizeof(buff) - 1);
  return Error::eClear;
}
 

Error ush::Repl::help(void)
{
  std::print("Welcome to unified shell(ush) ***");
  return Error::eSuccess;
}

Error ush::Repl::exit(void)
{
  return Error::eExit;
}
