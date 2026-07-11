#include "repl.h"

#include <print>
#include <stdio.h>
#include <unistd.h>
#include <unistd.h>

#ifdef __linux__
#include <sys/wait.h>
#include <sys/types.h>
#elif __windows__ || defined (WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#include <Windows.h>
#endif

int ush::Repl::loop(void)
{
  std::array<char, charsForLine> chars {};
  std::array<char[charsForArg], maxArgs> args {};

  while(true) {
    // reset array content
    chars = {};
    args = {};

    std::print("> ");

    if (Error::eError == readLine(chars)) {
      std::print("readLine() error");
      return -1;
    }
    if (Error::eError == splitArgs(chars, args)) {
      std::print("splitLine() error");
      return -1;
    }
    Error e = execute(args);
    if (Error::eExit == e) {
      return 0;
    }
    if (Error::eError == e) {
      std::print("execute() error\n");
      return -1;
    }
  }
}

Error ush::Repl::readLine(std::array<char, charsForLine>& chars)
{
	int c = 0U;
	uint32_t position = 0U;

	while (true) {
		// Read a character
		c = getchar();

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
}

Error ush::Repl::splitArgs(const std::array<char, charsForLine>& chars,
  std::array<char[charsForArg], maxArgs>& args)
{
  uint32_t j = 0U;
  //bool inSpace = false;
  uint32_t currentArg = 0U;
  for (size_t i = 0; i < charsForArg; i++) {
    char currentChar = chars[i];
    if (currentChar == '\0') {
      return Error::eSuccess;
    }
    if (isalnum(currentChar)) {
      if (isspace(currentChar)) {
        currentArg++;
        j = 0;
        //inSpace = false;
      }
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
	  return clear();
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
      std::print("ush::launch() --> execvp() failed. argv[0]: {0}\n", argv[0]);
      return Error::eError;
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

Error ush::Repl::clear(void)
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
  std::print("\033[3J\033[2J\033[H");
#endif
  return Error::eSuccess;
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
