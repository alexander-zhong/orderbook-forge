#include <gtest/gtest.h>

#include "Orderbook.h"

namespace {

Order makeOrder(uint64_t id, Side side, double price, uint64_t quantity) {
  return Order{id, side, price, quantity, 0, OrderType::LIMIT};
}

Order makeMarket(uint64_t id, Side side, uint64_t quantity) {
  return Order{id, side, 0.0, quantity, 0, OrderType::MARKET};
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

TEST(MatchingTest, FullFillRemovesBothOrders) {
  OrderBook book;
  book.insert(makeOrder(1, Side::SELL, 100.0, 10));

  auto trades = book.submit(makeOrder(2, Side::BUY, 100.0, 10));

  ASSERT_EQ(trades.size(), 1u);
  EXPECT_EQ(trades[0].maker_id, 1u);
  EXPECT_EQ(trades[0].taker_id, 2u);
  EXPECT_DOUBLE_EQ(trades[0].price, 100.0);
  EXPECT_EQ(trades[0].quantity, 10u);

  EXPECT_TRUE(book.empty());
  EXPECT_FALSE(book.bestAsk().has_value());
  EXPECT_FALSE(book.bestBid().has_value());
}

TEST(MatchingTest, PartialFillLeavesRestingRemainder) {
  OrderBook book;
  book.insert(makeOrder(1, Side::SELL, 100.0, 100));

  auto trades = book.submit(makeOrder(2, Side::BUY, 100.0, 30));

  ASSERT_EQ(trades.size(), 1u);
  EXPECT_EQ(trades[0].quantity, 30u);

  EXPECT_EQ(book.size(), 1u);
  EXPECT_EQ(book.quantityAtPrice(Side::SELL, 100.0), 70u);
  ASSERT_TRUE(book.bestAsk().has_value());
  EXPECT_DOUBLE_EQ(book.bestAsk().value(), 100.0);
  EXPECT_FALSE(book.bestBid().has_value());
}

TEST(MatchingTest, IncomingLimitRemainderRestsInBook) {
  OrderBook book;
  book.insert(makeOrder(1, Side::SELL, 100.0, 30));

  auto trades = book.submit(makeOrder(2, Side::BUY, 100.0, 100));

  ASSERT_EQ(trades.size(), 1u);
  EXPECT_EQ(trades[0].quantity, 30u);

  EXPECT_EQ(book.size(), 1u);
  ASSERT_TRUE(book.bestBid().has_value());
  EXPECT_DOUBLE_EQ(book.bestBid().value(), 100.0);
  EXPECT_EQ(book.quantityAtPrice(Side::BUY, 100.0), 70u);
  EXPECT_FALSE(book.bestAsk().has_value());
}

TEST(MatchingTest, WalksMultiplePriceLevels) {
  OrderBook book;
  book.insert(makeOrder(1, Side::SELL, 100.0, 10));
  book.insert(makeOrder(2, Side::SELL, 101.0, 10));

  auto trades = book.submit(makeOrder(3, Side::BUY, 101.0, 15));

  ASSERT_EQ(trades.size(), 2u);
  EXPECT_EQ(trades[0].maker_id, 1u);
  EXPECT_DOUBLE_EQ(trades[0].price, 100.0);
  EXPECT_EQ(trades[0].quantity, 10u);
  EXPECT_EQ(trades[1].maker_id, 2u);
  EXPECT_DOUBLE_EQ(trades[1].price, 101.0);
  EXPECT_EQ(trades[1].quantity, 5u);

  EXPECT_EQ(book.size(), 1u);
  ASSERT_TRUE(book.bestAsk().has_value());
  EXPECT_DOUBLE_EQ(book.bestAsk().value(), 101.0);
  EXPECT_EQ(book.quantityAtPrice(Side::SELL, 101.0), 5u);
}

TEST(MatchingTest, NoCrossRestsWithoutTrading) {
  OrderBook book;
  book.insert(makeOrder(1, Side::SELL, 101.0, 10));

  auto trades = book.submit(makeOrder(2, Side::BUY, 99.0, 10));

  EXPECT_TRUE(trades.empty());
  EXPECT_EQ(book.size(), 2u);
  ASSERT_TRUE(book.bestBid().has_value());
  EXPECT_DOUBLE_EQ(book.bestBid().value(), 99.0);
  ASSERT_TRUE(book.bestAsk().has_value());
  EXPECT_DOUBLE_EQ(book.bestAsk().value(), 101.0);
}

TEST(MatchingTest, TradeExecutesAtMakerPrice) {
  OrderBook book;
  book.insert(makeOrder(1, Side::SELL, 100.0, 10));

  auto trades = book.submit(makeOrder(2, Side::BUY, 105.0, 10));

  ASSERT_EQ(trades.size(), 1u);
  EXPECT_DOUBLE_EQ(trades[0].price, 100.0);
}

TEST(MatchingTest, MarketOrderFillsThenDiscardsRemainder) {
  OrderBook book;
  book.insert(makeOrder(1, Side::SELL, 100.0, 30));

  auto trades = book.submit(makeMarket(2, Side::BUY, 100));

  ASSERT_EQ(trades.size(), 1u);
  EXPECT_EQ(trades[0].quantity, 30u);
  EXPECT_DOUBLE_EQ(trades[0].price, 100.0);

  EXPECT_TRUE(book.empty());
  EXPECT_FALSE(book.bestBid().has_value());
}

TEST(MatchingTest, MarketOrderIntoEmptyBookDoesNothing) {
  OrderBook book;

  auto trades = book.submit(makeMarket(1, Side::BUY, 50));

  EXPECT_TRUE(trades.empty());
  EXPECT_TRUE(book.empty());
}

TEST(MatchingTest, TimePriorityOldestRestingFillsFirst) {
  OrderBook book;
  book.insert(makeOrder(1, Side::SELL, 100.0, 10));
  book.insert(makeOrder(2, Side::SELL, 100.0, 10));

  auto trades = book.submit(makeOrder(3, Side::BUY, 100.0, 10));

  ASSERT_EQ(trades.size(), 1u);
  EXPECT_EQ(trades[0].maker_id, 1u);

  EXPECT_EQ(book.quantityAtPrice(Side::SELL, 100.0), 10u);
  EXPECT_FALSE(book.cancel(1));
  EXPECT_TRUE(book.cancel(2));
}

TEST(MatchingTest, FullyFilledLevelIsRemovedAndBestAskAdvances) {
  OrderBook book;
  book.insert(makeOrder(1, Side::SELL, 100.0, 10));
  book.insert(makeOrder(2, Side::SELL, 101.0, 10));

  ASSERT_TRUE(book.bestAsk().has_value());
  EXPECT_DOUBLE_EQ(book.bestAsk().value(), 100.0);

  book.submit(makeOrder(3, Side::BUY, 100.0, 10));

  ASSERT_TRUE(book.bestAsk().has_value());
  EXPECT_DOUBLE_EQ(book.bestAsk().value(), 101.0);
}
