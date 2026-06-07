#ifndef MATCHINGENGINE_H
#define MATCHINGENGINE_H
#include "Order.h" 
#include "Trade.h" 
#include "OrderBook.h" 

class MatchingEngine{
    public: 
        std::vector<Trade> submitOrder(Order order); 
        bool cancelOrder(OrderId order_id); 
        Quantity quantityAtBid(Price price) const; 
        Quantity quantityAtAsk(Price price) const; 

    private:
        OrderBook book;
        Timestamp current_time = 0; 
};
#endif
