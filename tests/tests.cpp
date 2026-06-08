#include <gtest/gtest.h>

#include "MatchingEngine.h"
#include "Order.h"
#include "Trade.h"

namespace {

Order makeLimitOrder(
    Side side,
    OrderId id,
    Price price,
    Quantity quantity
) {
    return Order{
        side,
        OrderType::Limit,
        id,
        price,
        quantity,
        0
    };
}

Order makeMarketOrder(
    Side side,
    OrderId id,
    Quantity quantity
) {
    return Order{
        side,
        OrderType::Market,
        id,
        0,
        quantity,
        0
    };
}

}

TEST(MatchingEngineTest, LimitBuyOrderAddsToBidBook) {
    MatchingEngine engine;

    auto trades = engine.submitOrder(
        "AAPL",
        makeLimitOrder(Side::Buy, 1, 100, 10)
    );

    EXPECT_TRUE(trades.empty());
    EXPECT_EQ(engine.quantityAtBid("AAPL", 100), 10);
    EXPECT_EQ(engine.quantityAtAsk("AAPL", 100), 0);
}

TEST(MatchingEngineTest, LimitSellOrderAddsToAskBook) {
    MatchingEngine engine;

    auto trades = engine.submitOrder(
        "AAPL",
        makeLimitOrder(Side::Sell, 1, 105, 20)
    );

    EXPECT_TRUE(trades.empty());
    EXPECT_EQ(engine.quantityAtAsk("AAPL", 105), 20);
    EXPECT_EQ(engine.quantityAtBid("AAPL", 105), 0);
}

TEST(MatchingEngineTest, BuyLimitMatchesRestingSell) {
    MatchingEngine engine;

    engine.submitOrder(
        "AAPL",
        makeLimitOrder(Side::Sell, 1, 100, 10)
    );

    auto trades = engine.submitOrder(
        "AAPL",
        makeLimitOrder(Side::Buy, 2, 100, 10)
    );

    ASSERT_EQ(trades.size(), 1);

    EXPECT_EQ(trades[0], Trade(1, 2, 100, 10));

    EXPECT_EQ(engine.quantityAtAsk("AAPL", 100), 0);
    EXPECT_EQ(engine.quantityAtBid("AAPL", 100), 0);
}

TEST(MatchingEngineTest, SellLimitMatchesRestingBuy) {
    MatchingEngine engine;

    engine.submitOrder(
        "AAPL",
        makeLimitOrder(Side::Buy, 1, 100, 10)
    );

    auto trades = engine.submitOrder(
        "AAPL",
        makeLimitOrder(Side::Sell, 2, 100, 10)
    );

    ASSERT_EQ(trades.size(), 1);

    EXPECT_EQ(trades[0], Trade(1, 2, 100, 10));

    EXPECT_EQ(engine.quantityAtBid("AAPL", 100), 0);
    EXPECT_EQ(engine.quantityAtAsk("AAPL", 100), 0);
}

TEST(MatchingEngineTest, BuyLimitCrossesAskAtRestingAskPrice) {
    MatchingEngine engine;

    engine.submitOrder(
        "AAPL",
        makeLimitOrder(Side::Sell, 1, 95, 10)
    );

    auto trades = engine.submitOrder(
        "AAPL",
        makeLimitOrder(Side::Buy, 2, 100, 10)
    );

    ASSERT_EQ(trades.size(), 1);

    // Trade should happen at resting order's price, not incoming order's price.
    EXPECT_EQ(trades[0], Trade(1, 2, 95, 10));
}

TEST(MatchingEngineTest, SellLimitCrossesBidAtRestingBidPrice) {
    MatchingEngine engine;

    engine.submitOrder(
        "AAPL",
        makeLimitOrder(Side::Buy, 1, 105, 10)
    );

    auto trades = engine.submitOrder(
        "AAPL",
        makeLimitOrder(Side::Sell, 2, 100, 10)
    );

    ASSERT_EQ(trades.size(), 1);

    EXPECT_EQ(trades[0], Trade(1, 2, 105, 10));
}

TEST(MatchingEngineTest, IncomingBuyPartiallyFillsRestingSell) {
    MatchingEngine engine;

    engine.submitOrder(
        "AAPL",
        makeLimitOrder(Side::Sell, 1, 100, 20)
    );

    auto trades = engine.submitOrder(
        "AAPL",
        makeLimitOrder(Side::Buy, 2, 100, 5)
    );

    ASSERT_EQ(trades.size(), 1);

    EXPECT_EQ(trades[0], Trade(1, 2, 100, 5));

    // Resting sell should have 15 left.
    EXPECT_EQ(engine.quantityAtAsk("AAPL", 100), 15);
    EXPECT_EQ(engine.quantityAtBid("AAPL", 100), 0);
}

