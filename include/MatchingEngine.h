#ifndef MATCHINGENGINE_H
#define MATCHINGENGINE_H
#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

#include "Order.h"
#include "Trade.h"
#include "OrderBook.h"

using SymbolId = std::size_t;

class MatchingEngine {
    public:
        MatchingEngine() = default;
        explicit MatchingEngine(std::size_t expected_symbols);

        SymbolId addSymbol(const std::string& symbol, std::size_t expected_orders);
        SymbolId getSymbolId(const std::string& symbol) const;

        std::vector<Trade> submitOrder(SymbolId symbol_id, Order order);
        bool cancelOrder(SymbolId symbol_id, OrderId order_id);

        std::vector<Trade> submitOrder(const std::string& symbol, Order order);
        bool cancelOrder(const std::string& symbol, OrderId order_id);

        Quantity quantityAtBid(const std::string& symbol, Price price) const;
        Quantity quantityAtAsk(const std::string& symbol, Price price) const;

        Quantity quantityAtBid(SymbolId symbol_id, Price price) const;
        Quantity quantityAtAsk(SymbolId symbol_id, Price price) const;

        std::size_t symbolCount() const { return symbols_.size(); }

    private:
        SymbolId getOrCreateSymbolId(const std::string& symbol);

        struct SymbolBook {
            std::string name;
            OrderBook book;
            Timestamp current_time = 0;
        };

        std::vector<SymbolBook> symbols_;
        std::unordered_map<std::string, SymbolId> symbol_ids_;
};

#endif
