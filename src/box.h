#ifndef USH_BOX_H
#define USH_BOX_H

#include "error.h"

#include <cstdint>
#include <cstddef>

namespace ush
{
  class Box 
  {
    public:
      class Position{
        public:
          // Box::m_row + 1 --> because of top border
          uint32_t m_row;

          // Box::m_col + 1 --> because of left border
          uint32_t m_col;

          // Box::m_width - 2 --> because of left and right borders
          uint32_t m_width;

          // Box::m_height - 2 --> because of top and bottom borders
          uint32_t m_height;
      };
      Position m_borderPosition;

      Position m_contentPosition;
 
      Error moveCursorInBorderArea(uint32_t row, uint32_t col);

      Error moveCursorInContentArea(uint32_t row, uint32_t col);

      Error drawBorder(uint32_t row, uint32_t col, uint32_t width, uint32_t height);

      Error writeSpace();

      Error writeNewLine();

      Error writeIcon(const char8_t* iconName);

      Error writeText(const char* name, size_t size);

      Error clearLine(uint32_t row, uint32_t col);

      Error clearBoxContent(void);

    private:
      Error drawBorderContent(uint32_t& row, uint32_t& col);
  };
}
#endif // USH_BOX_H
