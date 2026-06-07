#include "MatchingEngine.h" 

std::vector<Trade> MatchingEngine::submitOrder(const std::string& symbol, Order order) {
    order.timestamp = current_time++;
    return books[symbol].addOrder(order); 
}

bool MatchingEngine::cancelOrder(const std::string& symbol, OrderId order_id) {
    return books[symbol].cancelOrder(order_id); 
}

Quantity MatchingEngine::quantityAtBid(const std::string& symbol, Price price) const {
    auto it = books.find(symbol); 
    if (it==books.end()) {
        return 0; 
    }
    return it->second.quantityAtBid(price); 
}

Quantity MatchingEngine::quantityAtAsk(const std::string& symbol, Price price) const {
    auto it = books.find(symbol);
    if (it == books.end()) {
        return 0;
    }
    return it->second.quantityAtAsk(price); 
}
