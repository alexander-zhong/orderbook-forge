#include <gtest/gtest.h>

#include "Orderbook.h"

namespace {

Order makeOrder(uint64_t id, Side side, double price, uint64_t quantity) {
  return Order{id, side, price, quantity, 0, OrderType::LIMIT};
}

}  // namespace

TEST(OrderBookTest, EmptyBookHasNoOrders) {
  OrderBook book;
  EXPECT_TRUE(book.empty());
  EXPECT_EQ(book.size(), 0u);
  EXPECT_FALSE(book.bestBid().has_value());
  EXPECT_FALSE(book.bestAsk().has_value());
}

TEST(OrderBookTest, InsertIncreasesSize) {
  OrderBook book;
  book.insert(makeOrder(1, Side::BUY, 100.0, 10));
  EXPECT_FALSE(book.empty());
  EXPECT_EQ(book.size(), 1u);
}

TEST(OrderBookTest, BestBidIsHighestBuyPrice) {
  OrderBook book;
  book.insert(makeOrder(1, Side::BUY, 100.0, 10));
  book.insert(makeOrder(2, Side::BUY, 101.0, 10));
  book.insert(makeOrder(3, Side::BUY, 99.5, 10));

  ASSERT_TRUE(book.bestBid().has_value());
  EXPECT_DOUBLE_EQ(book.bestBid().value(), 101.0);
}

TEST(OrderBookTest, BestAskIsLowestSellPrice) {
  OrderBook book;
  book.insert(makeOrder(1, Side::SELL, 103.0, 10));
  book.insert(makeOrder(2, Side::SELL, 102.0, 10));
  book.insert(makeOrder(3, Side::SELL, 104.0, 10));

  ASSERT_TRUE(book.bestAsk().has_value());
  EXPECT_DOUBLE_EQ(book.bestAsk().value(), 102.0);
}

TEST(OrderBookTest, QuantityAtPriceSumsAllOrdersAtThatPrice) {
  OrderBook book;
  book.insert(makeOrder(1, Side::BUY, 100.0, 10));
  book.insert(makeOrder(2, Side::BUY, 100.0, 25));
  book.insert(makeOrder(3, Side::BUY, 101.0, 7));

  EXPECT_EQ(book.quantityAtPrice(Side::BUY, 100.0), 35u);
  EXPECT_EQ(book.quantityAtPrice(Side::BUY, 101.0), 7u);
}

TEST(OrderBookTest, QuantityAtPriceIsZeroWhenPriceHasNoOrders) {
  OrderBook book;
  book.insert(makeOrder(1, Side::SELL, 102.0, 10));

  EXPECT_EQ(book.quantityAtPrice(Side::SELL, 999.0), 0u);
  EXPECT_EQ(book.quantityAtPrice(Side::BUY, 102.0), 0u);
}

TEST(OrderBookTest, CancelRemovesOrder) {
  OrderBook book;
  book.insert(makeOrder(1, Side::BUY, 100.0, 10));

  EXPECT_TRUE(book.cancel(1));
  EXPECT_TRUE(book.empty());
  EXPECT_EQ(book.size(), 0u);
}

TEST(OrderBookTest, CancelReturnsFalseForUnknownId) {
  OrderBook book;
  book.insert(makeOrder(1, Side::BUY, 100.0, 10));

  EXPECT_FALSE(book.cancel(999));
  EXPECT_EQ(book.size(), 1u);
}

TEST(OrderBookTest, CancelLastOrderAtPriceUpdatesBestBid) {
  OrderBook book;
  book.insert(makeOrder(1, Side::BUY, 100.0, 10));
  book.insert(makeOrder(2, Side::BUY, 101.0, 10));

  ASSERT_TRUE(book.bestBid().has_value());
  EXPECT_DOUBLE_EQ(book.bestBid().value(), 101.0);

  EXPECT_TRUE(book.cancel(2));

  ASSERT_TRUE(book.bestBid().has_value());
  EXPECT_DOUBLE_EQ(book.bestBid().value(), 100.0);
}

TEST(OrderBookTest, MultipleOrdersAtSamePriceCountTowardSize) {
  OrderBook book;
  book.insert(makeOrder(1, Side::BUY, 100.0, 10));
  book.insert(makeOrder(2, Side::BUY, 100.0, 10));
  book.insert(makeOrder(3, Side::BUY, 100.0, 10));

  EXPECT_EQ(book.size(), 3u);
  EXPECT_EQ(book.quantityAtPrice(Side::BUY, 100.0), 30u);
}
