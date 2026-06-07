#include "MatchingEngine.h" 

std::vector<Trade> MatchingEngine::submitOrder(Order order) {
    order.timestamp = current_time++;
    return book.addOrder(order); 
}

bool MatchingEngine::cancelOrder(OrderId order_id) {
    return book.cancelOrder(order_id); 
}
