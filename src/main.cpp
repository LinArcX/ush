#include "repl.h"

int main(int argc, char** argv)
{
  ush::Repl repl;
  // Load config files, if any.

  // Run command loop.
  return repl.loop();
}
