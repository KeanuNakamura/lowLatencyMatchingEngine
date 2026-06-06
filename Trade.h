ifndef TRADE_H
#define TRADE_H
#include "Order.h"

struct Trade {
    OrderId restingOrderId;
    OrderId incomingOrderId;
    Price price;
    Quantity quantity;     
}
#endif
