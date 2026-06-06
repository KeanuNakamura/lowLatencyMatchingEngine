#include <cassert>
#include <iostream>

#include "OrderBook.h"

void test_empty_book_has_zero_quantity(){
    OrderBook book; 
    Quantity quantity = book.quantityAtBid(100); 
    assert(quantity == 0); 
}

void testAddOrder(){
    OrderBook book; 
    Order order1{Side::Buy, OrderType::Limit, 1, 50, 10, 1};
    Order order2{Side::Buy, OrderType::Limit, 2, 100, 10, 2};
    book.addOrder(order1); 
    book.addOrder(order2); 
    Order order3{Side::Sell, OrderType::Limit, 3, 80, 10, 3};

    std::vector<Trade> trades = book.addOrder(order3); 
    Trade trade{2, 3, 100, 10}; 
    assert(trades == std::vector<Trade>{trade}); 
}

int main() {
    test_empty_book_has_zero_quantity(); 
    testAddOrder(); 
    return 0; 

}
