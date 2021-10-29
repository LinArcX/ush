#ifndef OSWRAPPER_H
#define OSWRAPPER_H

#pragma warning(push, 3)
// C header files
#include <stdio.h>
#include <cstdlib>
#include <string.h>
// C++ header file
#include <iostream>
#pragma warning(pop)

#pragma warning(disable: 4996)

#define USH_RL_BUFSIZE 1024
#define USH_TOK_BUFSIZE 64
#define USH_TOK_DELIM " \t\r\n\a"

void ush_loop(void);
char *ush_read_line(void);
char **ush_split_line(char *line);
int ush_execute(char **args);
int ush_launch(char **args);

int ush_cd(char **args);
int ush_echo(char **args);
int ush_clear(char **args);
int ush_pwd(char **args);
int ush_help(char **args);
int ush_exit(char **args);

#endif