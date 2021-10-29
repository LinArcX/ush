#include "oswrapper.h"

// List of builtin commands, followed by their corresponding functions.
char *builtin_names[] = 
{
	"cd",
	"echo",
	"clear",
	"pwd",
	"help",
	"exit"
};

int (*builtin_functions[]) (char **) =
{
	&ush_cd,
	&ush_echo,
	&ush_clear,
	&ush_pwd,
	&ush_help,
	&ush_exit
};

int ush_builtins_number() {
  return sizeof(builtin_names) / sizeof(char *);
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

int ush_exit(char **args)
{
    return 0;
}

int ush_help(char **args)
{
	printf("*** Welcome to unified shell(ush) ***\nBuilt-in commands:\n");
    for (int i = 0; i < ush_builtins_number(); i++) {
		printf("%d: %s\n", i+1, builtin_names[i]);
	}
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