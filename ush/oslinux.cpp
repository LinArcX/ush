#include "oswrapper.h"

#ifdef __linux__

#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

int ush_launch(char **args)
{
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
	return 1;
}

int ush_cd(char **args)
{
    if (args[1] == NULL) 
    {
		fprintf(stderr, "ush: expected argument to \"cd\"\n");
	} 
    else
    {
		if (chdir(args[1]) != 0) {
		  perror("ush");
		}
    }
	return 1;
}

#endif