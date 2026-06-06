#ifndef ORDERBOOK_H
#define ORDERBOOK_H

#include <map> 
#include <unordered_map> 
#include <list> 
#include <vector> 

class OrderBook {
    public: 
        std::vector addOrder(Order order);
        bool cancelOrder(OrderId id); 
    private:
        using OrderList std::list<Order>; 
        std::map<Price, OrderList, std::greater<Price>> bids; //greatest to lowest
        std::map<Price, OrderList> asks;  //sorted lowest to greatest
        
        struct OrderLocation {
            Side side; 
            Price price;
            OrderList::iterator iterator;  
        }
        std::unordered_map<OrderId, OrderLocation> order_locations; 
}
#endif 
