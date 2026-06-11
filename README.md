# Low-Latency Matching Engine

A C++ matching engine that simulates the core order-matching logic used by electronic exchanges. The engine supports multiple stock symbols, independent order books per symbol, limit and market orders, cancellations, trade generation, and latency benchmarking (p50 - p99, max).

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

By default, the benchmark runs in **symbol-parallel mode** for HPC throughput:

1. All events are pre-generated with a fixed RNG seed.
2. Events are partitioned by symbol while preserving per-symbol order.
3. Each worker thread owns its own `OrderBook` instances (constructed in-thread for NUMA first-touch).
4. Parallelism uses `std::thread` by default; pass `--openmp` to use OpenMP instead.

Symbols are independent, so this parallelization is lock-free and scales with symbol count up to the number of worker threads. Use `--symbols` to increase the number of independent books on large nodes, and `--threads` to control worker count.

Per-event latency sampling is **disabled by default** in parallel mode because timing every event adds significant overhead at scale. Use `--latency` when you need percentile stats. Use `--sequential` for the original single-threaded interleaved event loop.

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

Compile the benchmark (std::thread parallelism; add `-fopenmp` only if you want `--openmp`):

```bash
g++ -std=c++17 -O3 -pthread -Iinclude \
    benchmark/benchmark.cpp src/MatchingEngine.cpp src/OrderBook.cpp \
    -o benchmark_runner
```

Run the default symbol-parallel benchmark:

```bash
./benchmark_runner
```

Run with more symbols and explicit thread count for HPC scaling:

```bash
./benchmark_runner --symbols 64 --threads 32
```

Run the original single-threaded latency profile:

```bash
./benchmark_runner --sequential
```

Record per-event latency percentiles in parallel mode:

```bash
./benchmark_runner --latency
```

Optional OpenMP backend (rebuild with `-fopenmp`):

```bash
g++ -std=c++17 -O3 -fopenmp -pthread -Iinclude \
    benchmark/benchmark.cpp src/MatchingEngine.cpp src/OrderBook.cpp \
    -o benchmark_runner_omp

./benchmark_runner_omp --openmp --symbols 64 --threads 32
```

On HPC nodes, bind threads to cores when using OpenMP:

```bash
export OMP_PROC_BIND=close
export OMP_PLACES=cores
```


### Unit Tests

Compile the tests:

```bash
g++ -std=c++17 -O3 -Iinclude \
    -I/opt/homebrew/include \
    tests/tests.cpp src/MatchingEngine.cpp src/OrderBook.cpp \
    -L/opt/homebrew/lib \
    -lgtest -lgtest_main -pthread \
    -o tests_runner
```

