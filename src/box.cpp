#include "box.h"
#include "icons.h"

#include <string>
#include <unistd.h>

void ush::Box::moveCursor(uint32_t row, uint32_t col)
{
  std::string s =
    "\033[" +
    std::to_string(row) +
    ";" +
    std::to_string(col) +
    "H";

  write(STDOUT_FILENO, s.data(), s.size());
}

void ush::Box::draw(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
  m_x = x;
  m_y = y;
  m_width = width;
  m_height = height;

  moveCursor(x, y);

  // top border
  {
    // top-left
    write(STDOUT_FILENO, reinterpret_cast<const char*>(Icons::topLeft),
      std::char_traits<char8_t>::length(Icons::topLeft));

    // top
    for (uint32_t i = 1; i < width - 1; i++) {
      write(STDOUT_FILENO, reinterpret_cast<const char*>(Icons::top),
        std::char_traits<char8_t>::length(Icons::top));
    }

    // top-right
    write(STDOUT_FILENO, reinterpret_cast<const char*>(Icons::topRight),
      std::char_traits<char8_t>::length(Icons::topRight));

    write(STDOUT_FILENO, "\r\n", 2);
  }

  // content border
  {
    for (uint32_t i = x + 1; i < height - 1; i++) {
      // left
      write(STDOUT_FILENO, reinterpret_cast<const char*>(Icons::left),
        std::char_traits<char8_t>::length(Icons::left));

      // right
      moveCursor(x + 1, y);
      write(STDOUT_FILENO, reinterpret_cast<const char*>(Icons::right),
        std::char_traits<char8_t>::length(Icons::right));
 
      write(STDOUT_FILENO, "\r\n", 2);
    }
  }

  // bottom border
  {
    // bottom-left
    write(STDOUT_FILENO, reinterpret_cast<const char*>(Icons::bottomLeft),
      std::char_traits<char8_t>::length(Icons::bottomLeft));

    // bottom
    for (uint32_t i = 1; i < width - 1; i++) {
      write(STDOUT_FILENO, reinterpret_cast<const char*>(Icons::bottom),
        std::char_traits<char8_t>::length(Icons::bottom));
    }

    // bottom-right
    write(STDOUT_FILENO, reinterpret_cast<const char*>(Icons::bottomRight),
      std::char_traits<char8_t>::length(Icons::bottomRight));

    write(STDOUT_FILENO, "\r\n", 2);
  }
}

void ush::Box::clearBox(void)
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
  for (size_t i = 0; i < m_height; i++) {
    clearLine();
  }
  moveCursor(m_x, m_y);
#endif
}
 
void ush::Box::clearLine(void)
{
  //constexpr char clear_seq[] = "\x1b[3J\x1b[2J\x1b[H";
  //constexpr char clear_seq[] = "\x1b[2k";
  //write(STDOUT_FILENO, clear_seq, sizeof(clear_seq) - 1);
  write(STDOUT_FILENO, "\r", 1);
  write(STDOUT_FILENO, "\x1b[2K", 4);
}
