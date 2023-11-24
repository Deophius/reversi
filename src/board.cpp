#include "game.h"
#include <doctest.h>
#include <stdexcept>

namespace Reversi {
    void Board::set(int x, int y, Square sq) noexcept {
        const int subscript = ((MAX_FILES + 2) * x + y) >> 2;
        const int partit = ((MAX_FILES + 2) * x + y) & 3;
        switch (partit) {
        case 0:
            squares[subscript] &= 0xFC;
            squares[subscript] |= static_cast<unsigned char>(sq);
            break;
        case 1:
            squares[subscript] &= 0xF3;
            squares[subscript] |= static_cast<unsigned char>(sq) << 2;
            break;
        case 2:
            squares[subscript] &= 0xCF;
            squares[subscript] |= static_cast<unsigned char>(sq) << 4;
            break;
        case 3:
            squares[subscript] &= 0x3F;
            squares[subscript] |= static_cast<unsigned char>(sq) << 6;
        }
    }

    Board::Board() noexcept : squares{0}, piece_cnt(4), next_player(Player::Black) {
        set(4, 4, Square::Black);
        set(5, 5, Square::Black);
        set(4, 5, Square::White);
        set(5, 4, Square::White);
        for (int i = 0; i < MAX_FILES + 2; i++) {
            set(0, i, Square::OutOfRange);
            set(MAX_FILES + 1, i, Square::OutOfRange);
            set(i, 0, Square::OutOfRange);
            set(i, MAX_FILES + 1, Square::OutOfRange);
        }
    }

    Square Board::operator() (int x, int y) const noexcept {
        const int sub = (x * (MAX_FILES + 2) + y) >> 2;
        const int partit = (x * (MAX_FILES + 2) + y) & 3;
        return Square((squares[sub] >> (partit << 1)) & 3);
    }

    Square Board::at(int x, int y) const noexcept {
        if (1 <= x && x <= MAX_FILES && 1 <= y && y <= MAX_RANK)
            return (*this)(x, y);
        return Square::OutOfRange;
    }

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

    bool Board::is_placable(int x, int y) const noexcept {
        static constexpr int dx[] = { -1, 0, 1, -1, 1, -1, 0, 1 };
        static constexpr int dy[] = { -1, -1, -1, 0, 0, 1, 1, 1 };
        const Square opponent = next_player == Player::Black ? Square::White : Square::Black;
        const Square player = next_player == Player::Black ? Square::Black : Square::White;
        // If (x, y) is out of range, returns here.
        if (at(x, y) != Square::Empty)
            return false;
        // Now we can assume that (x, y) is not out of range.
        for (int k = 0; k < 8; k++) {
            int currx = x + dx[k], curry = y + dy[k];
            // Since dx[] and dy[] have abs value <= 1, even if (currx, curry)
            // is out of range, we can safely access it because of the margin
            // elements in square.
            if ((*this)(currx, curry) != opponent)
                continue;
            do {
                currx += dx[k];
                curry += dy[k];
            } while ((*this)(currx, curry) == opponent);
            // Now (currx, curry) is not an opponent piece.
            // Reversi says that we can place here if we have our piece here.
            if ((*this)(currx, curry) == player)
                return true;
        }
        return false;
    }

    std::vector<std::pair<int, int>> Board::get_placable() const {
        std::vector<std::pair<int, int>> ans;
        for (int i = 1; i <= MAX_FILES; i++)
            for (int j = 1; j <= MAX_RANK; j++)
                if (is_placable(i, j))
                    ans.push_back({ i, j });
        return ans;
    }

    TEST_CASE("placable") {
        Board b;
        REQUIRE(b.at(4, 4) == Square::Black);
        CHECK(!b.is_placable(4, 4));
        CHECK(!b.is_placable(4, 5));
        CHECK(!b.is_placable(-1, -1));
        CHECK(!b.is_placable(9, 9));
        CHECK(b.is_placable(4, 6));
        CHECK(b.get_placable() == std::vector<std::pair<int, int>>{ {3, 5}, {4, 6}, {5, 3}, {6, 4} });
    }

    void Board::place(int x, int y) {
        if (x <= 0 || x > MAX_FILES || y <= 0 || y > MAX_FILES)
            throw std::out_of_range("place argument out of range");
        static constexpr int dx[] = { -1, 0, 1, -1, 1, -1, 0, 1 };
        static constexpr int dy[] = { -1, -1, -1, 0, 0, 1, 1, 1 };
        const Square opponent = next_player == Player::Black ? Square::White : Square::Black;
        const Square player = next_player == Player::Black ? Square::Black : Square::White;
        for (int k = 0; k < 8; k++) {
            int currx = x + dx[k], curry = y + dy[k];
            // Since dx[] and dy[] have abs value <= 1, even if (currx, curry)
            // is out of range, we can safely access it because of the margin
            // elements in square.
            if ((*this)(currx, curry) != opponent)
                continue;
            do {
                currx += dx[k];
                curry += dy[k];
            } while ((*this)(currx, curry) == opponent);
            // Now (currx, curry) is not an opponent piece.
            // Reversi says that we can place here if we have our piece here.
            if ((*this)(currx, curry) == player) {
                currx -= dx[k];
                curry -= dy[k];
                do {
                    set(currx, curry, player);
                    currx -= dx[k];
                    curry -= dy[k];
                } while (currx != x || curry != y);
                // Loop stops when currx, curry == x, y
            }
        }
        // Finally put our piece here
        set(x, y, player);
        // Let's abuse the function.
        skip();
    }

    MatchResult Board::count() const noexcept {
        int bcnt = 0, wcnt = 0;
        for (int ix = 1; ix <= MAX_FILES; ix++) {
            for (int iy = 1; iy <= MAX_RANK; iy++) {
                switch ((*this)(ix, iy)) {
                    case Square::White: ++wcnt; break;
                    case Square::Black: ++bcnt; break;
                }
            }
        }
        if (wcnt < bcnt)
            return MatchResult::Black;
        else if (wcnt == bcnt)
            return MatchResult::Draw;
        else
            return MatchResult::White;
    }

    TEST_CASE("simple game") {
        Board b;
        // The game starts with black and an even position
        CHECK(b.whos_next() == Player::Black);
        CHECK(b.count() == MatchResult::Draw);
        REQUIRE(b(4, 4) == Square::Black);
        REQUIRE(b.is_placable(4, 6));
        b.place(4, 6);
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