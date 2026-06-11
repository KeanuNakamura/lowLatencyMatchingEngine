#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#ifdef _OPENMP
#include <omp.h>
#endif

#include "MatchingEngine.h"
#include "OrderBook.h"

namespace {

enum class EventType {
    Submit,
    Cancel
};

enum class ParallelBackend {
    StdThread,
    OpenMP
};

struct BenchmarkEvent {
    EventType type;
    SymbolId symbol_id;
    OrderId order_id;
    Side side;
    Price price;
    Quantity quantity;
};

struct BenchmarkConfig {
    int num_events = 50'000'000;
    int num_symbols = 5;
    int num_threads = 0;
    bool sequential = false;
    bool measure_latency = false;
    ParallelBackend backend = ParallelBackend::StdThread;
};

struct BenchmarkStats {
    std::size_t orders_submitted = 0;
    std::size_t cancels_submitted = 0;
    std::size_t successful_cancels = 0;
    std::size_t trades_generated = 0;

    void add(const BenchmarkStats& other) {
        orders_submitted += other.orders_submitted;
        cancels_submitted += other.cancels_submitted;
        successful_cancels += other.successful_cancels;
        trades_generated += other.trades_generated;
    }
};

struct SymbolWorkerState {
    OrderBook book;
    Timestamp current_time = 0;
};

void printUsage(const char* program) {
    std::cerr
        << "Usage: " << program
        << " [--events N] [--symbols N] [--threads N] [--sequential]"
        << " [--latency] [--openmp]\n"
        << "  --events N      Number of events to process (default: 50000000)\n"
        << "  --symbols N     Number of symbols to use (default: 5)\n"
        << "  --threads N     Worker threads for symbol-parallel mode\n"
        << "                  (default: min(hardware threads, num symbols))\n"
        << "  --sequential    Run single-threaded interleaved event loop\n"
        << "  --latency       Record per-event latency (off by default in parallel mode)\n"
        << "  --openmp        Use OpenMP instead of std::thread (default: std::thread)\n";
}

bool parseArgs(int argc, char* argv[], BenchmarkConfig& config) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--sequential") {
            config.sequential = true;
            config.measure_latency = true;
            continue;
        }

        if (arg == "--latency") {
            config.measure_latency = true;
            continue;
        }

#ifdef _OPENMP
        if (arg == "--openmp") {
            config.backend = ParallelBackend::OpenMP;
            continue;
        }
#endif

        if ((arg == "--events" || arg == "--symbols" || arg == "--threads")
            && i + 1 < argc) {
            int value = std::atoi(argv[++i]);

            if (value <= 0) {
                std::cerr << "Invalid value for " << arg << ": " << value << '\n';
                return false;
            }

            if (arg == "--events") {
                config.num_events = value;
            } else if (arg == "--symbols") {
                config.num_symbols = value;
            } else {
                config.num_threads = value;
            }
            continue;
        }

        std::cerr << "Unknown argument: " << arg << '\n';
        printUsage(argv[0]);
        return false;
    }

    return true;
}

std::vector<std::string> makeSymbols(int count) {
    static const std::vector<std::string> defaults{
        "AAPL", "MSFT", "GOOG", "AMZN", "NVDA"
    };

    std::vector<std::string> symbols;
    symbols.reserve(static_cast<std::size_t>(count));

    for (int i = 0; i < count; ++i) {
        if (i < static_cast<int>(defaults.size())) {
            symbols.push_back(defaults[static_cast<std::size_t>(i)]);
        } else {
            symbols.push_back("SYM" + std::to_string(i));
        }
    }

    return symbols;
}

