#include "repl.h"
#include "terminal.h"

int main(int argc, char** argv)
{
  int result = -1;

  if (ush::Error::eSuccess == ush::Terminal::requestGetTerminalWindowSize()) {
    ush::Repl repl;
    result = repl.loop();
  }

  return result;
}
