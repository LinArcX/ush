#include "oswrapper.h"

#if defined (WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)

#include <Windows.h>

int ush_launch(char **args)
{	
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
}

int ush_cd(char **args)
{
    if (args[1] == NULL) 
    {
		fprintf(stderr, "ush: expected argument to \"cd\"\n");
	} 
    else
    {
		if(!SetCurrentDirectory(args[1]))
		{
			printf("cd failed (%d)\n", GetLastError());
		}
    }
	return 1;
}

void clear(HANDLE hConsole)
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
    clear(hStdout);
	return 1;
}

int ush_pwd(char **args)
{
	TCHAR path[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, path);
	std::cout << path << std::endl;
	return 1;
}

#endif