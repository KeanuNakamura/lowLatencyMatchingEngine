#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "MatchingEngine.h"

Side parseSide(const std::string& side) {
    if (side == "BUY") {
        return Side::Buy;
    } else if (side == "SELL") {
        return Side::Sell;
    }

    throw std::invalid_argument("Invalid side: " + side);
}

OrderType parseOrderType(const std::string& type) {
    if (type == "LIMIT") {
        return OrderType::Limit;
    }

    throw std::invalid_argument("Invalid order type: " + type);
}

void printTrade(const std::string& symbol, const Trade& trade) {
    std::cout << "TRADE "
              << "symbol=" << symbol << " "
              << "resting=" << trade.restingOrderId << " "
              << "incoming=" << trade.incomingOrderId << " "
              << "price=" << trade.price << " "
              << "quantity=" << trade.quantity << '\n';
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: ./matching_engine input.txt\n";
        return 1;
    }

    std::ifstream input(argv[1]);

    if (!input) {
        std::cerr << "Could not open file: " << argv[1] << '\n';
        return 1;
    }

    MatchingEngine engine;

    std::string line;

    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }

        std::stringstream ss(line);

        std::string symbol;
        ss >> symbol;

        std::string second_token;
        ss >> second_token;

        if (!ss) {
            std::cerr << "Invalid line: " << line << '\n';
            continue;
        }

        if (second_token == "CANCEL") {
            OrderId order_id;
            ss >> order_id;

            if (!ss) {
                std::cerr << "Invalid cancel line: " << line << '\n';
                continue;
            }

            bool cancelled = engine.cancelOrder(symbol, order_id);

            if (cancelled) {
                std::cout << "CANCELLED "
                          << "symbol=" << symbol << " "
                          << "id=" << order_id << '\n';
            } else {
                std::cout << "CANCEL_REJECTED "
                          << "symbol=" << symbol << " "
                          << "id=" << order_id << '\n';
            }

        } else {
            OrderId order_id = std::stoull(second_token);

            std::string side_str;
            std::string type_str;
            Price price;
            Quantity quantity;

            ss >> side_str >> type_str >> price >> quantity;

            if (!ss) {
                std::cerr << "Invalid order line: " << line << '\n';
                continue;
            }

            Order order{
                parseSide(side_str),
                parseOrderType(type_str),
                order_id,
                price,
                quantity,
                0
            };

            std::vector<Trade> trades = engine.submitOrder(symbol, order);

            for (const Trade& trade : trades) {
                printTrade(symbol, trade);
            }
        }
    }

    return 0;
}
