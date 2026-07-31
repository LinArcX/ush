#include "repl.h"
#include "terminal.h"

int main(int argc, char** argv)
{
  int result = -1;

  if (ush::Error::eSuccess != ush::Terminal::requestGetTerminalWindowSize()) {
    return result;
  }

  ush::Repl repl;
  if (ush::Error::eSuccess != repl.init()) {
    return result;
  }

  result = repl.loop();
  return result;
}