std::vector<BenchmarkEvent> generateEvents(
    const std::vector<std::string>& symbols,
    const std::vector<SymbolId>& symbol_ids,
    int num_events
) {
    const int num_symbols = static_cast<int>(symbols.size());

    std::mt19937 rng(42);
    std::uniform_int_distribution<int> symbol_dist(0, num_symbols - 1);
    std::uniform_int_distribution<int> side_dist(0, 1);
    std::uniform_int_distribution<int> price_dist(9900, 10100);
    std::uniform_int_distribution<int> quantity_dist(1, 100);
    std::uniform_int_distribution<int> event_dist(1, 100);

    struct ActiveOrder {
        SymbolId symbol_id;
        OrderId order_id;
    };

    std::vector<ActiveOrder> active_orders;
    active_orders.reserve(static_cast<std::size_t>(num_events));

    std::vector<BenchmarkEvent> events;
    events.reserve(static_cast<std::size_t>(num_events));

    OrderId next_order_id = 1;

    for (int i = 0; i < num_events; ++i) {
        bool do_cancel = !active_orders.empty() && event_dist(rng) <= 10;

        if (do_cancel) {
            std::uniform_int_distribution<std::size_t> index_dist(
                0,
                active_orders.size() - 1
            );

            std::size_t index = index_dist(rng);
            const ActiveOrder& target = active_orders[index];

            events.push_back(BenchmarkEvent{
                EventType::Cancel,
                target.symbol_id,
                target.order_id,
                Side::Buy,
                0,
                0
            });

            active_orders[index] = active_orders.back();
            active_orders.pop_back();
        } else {
            SymbolId symbol_id = symbol_ids[static_cast<std::size_t>(symbol_dist(rng))];

            events.push_back(BenchmarkEvent{
                EventType::Submit,
                symbol_id,
                next_order_id,
                side_dist(rng) == 0 ? Side::Buy : Side::Sell,
                price_dist(rng),
                static_cast<Quantity>(quantity_dist(rng))
            });

            active_orders.push_back(ActiveOrder{symbol_id, next_order_id});
            ++next_order_id;
        }
    }

    return events;
}

std::vector<std::vector<BenchmarkEvent>> partitionBySymbol(
    const std::vector<BenchmarkEvent>& events,
    std::size_t num_symbols
) {
    std::vector<std::vector<BenchmarkEvent>> partitioned(num_symbols);
    std::vector<std::size_t> counts(num_symbols, 0);

    for (const BenchmarkEvent& event : events) {
        ++counts[event.symbol_id];
    }

    for (std::size_t symbol_id = 0; symbol_id < num_symbols; ++symbol_id) {
        partitioned[symbol_id].reserve(counts[symbol_id]);
    }

    for (const BenchmarkEvent& event : events) {
        partitioned[event.symbol_id].push_back(event);
    }

    return partitioned;
}

BenchmarkStats processSymbolEvents(
    SymbolWorkerState& worker,
    const std::vector<BenchmarkEvent>& events,
    std::vector<long long>* latencies_ns
) {
    BenchmarkStats stats;

    for (const BenchmarkEvent& event : events) {
        std::chrono::high_resolution_clock::time_point event_start;
        if (latencies_ns != nullptr) {
            event_start = std::chrono::high_resolution_clock::now();
        }

        if (event.type == EventType::Cancel) {
            bool cancelled = worker.book.cancelOrder(event.order_id);
            ++stats.cancels_submitted;

            if (cancelled) {
                ++stats.successful_cancels;
            }
        } else {
            Order order{
                event.side,
                OrderType::Limit,
                event.order_id,
                event.price,
                event.quantity,
                worker.current_time++
            };

            std::vector<Trade> trades = worker.book.addOrder(order);
            stats.trades_generated += trades.size();
            ++stats.orders_submitted;
        }

        if (latencies_ns != nullptr) {
            auto event_end = std::chrono::high_resolution_clock::now();
            latencies_ns->push_back(
                std::chrono::duration_cast<std::chrono::nanoseconds>(
                    event_end - event_start
                ).count()
            );
        }
    }

    return stats;
}

