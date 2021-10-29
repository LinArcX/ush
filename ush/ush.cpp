#include "ush.h"
#pragma warning(disable: 4996)

int main(int argc, char** argv)
{
	// Load config files, if any.

	// Run command loop.
	ush_loop();

	// Perform any shutdown/cleanup.

	return EXIT_SUCCESS;
}

void ush_loop(void)
{
	int status;
	char *line;
	char **args;

	do {
		printf("> ");
		line = ush_read_line();
		args = ush_split_line(line);
		status = ush_execute(args);

		free(line);
		free(args);
	} while (status);
}

char *ush_read_line(void)
{
	int bufsize = USH_RL_BUFSIZE;
	int position = 0;
	char* buffer = (char*) malloc(sizeof(char) * bufsize);
	int c;

	if (!buffer) {
		fprintf(stderr, "ush: allocation error\n");
		exit(EXIT_FAILURE);
	}

	while (1) {
		// Read a character
		c = getchar();

		// If we hit EOF, replace it with a null character and return.
		if (c == EOF || c == '\n') {
			buffer[position] = '\0';
			return buffer;
		} else {
			buffer[position] = c;
		}
		position++;

		// If we have exceeded the buffer, reallocate.
		if (position >= bufsize) {
			bufsize += USH_RL_BUFSIZE;
			buffer = (char*) realloc(buffer, bufsize);
			if (!buffer) {
				fprintf(stderr, "ush: allocation error\n");
				exit(EXIT_FAILURE);
			}
		}
	}
}

char **ush_split_line(char *line)
{
	int bufsize = USH_TOK_BUFSIZE, position = 0;
	char **tokens = (char**) malloc(bufsize * sizeof(char*));
	char *token;

	if (!tokens) {
		fprintf(stderr, "ush: allocation error\n");
		exit(EXIT_FAILURE);
	}

	token = strtok(line, USH_TOK_DELIM);
	while (token != NULL) {
		tokens[position] = token;
		position++;

		if (position >= bufsize) {
			bufsize += USH_TOK_BUFSIZE;
			tokens = (char**) realloc(tokens, bufsize * sizeof(char*));
			if (!tokens) {
				fprintf(stderr, "ush: allocation error\n");
				exit(EXIT_FAILURE);
			}
		}

		token = strtok(NULL, USH_TOK_DELIM);
	}
	tokens[position] = NULL;
	return tokens;
}

int ush_launch(char **args)
{
#if defined (WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
	//define something for Windows (32-bit and 64-bit, this part is common)
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory( &si, sizeof(si) );
	si.cb = sizeof(si);
	ZeroMemory( &pi, sizeof(pi) );

	if(!CreateProcess(NULL, args[0], NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
	{
		switch(GetLastError())
		{
			case ERROR_INVALID_PARAMETER:
				printf( "CreateProcess failed (%d).\n", GetLastError() );
				break;
			case ERROR_FILE_NOT_FOUND:
				printf("Unknown internal or external command.\n");
				break;
		}
		return -1;
	}

	WaitForSingleObject( pi.hProcess, INFINITE );

	CloseHandle( pi.hProcess );
	CloseHandle( pi.hThread );
	return 1;
#elif __linux__
	pid_t pid, wpid;
	int status;

	pid = fork();
	if (pid == 0) {
		// Child process
		if (execvp(args[0], args) == -1) {
			perror("ush");
		}
		exit(EXIT_FAILURE);
	} else if (pid < 0) {
		// Error forking
		perror("ush");
	} else {
		// Parent process
		do {
			wpid = waitpid(pid, &status, WUNTRACED);
		} while (!WIFEXITED(status) && !WIFSIGNALED(status));
	}
#endif
	return 1;
}

// List of builtin commands, followed by their corresponding functions.
char *builtin_names[] = 
{
	"cd",
	"echo",
	"clear",
	"help",
	"exit"
};

int (*builtin_functions[]) (char **) =
{
	&ush_cd,
	&ush_echo,
	&ush_clear,
	&ush_help,
	&ush_exit
};

int ush_builtins_number() {
  return sizeof(builtin_names) / sizeof(char *);
}

int ush_cd(char **args)
{
    if (args[1] == NULL) 
    {
		fprintf(stderr, "ush: expected argument to \"cd\"\n");
	} 
    else
    {
#if defined (WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
		if(!SetCurrentDirectory(args[1]))
		{
			printf("cd failed (%d)\n", GetLastError());
		}
#elif __linux__
		if (chdir(args[1]) != 0) {
		  perror("ush");
		}
#endif
    }
	return 1;
}

int ush_echo(char **args)
{
	int flag;

	/* This utility may NOT do getopt(3) option parsing. */
	if (*++args && !strcmp(*args, "-n"))
	{
		++args;
		flag = 1;
	}
	else
	{
		flag = 0;		
	}

	while (*args)
	{
		(void)fputs(*args, stdout);
		if (*++args)
		{
			putchar(' ');
		}			
	}

	if (!flag)
	{
		putchar('\n');
	}	
	return 1;
}

int ush_help(char **args)
{
	printf("*** Welcome to unified shell(ush) ***\nBuilt-in commands:\n");
    for (int i = 0; i < ush_builtins_number(); i++) {
		printf("%d: %s\n", i+1, builtin_names[i]);
	}
    return 1;
}

int ush_exit(char **args)
{
    return 0;
}

void cls(HANDLE hConsole)
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    SMALL_RECT scrollRect;
    COORD scrollTarget;
    CHAR_INFO fill;

    // Get the number of character cells in the current buffer.
    if (!GetConsoleScreenBufferInfo(hConsole, &csbi))
    {
        return;
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
    ScrollConsoleScreenBuffer(hConsole, &scrollRect, NULL, scrollTarget, &fill);

    // Move the cursor to the top left corner too.
    csbi.dwCursorPosition.X = 0;
    csbi.dwCursorPosition.Y = 0;

    SetConsoleCursorPosition(hConsole, csbi.dwCursorPosition);
}

int ush_clear(char **args)
{
	HANDLE hStdout;
    hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    cls(hStdout);
	return 1;
}

int ush_execute(char **args)
{
	if (args[0] == NULL) {
		// An empty command was entered.
		return 1;
	}

	for (int i = 0; i < ush_builtins_number(); i++) {
		if (0 == strcmp(args[0], builtin_names[i]))
		{
			return (*builtin_functions[i])(args);
		}
	}

	return ush_launch(args);
}