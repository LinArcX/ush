#ifndef USH_BOX_H
#define USH_BOX_H

#include "error.h"
#include <cstdint>

namespace ush
{
  class Box 
  {
    public:
      uint32_t m_col;

      uint32_t m_row;

      uint32_t m_width;

      uint32_t m_height;

      class Position{
        public:
          uint32_t m_row;
          uint32_t m_col;
      };

      Position m_position;
 
      Error moveCursor(uint32_t row, uint32_t col);

      Error draw(uint32_t x, uint32_t y, uint32_t width, uint32_t height);

      Error drawContent(uint32_t& row, uint32_t& col);

      Error clearBox(void);

      Error clearLine(uint32_t row, uint32_t col);

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
