#ifndef USH_H
#define USH_H

#pragma warning(push, 3)

#include <stdio.h>
#include <cstdlib>
#include <string.h>

#if defined (WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#include <Windows.h>
#elif __linux__
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#endif

#pragma warning(pop)

#define USH_RL_BUFSIZE 1024
#define USH_TOK_BUFSIZE 64
#define USH_TOK_DELIM " \t\r\n\a"

void ush_loop(void);
char *ush_read_line(void);
char **ush_split_line(char *line);
int ush_execute(char **args);

int ush_cd(char **args);
int ush_echo(char **args);
int ush_clear(char **args);
int ush_help(char **args);
int ush_exit(char **args);

#endif