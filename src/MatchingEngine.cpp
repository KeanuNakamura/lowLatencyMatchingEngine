#include "MatchingEngine.h"

#include <stdexcept>

MatchingEngine::MatchingEngine(std::size_t expected_symbols) {
    symbols_.reserve(expected_symbols);
    symbol_ids_.reserve(expected_symbols);
}

SymbolId MatchingEngine::addSymbol(
    const std::string& symbol,
    std::size_t expected_orders
) {
    auto it = symbol_ids_.find(symbol);
    if (it != symbol_ids_.end()) {
        return it->second;
    }

    SymbolId id = symbols_.size();
    symbols_.push_back(SymbolBook{symbol, OrderBook(expected_orders), 0});
    symbol_ids_.emplace(symbol, id);
    return id;
}

SymbolId MatchingEngine::getSymbolId(const std::string& symbol) const {
    auto it = symbol_ids_.find(symbol);
    if (it == symbol_ids_.end()) {
        throw std::out_of_range("Unknown symbol: " + symbol);
    }
    return it->second;
}

SymbolId MatchingEngine::getOrCreateSymbolId(const std::string& symbol) {
    auto it = symbol_ids_.find(symbol);
    if (it != symbol_ids_.end()) {
        return it->second;
    }
    return addSymbol(symbol, 0);
}

std::vector<Trade> MatchingEngine::submitOrder(SymbolId symbol_id, Order order) {
    SymbolBook& entry = symbols_.at(symbol_id);
    order.timestamp = entry.current_time++;
    return entry.book.addOrder(order);
}

bool MatchingEngine::cancelOrder(SymbolId symbol_id, OrderId order_id) {
    return symbols_.at(symbol_id).book.cancelOrder(order_id);
}

std::vector<Trade> MatchingEngine::submitOrder(
    const std::string& symbol,
    Order order
) {
    return submitOrder(getOrCreateSymbolId(symbol), order);
}

bool MatchingEngine::cancelOrder(const std::string& symbol, OrderId order_id) {
    return cancelOrder(getOrCreateSymbolId(symbol), order_id);
}

Quantity MatchingEngine::quantityAtBid(SymbolId symbol_id, Price price) const {
    return symbols_.at(symbol_id).book.quantityAtBid(price);
}

Quantity MatchingEngine::quantityAtAsk(SymbolId symbol_id, Price price) const {
    return symbols_.at(symbol_id).book.quantityAtAsk(price);
}

Quantity MatchingEngine::quantityAtBid(const std::string& symbol, Price price) const {
    auto it = symbol_ids_.find(symbol);
    if (it == symbol_ids_.end()) {
        return 0;
    }
    return quantityAtBid(it->second, price);
}

Quantity MatchingEngine::quantityAtAsk(const std::string& symbol, Price price) const {
    auto it = symbol_ids_.find(symbol);
    if (it == symbol_ids_.end()) {
        return 0;
    }
    return quantityAtAsk(it->second, price);
}
