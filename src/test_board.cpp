#include "game.h"
#include <doctest.h>

namespace Reversi {
    TEST_CASE("Test default initialization") {
        Board b;
        CHECK(b(4, 4) == Square::Black);
        CHECK(b(5, 5) == Square::Black);
        CHECK(b(4, 5) == Square::White);
        CHECK(b(5, 4) == Square::White);
        CHECK(b(1, 1) == Square::Empty);
        CHECK(b(8, 8) == Square::Empty);
        CHECK(b(0, 0) == Square::OutOfRange);
        CHECK(b(9, 9) == Square::OutOfRange);
        CHECK(b(9, 0) == Square::OutOfRange);
        CHECK(b(0, 9) == Square::OutOfRange);
        CHECK(b(5, 0) == Square::OutOfRange);
        CHECK(b(0, 7) == Square::OutOfRange);
    }

    TEST_CASE("At bounds checking") {
        Board b;
        CHECK(b.at(0, 0) == Square::OutOfRange);
        CHECK(b.at(8, 9) == Square::OutOfRange);
        CHECK(b.at(9, 9) == Square::OutOfRange);
        CHECK(b.at(1, 3) != Square::OutOfRange);
        CHECK(b.at(5, 8) != Square::OutOfRange);
        CHECK(b.at(-1, 5) == Square::OutOfRange);
        CHECK(b.at(1000, 12345) == Square::OutOfRange);
    }

    TEST_CASE("placable") {
        Board b;
        REQUIRE(b.at(4, 4) == Square::Black);
        CHECK(!b.is_skip_legal());
        CHECK(!b.is_placable(4, 4));
        CHECK(!b.is_placable(4, 5));
        CHECK(!b.is_placable(-1, -1));
        CHECK(!b.is_placable(9, 9));
        CHECK(b.is_placable(4, 6));
        CHECK(b.get_placable() == std::vector<std::pair<int, int>>{ {3, 5}, {4, 6}, {5, 3}, {6, 4} });
    }

    TEST_CASE("simple game") {
        Board b;
        // Check == along the way
        CHECK(b == Board());
        // The game starts with black and an even position
        CHECK(b.whos_next() == Player::Black);
        CHECK(b.count() == MatchResult::Draw);
        REQUIRE(b(4, 4) == Square::Black);
        REQUIRE(b.is_placable(4, 6));
        b.place(4, 6);
        CHECK(!(b == Board()));
        CHECK(b.whos_next() == Player::White);
        CHECK(b(4, 6) == Square::Black);
        CHECK(b(4, 5) == Square::Black);
        CHECK(b.is_placable(3, 4));
        CHECK(b.is_placable(5, 6));
        REQUIRE(b.is_placable(3, 6));
        b.place(3, 6);
        CHECK(b.whos_next() == Player::Black);
        REQUIRE(b.is_placable(3, 5));
        b.place(3, 5);
        CHECK(b.whos_next() == Player::White);
        REQUIRE(b.is_placable(5, 6));
        b.place(5, 6);
        CHECK(b.whos_next() == Player::Black);
        // Check the white pieces
        std::vector<std::pair<int, int>> pos{ {3, 6}, {4, 6}, {5, 6}, {5, 5}, {5, 4} };
        for (auto [x, y] : pos) {
            CHECK(b(x, y) == Square::White);
        }
        pos.assign({ {3, 5}, {4, 5}, {4, 4} });
        for (auto [x, y] : pos) {
            CHECK(b(x, y) == Square::Black);
        }
        CHECK(b(3, 4) == Square::Empty);
        // Now white has much more pieces
        CHECK(b.count() == MatchResult::White);
    }

    TEST_CASE("place exception") {
        Board b;
        CHECK_THROWS_AS(b.place(0, -1), std::out_of_range);
    }
}