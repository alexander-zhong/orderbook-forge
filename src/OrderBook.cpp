#include "Orderbook.h"

void OrderBook::insert(const Order& order) {
  if (order.side == Side::BUY) {
    PriceLevel& level = bids_[order.price];

    level.push_back(order);

    order_index_[order.id] = OrderLocation{order.side, order.price, std::prev(level.end())};
  } else {
    PriceLevel& level = asks_[order.price];
    level.push_back(order);
    order_index_[order.id] = OrderLocation{order.side, order.price, std::prev(level.end())};
  }
}

bool OrderBook::cancel(uint64_t order_id) {
  auto it = order_index_.find(order_id);
  if (it == order_index_.end()) return false;  // does not exist

  const OrderLocation& loc = it->second;

  if (loc.side == Side::BUY) {
    bids_[loc.price].erase(loc.position);
  } else {
    asks_[loc.price].erase(loc.position);
  }

  order_index_.erase(it);

  return true;
}
