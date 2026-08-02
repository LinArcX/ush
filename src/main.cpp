#include "repl.h"
#include "terminal.h"

int main(int argc, char** argv)
{
  if (ush::Error::eSuccess != ush::Terminal::requestGetTerminalWindowSize()) {
    return -1;
  }

  ush::Repl repl;
  if (ush::Error::eSuccess != repl.init()) {
    return -1;
  }

  return repl.loop();
}
