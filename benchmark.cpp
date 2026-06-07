#include <algorithm>
#include <chrono>
#include <iostream>
#include <random>
#include <vector>

#include "MatchingEngine.h"

int main() {
    constexpr int NUM_EVENTS = 10'000'000;

    MatchingEngine engine;

    std::mt19937 rng(42);

    std::uniform_int_distribution<int> side_dist(0, 1);
    std::uniform_int_distribution<int> price_dist(9900, 10100);
    std::uniform_int_distribution<int> quantity_dist(1, 100);
    std::uniform_int_distribution<int> event_dist(1, 100);

    std::vector<OrderId> active_order_ids;
    active_order_ids.reserve(NUM_EVENTS);

    std::vector<long long> latencies_ns;
    latencies_ns.reserve(NUM_EVENTS);

    OrderId next_order_id = 1;

    std::size_t orders_submitted = 0;
    std::size_t cancels_submitted = 0;
    std::size_t successful_cancels = 0;
    std::size_t trades_generated = 0;

    auto total_start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < NUM_EVENTS; ++i) {
        bool do_cancel = !active_order_ids.empty() && event_dist(rng) <= 10;

        auto event_start = std::chrono::high_resolution_clock::now();

        if (do_cancel) {
            std::uniform_int_distribution<std::size_t> index_dist(
                0, active_order_ids.size() - 1
            );

            std::size_t index = index_dist(rng);
            OrderId order_id = active_order_ids[index];

            bool cancelled = engine.cancelOrder(order_id);

            ++cancels_submitted;

            if (cancelled) {
                ++successful_cancels;

                active_order_ids[index] = active_order_ids.back();
                active_order_ids.pop_back();
            }

        } else {
            Side side = side_dist(rng) == 0 ? Side::Buy : Side::Sell;
            Price price = price_dist(rng);
            Quantity quantity = quantity_dist(rng);

            Order order{
                side,
                OrderType::Limit,
                next_order_id,
                price,
                quantity,
                0
            };

            std::vector<Trade> trades = engine.submitOrder(order);

            trades_generated += trades.size();
            ++orders_submitted;

            active_order_ids.push_back(next_order_id);
            ++next_order_id;
        }

        auto event_end = std::chrono::high_resolution_clock::now();

        long long latency_ns =
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                event_end - event_start
            ).count();

        latencies_ns.push_back(latency_ns);
    }

    auto total_end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> elapsed = total_end - total_start;

    std::sort(latencies_ns.begin(), latencies_ns.end());

    auto percentile = [&](double p) {
        std::size_t index = static_cast<std::size_t>(
            p * static_cast<double>(latencies_ns.size() - 1)
        );
        return latencies_ns[index];
    };

    double seconds = elapsed.count();
    double throughput = NUM_EVENTS / seconds;
    double avg_latency_ns = (seconds * 1'000'000'000.0) / NUM_EVENTS;

    long long p50_latency = percentile(0.50);
    long long p95_latency = percentile(0.95);
    long long p99_latency = percentile(0.99);
    long long max_latency = latencies_ns.back();

    std::cout << "Events processed: " << NUM_EVENTS << '\n';
    std::cout << "Orders submitted: " << orders_submitted << '\n';
    std::cout << "Cancel attempts: " << cancels_submitted << '\n';
    std::cout << "Successful cancels: " << successful_cancels << '\n';
    std::cout << "Trades generated: " << trades_generated << '\n';

    std::cout << "Total time: " << seconds << " seconds\n";
    std::cout << "Throughput: " << throughput << " events/sec\n";
    std::cout << "Average latency: " << avg_latency_ns << " ns/event\n";

    std::cout << "p50 latency: " << p50_latency << " ns\n";
    std::cout << "p95 latency: " << p95_latency << " ns\n";
    std::cout << "p99 latency: " << p99_latency << " ns\n";
    std::cout << "Max latency: " << max_latency << " ns\n";

    return 0;
}
