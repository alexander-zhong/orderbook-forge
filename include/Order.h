#pragma once

#include <cstdint>

// The two sides of the book
enum class Side { BUY, SELL };

// LIMIT: rests in the book if it doesn't get matched immediately
// MARKET: Always matches immediately against the best available price(s)
enum class OrderType { LIMIT, MARKET };

struct Order {
  uint64_t id;
  Side side;
  double price;  // this is ignored for market orders
  uint64_t quantity;
  uint64_t timestamp;
  OrderType type;

  bool isFilled() const { return quantity == 0; }
};
