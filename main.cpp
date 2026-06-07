#include <iostream>
#include <fstream>
#include <sstream>
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

void printTrade(const Trade& trade) {
    std::cout << "TRADE "
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

        std::string command;
        ss >> command;

        if (command == "ADD") {
            OrderId order_id;
            std::string side_str;
            std::string type_str;
            Price price;
            Quantity quantity;

            ss >> order_id >> side_str >> type_str >> price >> quantity;

            Order order{
                parseSide(side_str),
                parseOrderType(type_str),
                order_id,
                price,
                quantity,
                0
            };

            std::vector<Trade> trades = engine.submitOrder(order);

            for (const Trade& trade : trades) {
                printTrade(trade);
            }

        } else if (command == "CANCEL") {
            OrderId order_id;
            ss >> order_id;

            bool cancelled = engine.cancelOrder(order_id);

            if (cancelled) {
                std::cout << "CANCELLED " << order_id << '\n';
            } else {
                std::cout << "CANCEL_REJECTED " << order_id << '\n';
            }

        } else {
            std::cerr << "Unknown command: " << command << '\n';
        }
    }

    return 0;
}
