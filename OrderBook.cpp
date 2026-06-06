#include "Order.h"
#include "Trade.h"
#include "OrderBook.h" 
#include <vector> 
#include <algorithm>

OrderBook::OrderBook()
std::vector<Trade> OrderBook::addOrder(Order order){
    std::vector<Trade> trades; 
    while (order.quantity > 0 && !asks.empty()) {
        auto it = asks.begin(); 
        Price bestAsk = it->first;
        OrderList& orderlist = it->second; 
        Order& firstOrder = orderlist.front(); 
        
        if (order.type == Limit && order.price < bestAsk) {
           break;  
        }

        int tradeQuantity = std::min(order.quantity, firstOrder.quantity); 
        Trade trade{firstOrder.id, order.id, bestAsk, tradeQuantity}; 
        trades.push_back(trade); 
        
        order.quantity -= tradeQuantity; 
        firstOrder.quantity -= tradeQuantity; 
        
        if (firstOrder.quantity <= 0) {
            int askId = firstOrder.id; 
            orderlist.pop_front();
            order_locations.erase(askId);  
        }
        if (orderlist.empty()) {
            asks.erase(bestAsk); 
        }
    }
    if (order.quantity > 0) {
        auto& ordersAtPrice = bids[order.price]; 
        ordersAtPrice.push_back(order); 
        auto it = std::prev(ordersAtPrice.end());
        order_locations[order.id] = OrderLocation{order.side, order.price, it};  
    }
    return trades; 
}


std::vector<Trade> OrderBook::sellOrder(Order order){
    std::vector<Trade> trades; 
    while (order.quantity > 0 && !bids.empty()) {
        auto it = bids.begin(); 
        Price bestBid = it->first;
        OrderList& orderlist = it->second; 
        Order& firstOrder = orderlist.front(); 
        
        if (order.type == Limit && order.price > bestBid) {
           break;  
        }

        int tradeQuantity = std::min(order.quantity, firstOrder.quantity); 
        Trade trade{firstOrder.id, order.id, bestBid, tradeQuantity}; 
        trades.push_back(trade); 
        
        order.quantity -= tradeQuantity; 
        firstOrder.quantity -= tradeQuantity; 
        
        if (firstOrder.quantity <= 0) {
            int BidId = firstOrder.id; 
            orderlist.pop_front();
            order_locations.erase(BidId);  
        }
        if (orderlist.empty()) {
            bids.erase(bestBid); 
        }
    }
    if (order.quantity > 0) {
        auto& ordersAtPrice = asks[order.price]; 
        ordersAtPrice.push_back(order); 
        auto it = std::prev(ordersAtPrice.end());
        order_locations[order.id] = OrderLocation{order.side, order.price, it};  
    }
    return trades; 
}

bool OrderBook::cancelOrder(OrderId order_id) {
    auto it order_locations.find(order_id); 
    if (it == order_locations.end()) {
        return false;
    }  
    OrderLocation& location = it->second;

    if (location.side == Side::Buy) {
        auto price_it = bids.find(location.price); 
        price_it->second.erase(location.iterator); 

        if (price_it->second.empty()) {
            bids.erase(price_it); 
        }
    } else {
        auto price_it = asks.find(location.price); 
        price_it->second.erase(location.iterator); 
        
        if (price_it->second.empty()) {
            asks.erase(price_it); 
        }
    }
    order_locations.erase(it); 
    return true; 
}

