#ifndef MATCHINGENGINE_H
#define MATCHINGENGINE_H
#include "Order.h" 
#include "Trade.h" 
#include "OrderBook.h" 

class MatchingEngine{
    public: 
        std::vector<Trade> submitOrder(const std::string& symbol, Order order); 
        bool cancelOrder(const std::string& symbol, OrderId order_id); 
        Quantity quantityAtBid(const std::string& symbol, Price price) const; 
        Quantity quantityAtAsk(const std::string& symbol, Price price) const; 

    private:
        std::unordered_map<std::string, OrderBook> books; 
        Timestamp current_time = 0; 
};
#endif
