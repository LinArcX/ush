#include "box.h"
#include "icons.h"
#include "terminal.h"

#include <unistd.h>

ush::Error ush::Box::moveCursor(uint32_t row, uint32_t col)
{
  //  Terminal::getTerminalWindowSize().ws_row
  //   col > Terminal::getTerminalWindowSize().ws_col
  if(row < m_row || row > m_position.m_row) {
    return Error::eError;
  }
  if(col < m_col || row > m_position.m_col) {
    return Error::eError;
  }

  Terminal::moveCursorToLineColumn(row, col);
  return Error::eSuccess;
}

ush::Error ush::Box::drawContent(uint32_t& row, uint32_t& col)
{
  // left
  if(moveCursor(row, col++) != Error::eSuccess) {
    return Error::eError;
  }
  Terminal::writeIcon(Icons::left);
  //write(STDOUT_FILENO, reinterpret_cast<const char*>(Icons::left),
  //  std::char_traits<char8_t>::length(Icons::left));

  // right
  col = col + (m_width - 2);
  if (moveCursor(row, col) != Error::eSuccess) {
    return Error::eError;
  }

  if (moveCursor(row, col++) != Error::eSuccess) {
    return Error::eError;
  }
  Terminal::writeIcon(Icons::right);
  //write(STDOUT_FILENO, reinterpret_cast<const char*>(Icons::right),
  //  std::char_traits<char8_t>::length(Icons::right));

  if (moveCursor(row++, col) != Error::eSuccess) {
    return Error::eError;
  }
  Terminal::goTostartOfLine();
  Terminal::makeNewLine();
  //write(STDOUT_FILENO, "\r\n", 2);
  col = m_col;

  return Error::eSuccess;
}

ush::Error ush::Box::draw(uint32_t col, uint32_t row, uint32_t width, uint32_t height)
{
  m_col = col;
  m_row = row;
  m_width = width;
  m_height = height;

  m_position.m_row = m_row + 1;
  m_position.m_col = m_col + 1;
  m_position.m_width = m_width - 2;
  m_position.m_height = m_height - 2;

  // top border
  {
    // top-left
    if (moveCursor(row, col++) != Error::eSuccess) {
      return Error::eError;
    }
    Terminal::writeIcon(Icons::topLeft);

    // write(STDOUT_FILENO, reinterpret_cast<const char*>(Icons::topLeft),
    //   std::char_traits<char8_t>::length(Icons::topLeft));

    // top
    for (uint32_t i = col; i < m_width; i++) {
      if (moveCursor(row, col++) != Error::eSuccess) {
        return Error::eError;
      }
      Terminal::writeIcon(Icons::top);
      //write(STDOUT_FILENO, reinterpret_cast<const char*>(Icons::top),
      //  std::char_traits<char8_t>::length(Icons::top));
    }

    // top-right
    if (moveCursor(row, col++) != Error::eSuccess) {
      return Error::eError;
    }
    Terminal::writeIcon(Icons::topRight);
    //write(STDOUT_FILENO, reinterpret_cast<const char*>(Icons::topRight),
    //  std::char_traits<char8_t>::length(Icons::topRight));

    if (moveCursor(row++, col) != Error::eSuccess) {
      return Error::eError;
    }
    Terminal::goTostartOfLine();
    Terminal::makeNewLine();
    //write(STDOUT_FILENO, "\r\n", 2);
    col = m_col;
  }

  // content border
  for (uint32_t i = 1; i < m_height - 1; i++) {
    if (drawContent(row, col) != Error::eSuccess) {
      return Error::eError;
    }
  }

  // bottom border
  {
    // bottom-left
    if (moveCursor(row, col++) != Error::eSuccess) {
      return Error::eError;
    }
    Terminal::writeIcon(Icons::bottomLeft);
      
    //write(STDOUT_FILENO, reinterpret_cast<const char*>(Icons::bottomLeft),
    //  std::char_traits<char8_t>::length(Icons::bottomLeft));

    // bottom
    for (uint32_t i = col; i < m_width; i++) {
      if (moveCursor(row, col++) != Error::eSuccess) {
        return Error::eError;
      }
      Terminal::writeIcon(Icons::bottom);
      //write(STDOUT_FILENO, reinterpret_cast<const char*>(Icons::bottom),
      //  std::char_traits<char8_t>::length(Icons::bottom));
    }

    // bottom-right
    if (moveCursor(row, col++) != Error::eSuccess) {
      return Error::eError;
    }
    Terminal::writeIcon(Icons::bottomRight);
    //write(STDOUT_FILENO, reinterpret_cast<const char*>(Icons::bottomRight),
    //  std::char_traits<char8_t>::length(Icons::bottomRight));

    if (moveCursor(row++, col) != Error::eSuccess) {
      return Error::eError;
    }
    Terminal::goTostartOfLine();
    Terminal::makeNewLine();
    //write(STDOUT_FILENO, "\r\n", 2);
    col = m_col;
  }
  return Error::eSuccess;
}

ush::Error ush::Box::clearBox(void)
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
  for (size_t i = ++m_row; i < m_height - 1; i++) {
    if (clearLine(i, m_col) != Error::eSuccess) {
      return Error::eError;
    }
  }
  return Error::eSuccess;
#endif
}
 
ush::Error ush::Box::clearLine(uint32_t row, uint32_t col)
{
  if (moveCursor(row, col) != Error::eSuccess) {
    return Error::eError;
  }
  Terminal::goTostartOfLine();
  Terminal::eraseEntireLine();

  // re-draw current row
  if (drawContent(row, col) != Error::eSuccess) {
    return Error::eError;
  }

  return Error::eSuccess;
}

ush::Error ush::Box::writeSpace()
{
  if (moveCursor(m_position.m_row, m_position.m_col++) != Error::eSuccess) {
    return Error::eError;
  }
  Terminal::writeSpace();
  return Error::eSuccess;
}

ush::Error ush::Box::writeNewLine()
{
  if (moveCursor(m_position.m_row++, m_position.m_col) != Error::eSuccess) {
    return Error::eError;
  }
  Terminal::makeNewLine();
  return Error::eSuccess;
}

ush::Error ush::Box::writeIcon(const char8_t* iconName)
{
  if (moveCursor(m_position.m_row, m_position.m_col++) != Error::eSuccess) {
    return Error::eError;
  }
  Terminal::writeIcon(iconName);
  return Error::eSuccess;
}

ush::Error ush::Box::writeText(const char* name, size_t size)
{
  if (moveCursor(m_position.m_row, m_position.m_col) != Error::eSuccess) {
    return Error::eError;
  }
  m_position.m_col += size;

  Terminal::writeText(name, size);
  return Error::eSuccess;
}
