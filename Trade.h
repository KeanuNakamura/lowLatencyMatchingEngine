#ifndef TRADE_H
#define TRADE_H
#include "Order.h"

struct Trade {
    OrderId restingOrderId;
    OrderId incomingOrderId;
    Price price;
    Quantity quantity;     

    Trade(OrderId restingId, OrderId incomingId, Price p, Quantity q) 
        :restingOrderId{restingId}, incomingOrderId{incomingId}, price{p}, quantity{q} {}

    bool operator==(const Trade& other) const {
    return restingOrderId == other.restingOrderId &&
           incomingOrderId == other.incomingOrderId &&
           price == other.price &&
           quantity == other.quantity;
    }
};
#endif
