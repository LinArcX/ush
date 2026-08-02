#include "box.h"
#include "icons.h"
#include "terminal.h"

#include <unistd.h>

ush::Error ush::Box::moveCursorInBorderArea(uint32_t row, uint32_t col)
{
  if(row < m_borderPosition.m_row || row > m_borderPosition.m_height) {
    return Error::eError;
  }
  if(col < m_borderPosition.m_col || col > m_borderPosition.m_width) {
    return Error::eError;
  }

  Terminal::moveCursorToLineColumn(row, col);
  return Error::eSuccess;
}

ush::Error ush::Box::moveCursorInContentArea(uint32_t row, uint32_t col)
{
  if(row < m_contentPosition.m_row || row > m_contentPosition.m_height) {
    return Error::eError;
  }
  if(col < m_contentPosition.m_col || col > m_contentPosition.m_width) {
    return Error::eError;
  }

  Terminal::moveCursorToLineColumn(row, col);
  return Error::eSuccess;
}

ush::Error ush::Box::drawBorder(uint32_t row, uint32_t col, uint32_t width, uint32_t height)
{
  m_borderPosition.m_row = row;
  m_borderPosition.m_col = col;
  m_borderPosition.m_width = width;
  m_borderPosition.m_height = height;

  m_contentPosition.m_row = m_borderPosition.m_row + 1;
  m_contentPosition.m_col = m_borderPosition.m_col + 1;
  m_contentPosition.m_width = m_borderPosition.m_width - 2;
  m_contentPosition.m_height = m_borderPosition.m_height - 2;

  // top border
  {
    // top-left
    if (Error::eSuccess != moveCursorInBorderArea(row, col++)) {
      return Error::eError;
    }
    Terminal::writeIcon(Icons::topLeft);

    // top
    for (uint32_t i = col; i < m_borderPosition.m_width; i++) {
      if (moveCursorInBorderArea(row, col++) != Error::eSuccess) {
        return Error::eError;
      }
      Terminal::writeIcon(Icons::top);
    }

    // top-right
    if (moveCursorInBorderArea(row, col++) != Error::eSuccess) {
      return Error::eError;
    }
    Terminal::writeIcon(Icons::topRight);

    // go to beginning of new line
    col = m_borderPosition.m_col;
    if (moveCursorInBorderArea(row++, col) != Error::eSuccess) {
      return Error::eError;
    }
    Terminal::goTostartOfLine();
    Terminal::makeNewLine();
  }

  // content border
  for (uint32_t i = 1; i < m_borderPosition.m_height - 1; i++) {
    if (drawBorderContent(row, col) != Error::eSuccess) {
      return Error::eError;
    }
  }

  // bottom border
  {
    // bottom-left
    if (moveCursorInBorderArea(row, col++) != Error::eSuccess) {
      return Error::eError;
    }
    Terminal::writeIcon(Icons::bottomLeft);

    // bottom
    for (uint32_t i = col; i < m_borderPosition.m_width; i++) {
      if (moveCursorInBorderArea(row, col++) != Error::eSuccess) {
        return Error::eError;
      }
      Terminal::writeIcon(Icons::bottom);
    }

    // bottom-right
    if (moveCursorInBorderArea(row, col++) != Error::eSuccess) {
      return Error::eError;
    }
    Terminal::writeIcon(Icons::bottomRight);

    // go to beginning of new line
    col = m_borderPosition.m_col;
    if (moveCursorInBorderArea(row++, col) != Error::eSuccess) {
      return Error::eError;
    }
    Terminal::goTostartOfLine();
    Terminal::makeNewLine();
  }
  return Error::eSuccess;
}

ush::Error ush::Box::clearBoxContent(void)
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
  for (size_t currentRow = m_contentPosition.m_row; currentRow < m_contentPosition.m_height; currentRow++) {
    if (clearLine(currentRow, m_contentPosition.m_col) != Error::eSuccess) {
      return Error::eError;
    }
  }
  return Error::eSuccess;
#endif
}
 
ush::Error ush::Box::clearLine(uint32_t row, uint32_t col)
{
  if (moveCursorInContentArea(row, col) != Error::eSuccess) {
    return Error::eError;
  }
  Terminal::goTostartOfLine();
  Terminal::eraseEntireLine();

  // re-draw current row
  if (drawBorderContent(row, col) != Error::eSuccess) {
    return Error::eError;
  }

  return Error::eSuccess;
}

ush::Error ush::Box::writeSpace()
{
  if (moveCursorInContentArea(m_contentPosition.m_row, m_contentPosition.m_col++) != Error::eSuccess) {
    return Error::eError;
  }
  Terminal::writeSpace();
  return Error::eSuccess;
}

ush::Error ush::Box::writeNewLine()
{
  if (moveCursorInContentArea(m_contentPosition.m_row++, m_contentPosition.m_col) != Error::eSuccess) {
    return Error::eError;
  }
  Terminal::makeNewLine();
  return Error::eSuccess;
}

ush::Error ush::Box::writeIcon(const char8_t* iconName)
{
  if (moveCursorInContentArea(m_contentPosition.m_row, m_contentPosition.m_col++) != Error::eSuccess) {
    return Error::eError;
  }
  Terminal::writeIcon(iconName);
  return Error::eSuccess;
}

ush::Error ush::Box::writeText(const char* name, size_t size)
{
  if (moveCursorInContentArea(m_contentPosition.m_row, m_contentPosition.m_col) != Error::eSuccess) {
    return Error::eError;
  }
  m_contentPosition.m_col += size;

  Terminal::writeText(name, size);
  return Error::eSuccess;
}

ush::Error ush::Box::drawBorderContent(uint32_t& row, uint32_t& col)
{
  // left
  if(moveCursorInBorderArea(row, col++) != Error::eSuccess) {
    return Error::eError;
  }
  Terminal::writeIcon(Icons::left);

  // right
  col = col + (m_borderPosition.m_width - 2);
  if (moveCursorInBorderArea(row, col) != Error::eSuccess) {
    return Error::eError;
  }

  if (moveCursorInBorderArea(row, col++) != Error::eSuccess) {
    return Error::eError;
  }
  Terminal::writeIcon(Icons::right);

  // go to beginning of new line
  col = m_borderPosition.m_col;
  if (moveCursorInBorderArea(row++, col) != Error::eSuccess) {
    return Error::eError;
  }
  Terminal::goTostartOfLine();
  Terminal::makeNewLine();

  return Error::eSuccess;
}
