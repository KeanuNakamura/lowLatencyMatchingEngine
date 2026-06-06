#include <cassert>
#include <iostream>

#include "OrderBook.h"

void test_empty_book_has_zero_quantity(){
    OrderBook book; 
    Quantity quantity = book.quantityAtBid(100); 
    assert(quantity == 0); 
}

void testAddSellOrder(){
    OrderBook book; 
    Order order1{Side::Buy, OrderType::Limit, 1, 50, 10, 1};
    Order order2{Side::Buy, OrderType::Limit, 2, 100, 10, 2};
    book.addOrder(order1); 
    book.addOrder(order2); 
    assert(book.quantityAtBid(50) == 10); 
    assert(book.quantityAtBid(100) == 10); 

    Order order3{Side::Sell, OrderType::Limit, 3, 80, 10, 3}; 
    book.addOrder(order3); 
    assert(book.quantityAtBid(100) == 0); 
    
    Order order4{Side::Sell, OrderType::Limit, 4, 30, 20, 4}; 
    book.addOrder(order4); 
    assert(book.quantityAtBid(50) == 0); 
    assert(book.quantityAtAsk(30) == 10); 
}

void testAddSellOrder2(){
    OrderBook book; 
    Order order1{Side::Buy, OrderType::Limit, 1, 50, 20, 1};
    Order order2{Side::Buy, OrderType::Limit, 2, 50, 40, 2};
    Order order3{Side::Buy, OrderType::Limit, 3, 70, 10, 3};
    Order order4{Side::Buy, OrderType::Limit, 4, 100, 10, 4};
    book.addOrder(order1); 
    book.addOrder(order2); 
    book.addOrder(order3); 
    book.addOrder(order4); 
    assert(book.quantityAtBid(50) == 60);
    
    Order order5{Side::Sell, OrderType::Limit, 5, 40, 50, 5};
    std::vector<Trade> trades = book.addOrder(order5); 
    Trade trade1{4, 5, 100, 10};
    Trade trade2{3, 5, 70, 10};
    Trade trade3{1, 5, 50, 20}; 
    Trade trade4{2, 5, 50, 10}; 
    std::vector<Trade> expected_trades{trade1, trade2, trade3, trade4}; 
    assert(trades == expected_trades);  
    assert(book.quantityAtBid(50)==30);
}

void testAddSellOrder3(){
    OrderBook book;

    Order order1{Side::Sell, OrderType::Limit, 1, 100, 10, 1};
    Order order2{Side::Sell, OrderType::Limit, 2, 50, 10, 2};

    book.addOrder(order1);
    book.addOrder(order2);

    assert(book.quantityAtAsk(100) == 10);
    assert(book.quantityAtAsk(50) == 10);

    Order order3{Side::Buy, OrderType::Limit, 3, 80, 10, 3};
    book.addOrder(order3);

    assert(book.quantityAtAsk(50) == 0);

    Order order4{Side::Buy, OrderType::Limit, 4, 120, 20, 4};
    book.addOrder(order4);

    assert(book.quantityAtAsk(100) == 0);
    assert(book.quantityAtBid(120) == 10);
}

void testCancellation() {
    OrderBook book; 
    Order order1{Side::Buy, OrderType::Limit, 1, 50, 20, 1};
    Order order2{Side::Buy, OrderType::Limit, 2, 50, 40, 2};
    book.addOrder(order1); 
    book.addOrder(order2); 
    assert(book.cancelOrder(1) == true); 
    assert(book.quantityAtBid(50) == 40); 
    assert(book.cancelOrder(1) == false); 
    assert(book.cancelOrder(300) == false); 
}

int main() {
    test_empty_book_has_zero_quantity(); 
    testAddSellOrder(); 
    testAddSellOrder2(); 
    testCancellation(); 
    return 0; 

}