TEST(MatchingEngineTest, IncomingBuyPartiallyRestsAfterFillingSell) {
    MatchingEngine engine;

    engine.submitOrder(
        "AAPL",
        makeLimitOrder(Side::Sell, 1, 100, 5)
    );

    auto trades = engine.submitOrder(
        "AAPL",
        makeLimitOrder(Side::Buy, 2, 100, 20)
    );

    ASSERT_EQ(trades.size(), 1);

    EXPECT_EQ(trades[0], Trade(1, 2, 100, 5));

    // Incoming buy should have 15 remaining and rest on the bid side.
    EXPECT_EQ(engine.quantityAtAsk("AAPL", 100), 0);
    EXPECT_EQ(engine.quantityAtBid("AAPL", 100), 15);
}

TEST(MatchingEngineTest, BuyLimitSweepsMultipleAskLevels) {
    MatchingEngine engine;

    engine.submitOrder(
        "AAPL",
        makeLimitOrder(Side::Sell, 1, 99, 10)
    );

    engine.submitOrder(
        "AAPL",
        makeLimitOrder(Side::Sell, 2, 100, 20)
    );

    auto trades = engine.submitOrder(
        "AAPL",
        makeLimitOrder(Side::Buy, 3, 100, 25)
    );

    ASSERT_EQ(trades.size(), 2);

    EXPECT_EQ(trades[0], Trade(1, 3, 99, 10));
    EXPECT_EQ(trades[1], Trade(2, 3, 100, 15));

    EXPECT_EQ(engine.quantityAtAsk("AAPL", 99), 0);
    EXPECT_EQ(engine.quantityAtAsk("AAPL", 100), 5);
    EXPECT_EQ(engine.quantityAtBid("AAPL", 100), 0);
}

TEST(MatchingEngineTest, SellLimitSweepsMultipleBidLevels) {
    MatchingEngine engine;

    engine.submitOrder(
        "AAPL",
        makeLimitOrder(Side::Buy, 1, 101, 10)
    );

    engine.submitOrder(
        "AAPL",
        makeLimitOrder(Side::Buy, 2, 100, 20)
    );

    auto trades = engine.submitOrder(
        "AAPL",
        makeLimitOrder(Side::Sell, 3, 100, 25)
    );

    ASSERT_EQ(trades.size(), 2);

    EXPECT_EQ(trades[0], Trade(1, 3, 101, 10));
    EXPECT_EQ(trades[1], Trade(2, 3, 100, 15));

    EXPECT_EQ(engine.quantityAtBid("AAPL", 101), 0);
    EXPECT_EQ(engine.quantityAtBid("AAPL", 100), 5);
    EXPECT_EQ(engine.quantityAtAsk("AAPL", 100), 0);
}

TEST(MatchingEngineTest, OrdersAtSamePriceMatchFIFO) {
    MatchingEngine engine;

    engine.submitOrder(
        "AAPL",
        makeLimitOrder(Side::Sell, 1, 100, 10)
    );

    engine.submitOrder(
        "AAPL",
        makeLimitOrder(Side::Sell, 2, 100, 10)
    );

    auto trades = engine.submitOrder(
        "AAPL",
        makeLimitOrder(Side::Buy, 3, 100, 15)
    );

    ASSERT_EQ(trades.size(), 2);

    // Order 1 should be filled first because it arrived first.
    EXPECT_EQ(trades[0], Trade(1, 3, 100, 10));
    EXPECT_EQ(trades[1], Trade(2, 3, 100, 5));

    EXPECT_EQ(engine.quantityAtAsk("AAPL", 100), 5);
}

TEST(MatchingEngineTest, CancelExistingBuyOrder) {
    MatchingEngine engine;

    engine.submitOrder(
        "AAPL",
        makeLimitOrder(Side::Buy, 1, 100, 10)
    );

    EXPECT_EQ(engine.quantityAtBid("AAPL", 100), 10);

    bool cancelled = engine.cancelOrder("AAPL", 1);

    EXPECT_TRUE(cancelled);
    EXPECT_EQ(engine.quantityAtBid("AAPL", 100), 0);
}