BenchmarkStats processSymbolEvents(
    MatchingEngine& engine,
    const std::vector<BenchmarkEvent>& events,
    std::vector<long long>* latencies_ns
) {
    BenchmarkStats stats;

    for (const BenchmarkEvent& event : events) {
        std::chrono::high_resolution_clock::time_point event_start;
        if (latencies_ns != nullptr) {
            event_start = std::chrono::high_resolution_clock::now();
        }

        if (event.type == EventType::Cancel) {
            bool cancelled = engine.cancelOrder(event.symbol_id, event.order_id);
            ++stats.cancels_submitted;

            if (cancelled) {
                ++stats.successful_cancels;
            }
        } else {
            Order order{
                event.side,
                OrderType::Limit,
                event.order_id,
                event.price,
                event.quantity,
                0
            };

            std::vector<Trade> trades = engine.submitOrder(event.symbol_id, order);
            stats.trades_generated += trades.size();
            ++stats.orders_submitted;
        }

        if (latencies_ns != nullptr) {
            auto event_end = std::chrono::high_resolution_clock::now();
            latencies_ns->push_back(
                std::chrono::duration_cast<std::chrono::nanoseconds>(
                    event_end - event_start
                ).count()
            );
        }
    }

    return stats;
}

struct SymbolLatency {
    std::size_t symbol_id;
    std::vector<long long> samples;
};

struct WorkerResult {
    BenchmarkStats stats;
    std::vector<SymbolLatency> latencies;
};

WorkerResult runWorker(
    int thread_id,
    int num_threads,
    const std::vector<std::vector<BenchmarkEvent>>& partitioned_events,
    std::size_t expected_orders_per_symbol,
    bool measure_latency
) {
    WorkerResult result;
    const std::size_t num_symbols = partitioned_events.size();

    for (std::size_t symbol_id = static_cast<std::size_t>(thread_id);
         symbol_id < num_symbols;
         symbol_id += static_cast<std::size_t>(num_threads)) {
        SymbolWorkerState worker{OrderBook(expected_orders_per_symbol), 0};

        std::vector<long long> local_latencies;
        std::vector<long long>* latencies_ptr = nullptr;
        if (measure_latency) {
            local_latencies.reserve(partitioned_events[symbol_id].size());
            latencies_ptr = &local_latencies;
        }

        BenchmarkStats symbol_stats = processSymbolEvents(
            worker,
            partitioned_events[symbol_id],
            latencies_ptr
        );
        result.stats.add(symbol_stats);

        if (measure_latency) {
            result.latencies.push_back(
                SymbolLatency{symbol_id, std::move(local_latencies)}
            );
        }
    }

    return result;
}

void mergeLatencies(
    const std::vector<WorkerResult>& worker_results,
    std::vector<long long>& latencies_ns
) {
    std::size_t total_samples = 0;
    for (const WorkerResult& worker_result : worker_results) {
        for (const SymbolLatency& symbol_latency : worker_result.latencies) {
            total_samples += symbol_latency.samples.size();
        }
    }

    latencies_ns.clear();
    latencies_ns.reserve(total_samples);

    for (const WorkerResult& worker_result : worker_results) {
        for (const SymbolLatency& symbol_latency : worker_result.latencies) {
            latencies_ns.insert(
                latencies_ns.end(),
                symbol_latency.samples.begin(),
                symbol_latency.samples.end()
            );
        }
    }
}

BenchmarkStats runParallelStdThread(
    const std::vector<std::vector<BenchmarkEvent>>& partitioned_events,
    int num_threads,
    std::size_t expected_orders_per_symbol,
    bool measure_latency,
    std::vector<long long>& latencies_ns
) {
    std::vector<std::thread> workers;
    workers.reserve(static_cast<std::size_t>(num_threads));

    std::vector<WorkerResult> results(static_cast<std::size_t>(num_threads));

    for (int thread_id = 0; thread_id < num_threads; ++thread_id) {
        workers.emplace_back(
            [thread_id, num_threads, &partitioned_events, expected_orders_per_symbol,
             measure_latency, &results]() {
                results[static_cast<std::size_t>(thread_id)] = runWorker(
                    thread_id,
                    num_threads,
                    partitioned_events,
                    expected_orders_per_symbol,
                    measure_latency
                );
            }
        );
    }

    for (std::thread& worker : workers) {
        worker.join();
    }

    BenchmarkStats total_stats;
    for (const WorkerResult& result : results) {
        total_stats.add(result.stats);
    }

    if (measure_latency) {
        mergeLatencies(results, latencies_ns);
    } else {
        latencies_ns.clear();
    }

    return total_stats;
}

