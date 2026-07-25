#ifndef USH_BOX_H
#define USH_BOX_H

#include <cstdint>

namespace ush
{
  class Box 
  {
    public:
      uint32_t m_x;

      uint32_t m_y;

      uint32_t m_width;

      uint32_t m_height;
 
      void moveCursor(uint32_t row, uint32_t col);

      void draw(uint32_t x, uint32_t y, uint32_t width, uint32_t height);

      void clearBox(void);

      void clearLine(void);

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
