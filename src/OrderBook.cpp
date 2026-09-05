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
    if (bids_[loc.price].empty()) {
      bids_.erase(loc.price);
    }
  } else {
    asks_[loc.price].erase(loc.position);
    if (asks_[loc.price].empty()) {
      asks_.erase(loc.price);
    }
  }

  order_index_.erase(it);

  return true;
}

std::optional<double> OrderBook::bestBid() const {
  if (bids_.empty()) return std::nullopt;
  return bids_.begin()->first;
}

std::optional<double> OrderBook::bestAsk() const {
  if (asks_.empty()) return std::nullopt;
  return asks_.begin()->first;
}

size_t OrderBook::size() const { return order_index_.size(); }

bool OrderBook::empty() const { return (order_index_.size() == 0); }

uint64_t OrderBook::quantityAtPrice(Side side, double price) const {
  uint64_t total_quantity = 0;

  const PriceLevel* level = nullptr;
  if (side == Side::BUY) {
    auto it = bids_.find(price);
    if (it != bids_.end()) level = &it->second;
  } else if (side == Side::SELL) {
    auto it = asks_.find(price);
    if (it != asks_.end()) level = &it->second;
  }

  if (level) {
    for (const Order& o : *level) {
      total_quantity += o.quantity;
    }
  }

  return total_quantity;
}