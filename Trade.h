#ifndef TRADE_H
#define TRADE_H
#include "Order.h"

struct Trade {
    OrderId restingOrderId;
    OrderId incomingOrderId;
    Price price;
    Quantity quantity;     

    bool operator==(const Trade& other) const {
    return restingOrderId == other.restingOrderId &&
           incomingOrderId == other.incomingOrderId &&
           price == other.price &&
           quantity == other.quantity;
    }
};
#endif
