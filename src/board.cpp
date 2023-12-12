#include "game.h"
#include <stdexcept>

namespace Reversi {
    void Board::set(int x, int y, Square sq) noexcept {
        const int subscript = ((MAX_FILES + 2) * x + y) >> 2;
        const int partit = ((MAX_FILES + 2) * x + y) & 3;
        switch (partit) {
        case 0:
            mSquares[subscript] &= 0xFC;
            mSquares[subscript] |= static_cast<unsigned char>(sq);
            break;
        case 1:
            mSquares[subscript] &= 0xF3;
            mSquares[subscript] |= static_cast<unsigned char>(sq) << 2;
            break;
        case 2:
            mSquares[subscript] &= 0xCF;
            mSquares[subscript] |= static_cast<unsigned char>(sq) << 4;
            break;
        case 3:
            mSquares[subscript] &= 0x3F;
            mSquares[subscript] |= static_cast<unsigned char>(sq) << 6;
        }
    }

    Board::Board() noexcept : mSquares{0}, mNextPlayer(Player::Black) {
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
        return Square((mSquares[sub] >> (partit << 1)) & 3);
    }

    Square Board::at(int x, int y) const noexcept {
        if (1 <= x && x <= MAX_FILES && 1 <= y && y <= MAX_RANK)
            return (*this)(x, y);
        return Square::OutOfRange;
    }

    bool Board::is_placable(int x, int y) const noexcept {
        static constexpr int dx[] = { -1, 0, 1, -1, 1, -1, 0, 1 };
        static constexpr int dy[] = { -1, -1, -1, 0, 0, 1, 1, 1 };
        const Square opponent = mNextPlayer == Player::Black ? Square::White : Square::Black;
        const Square player = mNextPlayer == Player::Black ? Square::Black : Square::White;
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

    bool Board::is_skip_legal() const noexcept {
        for (int i = 1; i <= MAX_FILES; i++)
            for (int j = 1; j <= MAX_RANK; j++)
                if (is_placable(i, j))
                    return false;
        return true;
    }

    void Board::place(int x, int y) {
        if (x <= 0 || x > MAX_FILES || y <= 0 || y > MAX_FILES)
            throw std::out_of_range("place argument out of range");
        static constexpr int dx[] = { -1, 0, 1, -1, 1, -1, 0, 1 };
        static constexpr int dy[] = { -1, -1, -1, 0, 0, 1, 1, 1 };
        const Square opponent = mNextPlayer == Player::Black ? Square::White : Square::Black;
        const Square player = mNextPlayer == Player::Black ? Square::Black : Square::White;
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
        // Let's abuse the function. This also updates the cache.
        skip();
    }

    MatchResult Board::count() const noexcept {
        int bcnt = 0, wcnt = 0;
        for (int ix = 1; ix <= MAX_FILES; ix++) {
            for (int iy = 1; iy <= MAX_RANK; iy++) {
                switch ((*this)(ix, iy)) {
                    case Square::White: ++wcnt; break;
                    case Square::Black: ++bcnt; break;
                    default: break; // It might be an empty square, do nothing
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

    bool operator == (const Board& lhs, const Board& rhs) noexcept {
        // Put the simple comparison first
        return lhs.mNextPlayer == rhs.mNextPlayer && lhs.mSquares == rhs.mSquares;
    }
}

// The hash specialization
std::size_t std::hash<Reversi::Board>::operator() (const Reversi::Board& b) const noexcept {
    static_assert(std::is_unsigned_v<std::size_t>, "Size type should be unsigned!");
    static_assert(sizeof(std::size_t) == 8, "Expects size_t to be 8 bytes long!");
    // Copied from cppreference
    std::size_t ans = 0xcbf29ce484222325;
    constexpr std::size_t prime = 0x100000001B3;
    // Who's to play is also an important part of the information.
    ans = (ans * prime) ^ static_cast<std::size_t>(b.whos_next());
    for (int i = 1; i <= 8; i++) {
        for (int j = 1; j <= 8; j++)
            // FIXME: After a few test runs, remove this at!
            ans = (ans * prime) ^ static_cast<std::size_t>(b(i, j));
    }
    return ans;
}