BenchmarkStats runParallelOpenMP(
    const std::vector<std::vector<BenchmarkEvent>>& partitioned_events,
    int num_threads,
    std::size_t expected_orders_per_symbol,
    bool measure_latency,
    std::vector<long long>& latencies_ns
) {
#ifdef _OPENMP
    const std::size_t num_symbols = partitioned_events.size();

    omp_set_num_threads(num_threads);

    std::vector<BenchmarkStats> per_symbol_stats(num_symbols);
    std::vector<std::vector<long long>> per_symbol_latencies;
    if (measure_latency) {
        per_symbol_latencies.resize(num_symbols);
    }

#pragma omp parallel for schedule(static)
    for (std::size_t symbol_id = 0; symbol_id < num_symbols; ++symbol_id) {
        SymbolWorkerState worker{OrderBook(expected_orders_per_symbol), 0};

        std::vector<long long>* latencies_ptr = nullptr;
        if (measure_latency) {
            per_symbol_latencies[symbol_id].reserve(
                partitioned_events[symbol_id].size()
            );
            latencies_ptr = &per_symbol_latencies[symbol_id];
        }

        per_symbol_stats[symbol_id] = processSymbolEvents(
            worker,
            partitioned_events[symbol_id],
            latencies_ptr
        );
    }

    BenchmarkStats total_stats;
    for (const BenchmarkStats& symbol_stats : per_symbol_stats) {
        total_stats.add(symbol_stats);
    }

    if (measure_latency) {
        std::size_t total_samples = 0;
        for (const auto& symbol_latencies : per_symbol_latencies) {
            total_samples += symbol_latencies.size();
        }

        latencies_ns.clear();
        latencies_ns.reserve(total_samples);

        for (const auto& symbol_latencies : per_symbol_latencies) {
            latencies_ns.insert(
                latencies_ns.end(),
                symbol_latencies.begin(),
                symbol_latencies.end()
            );
        }
    } else {
        latencies_ns.clear();
    }

    return total_stats;
#else
    (void)partitioned_events;
    (void)num_threads;
    (void)expected_orders_per_symbol;
    (void)measure_latency;
    latencies_ns.clear();
    return {};
#endif
}

BenchmarkStats runParallel(
    const std::vector<std::vector<BenchmarkEvent>>& partitioned_events,
    int num_threads,
    std::size_t expected_orders_per_symbol,
    bool measure_latency,
    ParallelBackend backend,
    std::vector<long long>& latencies_ns
) {
    if (backend == ParallelBackend::OpenMP) {
        return runParallelOpenMP(
            partitioned_events,
            num_threads,
            expected_orders_per_symbol,
            measure_latency,
            latencies_ns
        );
    }

    return runParallelStdThread(
        partitioned_events,
        num_threads,
        expected_orders_per_symbol,
        measure_latency,
        latencies_ns
    );
}

BenchmarkStats runSequential(
    MatchingEngine& engine,
    const std::vector<BenchmarkEvent>& events,
    bool measure_latency,
    std::vector<long long>& latencies_ns
) {
    latencies_ns.clear();
    if (measure_latency) {
        latencies_ns.reserve(events.size());
    }
    return processSymbolEvents(
        engine,
        events,
        measure_latency ? &latencies_ns : nullptr
    );
}

void printLatencyStats(const std::vector<long long>& latencies_ns) {
    if (latencies_ns.empty()) {
        return;
    }

    std::vector<long long> sorted_latencies = latencies_ns;
    std::sort(sorted_latencies.begin(), sorted_latencies.end());

    auto percentile = [&](double p) {
        std::size_t index = static_cast<std::size_t>(
            p * static_cast<double>(sorted_latencies.size() - 1)
        );
        return sorted_latencies[index];
    };

    std::cout << "p50 latency: " << percentile(0.50) << " ns\n";
    std::cout << "p95 latency: " << percentile(0.95) << " ns\n";
    std::cout << "p99 latency: " << percentile(0.99) << " ns\n";
    std::cout << "p99.99 latency: " << percentile(0.9999) << " ns\n";
    std::cout << "Max latency: " << sorted_latencies.back() << " ns\n";
}

