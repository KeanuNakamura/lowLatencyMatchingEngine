#ifndef TRADE_H
#define TRADE_H
#include "Order.h"

struct Trade {
    OrderId restingOrderId;
    OrderId incomingOrderId;
    Price price;
    Quantity quantity;     

    bool operator==(const Trade& other) const {
    return buy_order_id == other.buy_order_id &&
           sell_order_id == other.sell_order_id &&
           price == other.price &&
           quantity == other.quantity;
}

};
#endif
