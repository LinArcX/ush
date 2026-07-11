#include "repl.h"

int main(int argc, char** argv)
{
  ush::Repl repl;
  // Load config files, if any.

  repl.enableRawMode();
  int result = repl.loop();
  repl.disableRawMode();

  return result;
}
