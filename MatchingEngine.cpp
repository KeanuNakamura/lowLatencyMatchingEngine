#include "MatchingEngine.h" 

std::vector<Trade> MatchingEngine::submitOrder(Order order) {
    order.timestamp = current_time++;
    return book.addOrder(order); 
}

bool MatchingEngine::cancelOrder(OrderId order_id) {
    return book.cancelOrder(order_id); 
}

Quantity MatchingEngine::quantityAtBid(Price price) const {
    return book.quantityAtBid(price); 
}

Quantity MatchingEngine::quantityAtAsk(Price price) const {
    return book.quantityAtAsk(price); 
}
