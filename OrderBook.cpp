#include "Order.h"
#include "Trade.h"
#include "OrderBook.h" 
#include <vector> 
#include <algorithm>
#include <iostream>

OrderBook::OrderBook() {
    order_locations.reserve(10'000'000); 
}

std::vector<Trade> OrderBook::addOrder(Order order) {
    if (order.side == Side::Buy && order.type == OrderType::Limit) {
        return buyOrderLimit(order);
    } else if (order.side == Side::Buy && order.type == OrderType::Market) {
        return buyOrderMarket(order); 
    } else if (order.side == Side::Sell && order.type == OrderType::Limit) {
        return sellOrderLimit(order); 
    } else if (order.side == Side::Sell && order.type == OrderType::Market) {
        return sellOrderMarket(order); 
    } 
}


std::vector<Trade> OrderBook::buyOrderLimit(Order order){
    std::vector<Trade> trades; 
    while (order.quantity > 0 && !asks.empty()) {
        auto it = asks.begin(); 
        Price bestAsk = it->first;
        OrderList& orderlist = it->second; 
        Order& firstOrder = orderlist.front(); 
        
        if (order.price < bestAsk) {
           break;  
        }

        Quantity tradeQuantity = std::min(order.quantity, firstOrder.quantity); 
        Trade trade{firstOrder.id, order.id, bestAsk, tradeQuantity}; 
        trades.push_back(trade); 
        //trades.emplace_back(firstOrder.id, order.id, bestAsk, tradeQuantity); 
        
        order.quantity -= tradeQuantity; 
        firstOrder.quantity -= tradeQuantity; 
        
        if (firstOrder.quantity <= 0) {
            int askId = firstOrder.id; 
            orderlist.pop_front();
            order_locations.erase(askId);  
        }
        if (orderlist.empty()) {
            asks.erase(it); 
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

std::vector<Trade> OrderBook::buyOrderMarket(Order order){
    std::vector<Trade> trades; 
    while (order.quantity > 0 && !asks.empty()) {
        auto it = asks.begin(); 
        Price price = it->first; 
        OrderList& orderlist = it->second;
        Order& firstOrder = orderlist.front(); 
        Quantity tradeQuantity = std::min(order.quantity, firstOrder.quantity); 

        Trade trade{firstOrder.id, order.id, price, tradeQuantity}; 
        trades.push_back(trade); 

        order.quantity -= tradeQuantity; 
        firstOrder.quantity -= tradeQuantity; 
        if (firstOrder.quantity == 0) {
            order_locations.erase(firstOrder.id); 
            orderlist.pop_front(); 
        }
        if (orderlist.empty()) {
            asks.erase(it); 
        }
    } 
    return trades;  
}

std::vector<Trade> OrderBook::sellOrderLimit(Order order){
    std::vector<Trade> trades; 
    while (order.quantity > 0 && !bids.empty()) {
        auto it = bids.begin(); 
        Price bestBid = it->first;
        OrderList& orderlist = it->second; 
        Order& firstOrder = orderlist.front(); 
        
        if (order.price > bestBid) {
           break;  
        }

        Quantity tradeQuantity = std::min(order.quantity, firstOrder.quantity); 
        Trade trade{firstOrder.id, order.id, bestBid, tradeQuantity}; 
        trades.push_back(trade); 
        //trades.emplace_back(firstOrder.id, order.id, bestBid, tradeQuantity);  
        order.quantity -= tradeQuantity; 
        firstOrder.quantity -= tradeQuantity; 
        
        if (firstOrder.quantity <= 0) {
            int BidId = firstOrder.id; 
            orderlist.pop_front();
            order_locations.erase(BidId);  
        }
        if (orderlist.empty()) {
            bids.erase(it); 
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


std::vector<Trade> OrderBook::sellOrderMarket(Order order){
    std::vector<Trade> trades; 
    while (order.quantity > 0 && !bids.empty()) {
        auto it = bids.begin(); 
        Price price = it->first; 
        OrderList& orderlist = it->second;
        Order& firstOrder = orderlist.front(); 
        Quantity tradeQuantity = std::min(order.quantity, firstOrder.quantity); 

        Trade trade{firstOrder.id, order.id, price, tradeQuantity}; 
        trades.push_back(trade); 

        order.quantity -= tradeQuantity; 
        firstOrder.quantity -= tradeQuantity; 
        if (firstOrder.quantity == 0) {
            order_locations.erase(firstOrder.id); 
            orderlist.pop_front(); 
        }
        if (orderlist.empty()) {
            bids.erase(it); 
        }
    } 
    return trades;  
}


bool OrderBook::cancelOrder(OrderId order_id) {
    auto it = order_locations.find(order_id); 
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

std::optional<Price> OrderBook::bestBid() const {
    if (bids.empty()) {
        return std::nullopt; 
    }
    auto it = bids.begin(); 
    return it->first; 
}

std::optional<Price> OrderBook::bestAsk() const {

    if (asks.empty()) {
        return std::nullopt; 
    }
    auto it = asks.begin(); 
    return it->first; 
}

Quantity OrderBook::quantityAtBid(Price price) const {
    auto it = bids.find(price); 
    if (it == bids.end()) {
        return 0; 
    }
    Quantity total{}; 
    const auto& orderList =  it->second; 
    for (auto iter = orderList.begin(); iter != orderList.end(); ++iter) {
        total += iter->quantity; 
    }
    return total; 
}


Quantity OrderBook::quantityAtAsk(Price price) const {
    auto it = asks.find(price); 
    if (it == asks.end()) {
        return 0; 
    }
    Quantity total{}; 
    const auto& orderList =  it->second; 
    for (auto iter = orderList.begin(); iter != orderList.end(); ++iter) {
        total += iter->quantity; 
    }
    return total; 
}
