#include "commands.h"

const struct command command_table[] = {
  {0, 1210, 0.11, 0.2} // example command
};

const uint32_t command_table_len = sizeof(command_table) / sizeof(command_table[0]);