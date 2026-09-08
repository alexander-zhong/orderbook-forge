# orderbook-forge

A limit order book and matching engine written in C++. Financial Markets are the most DEMANDING for low-latency software where microseconds decide outcomes. This project was built to explore lowest-latency design, starting off with a simple matching as a baseline and then iteratively reducing latency through successive redesigns.

## What it does

`orderbook-forge` maintains a book of resting **bids** (buy orders) and **asks** (sell
orders) in price-time priority, and matches incoming orders against it to produce trades.
An incoming order is matched **synchronously** on submission: it crosses against the
opposite side as far as it can, and any unfilled remainder either rests in the book or is
discarded, depending on its type.

## Order types

- **LIMIT** — has a price limit. Fills only at prices at least as good as the limit
  (a buy fills at `<=` its price, a sell at `>=` its price). Any unfilled quantity **rests
  in the book** as a standing order at its limit price.
- **MARKET** — no price limit. Fills immediately against the best available prices until
  filled or the opposite side is empty. Any unfilled quantity is **discarded** (a market
  order never rests, because it has no price to rest at).

## Matching rules

1. **Price priority.** Incoming orders match the best price on the opposite side first:
   the highest bid, or the lowest ask.
2. **Time priority.** Within a single price level, the oldest resting order fills first
   (FIFO).
3. **Trade price is the maker's price.** A trade executes at the price of the resting
   (maker) order that was already on the book, not the incoming (taker) order's price.
4. **Partial fills.**
   - A LIMIT order that fills only partially rests its **remaining quantity** in the book.
   - A MARKET order that fills only partially **discards** its remaining quantity.
5. **Marketable limit orders.** A LIMIT order whose price crosses the opposite side fills
   what it can at acceptable prices, then rests any remainder at its limit price.
6. **No cross, no fill.** A LIMIT order whose price does not cross the opposite side does
   not trade; it rests in the book immediately.
7. **Empty book.** A MARKET order submitted against an empty opposite side fills nothing
   and is discarded. A LIMIT order simply rests.

## Invariants

- Price levels never sit empty: when the last order at a price is removed (by cancel or a
  full fill), that price level is deleted from the book.
- Every live order has exactly one entry in the id index, so lookups and cancels are O(1)
  average.

## Out of scope (for now)

- Concurrency / an inbound order queue (matching is single-threaded and synchronous).
- Advanced order types (IOC, FOK, stop, iceberg, post-only).
- Self-trade prevention and any notion of accounts.
- Duplicate-id and zero-quantity rejection (callers are assumed to submit valid,
  uniquely-identified orders).

## Building and testing

```bash
cmake -B build
cmake --build build
ctest --test-dir build --output-on-failure
```
