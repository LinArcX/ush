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
      uint32_t m_row; // top horizontal border, draws here
 
      uint32_t m_col; // left vertical border, draws here 

      uint32_t m_width; // right vertical border, draws here
      
      uint32_t m_height; // bottom horizontal border, draws here

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
      Position m_position;
 
      Error moveCursor(uint32_t row, uint32_t col);

      Error draw(uint32_t x, uint32_t y, uint32_t width, uint32_t height);

      Error drawContent(uint32_t& row, uint32_t& col);

      Error clearBox(void);

      Error clearLine(uint32_t row, uint32_t col);

      Error writeSpace();

      Error writeNewLine();

      Error writeIcon(const char8_t* iconName);

      Error writeText(const char* name, size_t size);

    private:
  };
}
#endif // USH_BOX_H

//#include "terminal.h"
//#include <termio.h>
//Terminal terminal;
//struct Rect {
//  uint16_t x;
//  uint16_t y;
//  uint16_t width;
//  uint16_t height;
//};