TEST(MatchingEngineTest, CancelExistingSellOrder) {
    MatchingEngine engine;

    engine.submitOrder(
        "AAPL",
        makeLimitOrder(Side::Sell, 1, 105, 20)
    );

    EXPECT_EQ(engine.quantityAtAsk("AAPL", 105), 20);

    bool cancelled = engine.cancelOrder("AAPL", 1);

    EXPECT_TRUE(cancelled);
    EXPECT_EQ(engine.quantityAtAsk("AAPL", 105), 0);
}

TEST(MatchingEngineTest, CancelNonexistentOrderReturnsFalse) {
    MatchingEngine engine;

    bool cancelled = engine.cancelOrder("AAPL", 999);

    EXPECT_FALSE(cancelled);
}

TEST(MatchingEngineTest, DifferentSymbolsHaveIndependentBooks) {
    MatchingEngine engine;

    engine.submitOrder(
        "AAPL",
        makeLimitOrder(Side::Buy, 1, 100, 10)
    );

    engine.submitOrder(
        "MSFT",
        makeLimitOrder(Side::Buy, 2, 200, 20)
    );

    EXPECT_EQ(engine.quantityAtBid("AAPL", 100), 10);
    EXPECT_EQ(engine.quantityAtBid("MSFT", 200), 20);

    EXPECT_EQ(engine.quantityAtBid("AAPL", 200), 0);
    EXPECT_EQ(engine.quantityAtBid("MSFT", 100), 0);
}

TEST(MatchingEngineTest, MatchingOnlyOccursWithinSameSymbol) {
    MatchingEngine engine;

    engine.submitOrder(
        "AAPL",
        makeLimitOrder(Side::Sell, 1, 100, 10)
    );

    auto trades = engine.submitOrder(
        "MSFT",
        makeLimitOrder(Side::Buy, 2, 100, 10)
    );

    EXPECT_TRUE(trades.empty());

    EXPECT_EQ(engine.quantityAtAsk("AAPL", 100), 10);
    EXPECT_EQ(engine.quantityAtBid("MSFT", 100), 10);
}

TEST(MatchingEngineTest, MarketBuyMatchesBestAsk) {
    MatchingEngine engine;

    engine.submitOrder(
        "AAPL",
        makeLimitOrder(Side::Sell, 1, 100, 10)
    );

    engine.submitOrder(
        "AAPL",
        makeLimitOrder(Side::Sell, 2, 101, 10)
    );

    auto trades = engine.submitOrder(
        "AAPL",
        makeMarketOrder(Side::Buy, 3, 15)
    );

    ASSERT_EQ(trades.size(), 2);

    EXPECT_EQ(trades[0], Trade(1, 3, 100, 10));
    EXPECT_EQ(trades[1], Trade(2, 3, 101, 5));

    EXPECT_EQ(engine.quantityAtAsk("AAPL", 100), 0);
    EXPECT_EQ(engine.quantityAtAsk("AAPL", 101), 5);
}

TEST(MatchingEngineTest, MarketSellMatchesBestBid) {
    MatchingEngine engine;

    engine.submitOrder(
        "AAPL",
        makeLimitOrder(Side::Buy, 1, 101, 10)
    );

    engine.submitOrder(
        "AAPL",
        makeLimitOrder(Side::Buy, 2, 100, 10)
    );

    auto trades = engine.submitOrder(
        "AAPL",
        makeMarketOrder(Side::Sell, 3, 15)
    );

    ASSERT_EQ(trades.size(), 2);

    EXPECT_EQ(trades[0], Trade(1, 3, 101, 10));
    EXPECT_EQ(trades[1], Trade(2, 3, 100, 5));

    EXPECT_EQ(engine.quantityAtBid("AAPL", 101), 0);
    EXPECT_EQ(engine.quantityAtBid("AAPL", 100), 5);
}

TEST(MatchingEngineTest, UnfilledMarketOrderDoesNotRest) {
    MatchingEngine engine;

    auto trades = engine.submitOrder(
        "AAPL",
        makeMarketOrder(Side::Buy, 1, 100)
    );

    EXPECT_TRUE(trades.empty());

    // Market orders should not be added to the book.
    EXPECT_EQ(engine.quantityAtBid("AAPL", 0), 0);
    EXPECT_EQ(engine.quantityAtAsk("AAPL", 0), 0);
}
