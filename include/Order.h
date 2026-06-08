#ifndef ORDER_H
#define ORDER_H
#include <iostream> 

enum class Side {
    Buy,
    Sell
};

enum class OrderType {
    Market,
    Limit
};

using OrderId = std::uint64_t;
using Price = std::int64_t;
using Quantity = std::uint64_t;
using Timestamp = std::uint64_t;

struct Order {
    Side side; 
    OrderType type;
    OrderId id;
    Price price;
    Quantity quantity;
    Timestamp timestamp;
};

#endif
