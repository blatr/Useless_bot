#include "bot.h"
#include <iostream>

int main(int argc, char **argv) {
  MyBot bot(argv[0], argv[1]);
  bot.Run();
}