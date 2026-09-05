#pragma once

#include <cstdint>
#include <list>
#include <map>
#include <optional>
#include <unordered_map>

#include "Order.h"

class OrderBook {
 public:
  // insert a resting order into the book and assume it does not cross
  // the matching machine is responsible for matching before inserting
  void insert(const Order& order);

  // Remove the resting order by the id - bool on success/failure
  bool cancel(uint64_t order_id);

  std::optional<double> bestBid() const;
  std::optional<double> bestAsk() const;

  bool empty() const;
  size_t size() const;

  uint64_t quantityAtPrice(Side side, double price) const;

 private:
  // front = oldest = highest priority
  using PriceLevel = std::list<Order>;

  std::map<double, PriceLevel, std::greater<double>> bids_;
  std::map<double, PriceLevel, std::less<double>> asks_;

  struct OrderLocation {
    Side side;
    double price;
    PriceLevel::iterator position;
  };
  std::unordered_map<uint64_t, OrderLocation> order_index_;
};