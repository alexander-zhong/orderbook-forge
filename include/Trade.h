#pragma once

#include <cstdint>

struct Trade {
  uint64_t maker_id;
  uint64_t taker_id;
  double price;
  uint64_t quantity;
};
