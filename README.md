# lowLatencyMatchingEngine
C++ low latency matching engine


Preallocated order lookup hashmap capacity to avoid runtime rehashing, improving throughput from 4.97M to 6.09M events/sec and reducing max observed latency from 68.5 ms to 40.5 μs over 10M simulated events.