const char* backendName(ParallelBackend backend) {
    return backend == ParallelBackend::OpenMP ? "openmp" : "std-thread";
}

}  // namespace

int main(int argc, char* argv[]) {
    BenchmarkConfig config;
    if (!parseArgs(argc, argv, config)) {
        return 1;
    }

    const std::vector<std::string> symbols = makeSymbols(config.num_symbols);
    const int num_symbols = static_cast<int>(symbols.size());

    if (config.num_threads == 0) {
        unsigned int hw = std::thread::hardware_concurrency();
        int hardware_threads = hw == 0 ? 1 : static_cast<int>(hw);
        config.num_threads = std::min(hardware_threads, num_symbols);
    }

    MatchingEngine engine(static_cast<std::size_t>(num_symbols));
    std::vector<SymbolId> symbol_ids;
    symbol_ids.reserve(static_cast<std::size_t>(num_symbols));

    std::size_t expected_orders_per_symbol =
        static_cast<std::size_t>(config.num_events / num_symbols);

    for (const std::string& symbol : symbols) {
        symbol_ids.push_back(engine.addSymbol(symbol, expected_orders_per_symbol));
    }

    auto generation_start = std::chrono::high_resolution_clock::now();
    std::vector<BenchmarkEvent> events = generateEvents(
        symbols,
        symbol_ids,
        config.num_events
    );
    auto generation_end = std::chrono::high_resolution_clock::now();

    std::vector<std::vector<BenchmarkEvent>> partitioned_events;
    if (!config.sequential) {
        partitioned_events = partitionBySymbol(
            events,
            static_cast<std::size_t>(num_symbols)
        );
    }

    std::vector<long long> latencies_ns;
    BenchmarkStats stats;

    auto processing_start = std::chrono::high_resolution_clock::now();

    if (config.sequential) {
        stats = runSequential(engine, events, config.measure_latency, latencies_ns);
    } else {
        stats = runParallel(
            partitioned_events,
            config.num_threads,
            expected_orders_per_symbol,
            config.measure_latency,
            config.backend,
            latencies_ns
        );
    }

    auto processing_end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> generation_elapsed = generation_end - generation_start;
    std::chrono::duration<double> processing_elapsed = processing_end - processing_start;
    std::chrono::duration<double> total_elapsed = processing_end - generation_start;

    double processing_seconds = processing_elapsed.count();
    double throughput = static_cast<double>(config.num_events) / processing_seconds;
    double avg_latency_ns =
        (processing_seconds * 1'000'000'000.0) / static_cast<double>(config.num_events);

    std::cout << "Events processed: " << config.num_events << '\n';
    std::cout << "Symbols: " << num_symbols << '\n';
    std::cout << "Execution mode: "
              << (config.sequential ? "sequential" : "symbol-parallel") << '\n';

    if (!config.sequential) {
        std::cout << "Parallel backend: " << backendName(config.backend) << '\n';
        std::cout << "Worker threads: " << config.num_threads << '\n';
        std::cout << "Latency sampling: "
                  << (config.measure_latency ? "enabled" : "disabled") << '\n';
    }

    std::cout << "Symbol list: ";
    for (const std::string& symbol : symbols) {
        std::cout << symbol << ' ';
    }
    std::cout << '\n';

    std::cout << "Orders submitted: " << stats.orders_submitted << '\n';
    std::cout << "Cancel attempts: " << stats.cancels_submitted << '\n';
    std::cout << "Successful cancels: " << stats.successful_cancels << '\n';
    std::cout << "Trades generated: " << stats.trades_generated << '\n';

    std::cout << "Event generation time: " << generation_elapsed.count() << " seconds\n";
    std::cout << "Processing time: " << processing_seconds << " seconds\n";
    std::cout << "Total time: " << total_elapsed.count() << " seconds\n";
    std::cout << "Throughput: " << throughput << " events/sec\n";
    std::cout << "Average latency: " << avg_latency_ns << " ns/event\n";

    if (config.measure_latency) {
        printLatencyStats(latencies_ns);
    }

    return 0;
}
