#include "repl.h"

int main(int argc, char** argv)
{
  ush::Repl repl;
  int result = repl.loop();

  return result;
}
