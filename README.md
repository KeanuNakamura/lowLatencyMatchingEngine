# Low-Latency Matching Engine

A C++ matching engine that simulates the core order-matching logic used by electronic exchanges. The engine supports multiple stock symbols, independent order books per symbol, limit orders, market orders, cancellations, trade generation, and latency benchmarking.

This project is designed to demonstrate low-level C++ data structure design, performance-aware programming, and exchange-style matching logic.

## Features

- Supports multiple stock symbols
- Maintains one independent `OrderBook` per symbol
- Supports buy and sell limit orders
- Supports buy and sell market orders
- Supports order cancellation by symbol and order ID
- Matches orders using price-time priority
- Tracks resting orders for fast cancellation
- Generates trade records when orders match
- Includes benchmark code for throughput and latency measurement
- Reports average, p50, p95, p99, p99.9, p99.99, and max latency
- Supports optional hash map capacity reservation to reduce rehashing spikes

## Project Structure

```txt
lowlatency/
├── include/
│   ├── Order.h
│   ├── Trade.h
│   ├── OrderBook.h
│   └── MatchingEngine.h
│
├── src/
│   ├── OrderBook.cpp
│   ├── MatchingEngine.cpp
│   ├── main.cpp
│   └── benchmark.cpp
│
├── tests.cpp
└── README.md
```

## Core Types

The engine uses simple type aliases for order-related data:

```cpp
using OrderId = std::uint64_t;
using Price = std::int64_t;
using Quantity = std::uint64_t;
using Timestamp = std::uint64_t;
```

Orders are represented as:

```cpp
struct Order {
    Side side;
    OrderType type;
    OrderId id;
    Price price;
    Quantity quantity;
    Timestamp timestamp;
};
```

Trades are represented as:

```cpp
struct Trade {
    OrderId restingOrderId;
    OrderId incomingOrderId;
    Price price;
    Quantity quantity;
};
```

## Matching Logic

Each stock symbol has its own independent order book.

```cpp
std::unordered_map<std::string, OrderBook> books;
```

Inside each `OrderBook`:

- Buy orders are stored in descending price order.
- Sell orders are stored in ascending price order.
- Orders at the same price level are matched FIFO.
- An `order_locations` hash map tracks where each resting order is stored, allowing fast cancellation.

```cpp
std::map<Price, OrderList, std::greater<Price>> bids;
std::map<Price, OrderList> asks;
std::unordered_map<OrderId, OrderLocation> order_locations;
```

## Price-Time Priority

The engine follows price-time priority:

1. Better prices are matched first.
2. For orders at the same price, older orders are matched first.

For example, if two sell orders are resting at the same price:

```txt
AAPL 1 SELL LIMIT 100 10
AAPL 2 SELL LIMIT 100 10
```

and a buy order arrives:

```txt
AAPL 3 BUY LIMIT 100 15
```

then order `1` is filled before order `2`.

## Input Format

The file-based driver accepts input in the following format:

```txt
SYMBOL ID BUY/SELL LIMIT PRICE QUANTITY
SYMBOL CANCEL ID
```

Example:

```txt
AAPL 1 BUY LIMIT 100 10
AAPL 2 SELL LIMIT 105 20
AAPL 3 SELL LIMIT 100 5
AAPL CANCEL 1
MSFT 4 BUY LIMIT 250 10
MSFT 5 SELL LIMIT 250 6
```

## Example Output

For matching orders, the engine prints trades:

```txt
TRADE symbol=AAPL resting=1 incoming=3 price=100 quantity=5
TRADE symbol=MSFT resting=4 incoming=5 price=250 quantity=6
```

For cancellations:

```txt
CANCELLED symbol=AAPL id=1
CANCEL_REJECTED symbol=AAPL id=999
```

## Benchmarking

The benchmark simulates millions of randomized order and cancel events.

Example benchmark configuration:

```cpp
constexpr int NUM_EVENTS = 10'000'000;

std::vector<std::string> symbols{
    "AAPL",
    "MSFT",
    "GOOG",
    "AMZN",
    "NVDA"
};
```

The benchmark reports:

- Events processed
- Orders submitted
- Cancel attempts
- Successful cancels
- Trades generated
- Total runtime
- Throughput
- Average latency
- p50 latency
- p95 latency
- p99 latency
- p99.9 latency
- p99.99 latency
- Max latency

Example output:

```txt
Events processed: 10000000
Symbols: 5
Symbol list: AAPL MSFT GOOG AMZN NVDA
Orders submitted: 8999476
Cancel attempts: 1000524
Successful cancels: 198205
Trades generated: 6982962
Total time: 2.2108 seconds
Throughput: 4.52324e+06 events/sec
Average latency: 221.08 ns/event
p50 latency: 125 ns
p95 latency: 708 ns
p99 latency: 1833 ns
p999 latency: 3542 ns
p9999 latency: 6125 ns
Max latency: 42042 ns
```

## Hash Map Reservation

The matching engine can reserve hash map capacity for the top-level symbol map and for each order book’s `order_locations` map.

This reduces expensive `unordered_map` rehashing during benchmark runs.

Example:

```cpp
MatchingEngine engine(symbols.size());

std::size_t expected_orders_per_symbol =
    NUM_EVENTS / symbols.size();

for (const std::string& symbol : symbols) {
    engine.addSymbol(symbol, expected_orders_per_symbol);
}
```

Benchmark results showed that reserving capacity does not significantly change average latency, but it can reduce worst-case latency spikes.

## Unit Tests

The project can be tested with GoogleTest. The tests cover:

- Adding limit buy orders
- Adding limit sell orders
- Matching buy and sell orders
- Partial fills
- FIFO behavior at the same price
- Cancellations
- Rejected cancellations
- Symbol isolation
- Market orders

## Building and Running

### Main Program

Compile the main program:

```bash
g++ -std=c++17 -O3 -Iinclude \
    src/main.cpp src/MatchingEngine.cpp src/OrderBook.cpp \
    -o matching_engine
```

Run with an input file:

```bash
./matching_engine input.txt
```

### Benchmark

Compile the benchmark:

```bash
g++ -std=c++17 -O3 -Iinclude \
    src/benchmark.cpp src/MatchingEngine.cpp src/OrderBook.cpp \
    -o benchmark
```

Run the benchmark:

```bash
./benchmark
```

### Unit Tests

Compile the tests:

```bash
g++ -std=c++17 -O3 -Iinclude \
    tests.cpp src/MatchingEngine.cpp src/OrderBook.cpp \
    -lgtest -lgtest_main -pthread \
    -o tests
```

Run the tests:

```bash
./tests
```

