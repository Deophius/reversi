#ifndef REVERSI_BOARD_H
#define REVERSI_BOARD_H
#include <array>
#include <iosfwd>
#include <vector>

namespace Reversi {
    // Friendly enum representation of the status of a square
    enum class Square : unsigned char {
        Empty, Black, White, OutOfRange
    };

    // Result of the game.
    enum class MatchResult : unsigned char {
        Black, White, Draw, InProgress
    };

    // Player enum
    enum class Player : unsigned char {
        Black, White
    };

    // The game board.
    class Board {
    public:
        // First some constexpr computations
        // The metric of the board
        constexpr static int MAX_FILES = 8, MAX_RANK = 8;

    private:
        constexpr static int ARR_SIZE = ((MAX_RANK + 2) * (MAX_FILES + 2) * 2 + 7) / 8;
        // Compressed array of the squares
        std::array<unsigned char, ARR_SIZE> squares;
        // The number of pieces on the board
        unsigned char piece_cnt;
        // The next player to play
        Player next_player;

        // Sets (x, y) to sq.
        // No range checking.
        void set(int x, int y, Square sq) noexcept;

    public:
        // Constructs the Board object with the initial position.
        Board() noexcept;

        // Gets the square at (x, y), without range checking.
        /// @note To prevent invalid modification, this is read only.
        Square operator() (int x, int y) const noexcept;

        // Gets the square at (x, y). Returns OutOfRange if the requested
        /// (x, y) is out of range.
        Square at(int x, int y) const noexcept;

        // Checks if the block (x, y) is placable by the next player to play.
        // False if out of range.
        bool is_placable(int x, int y) const noexcept;

        // Returns a vector of all (x, y) pairs at which the player can put his
        // next piece. Simple wrapper around is_placable.
        std::vector<std::pair<int, int>> get_placable() const;

        // Places a piece at (x, y).
        // Doesn't check whether this is valid.
        // If (x, y) is out of range, throws std::out_of_range.
        void place(int x, int y);

        // Skips the current player's turn.
        // Doesn't check that the skip is legitimate.
        inline void skip() noexcept {
            next_player = static_cast<Player>(1 - static_cast<unsigned char>(next_player));
        }

        // Assuming that both sides have nowhere to go, counts the material and
        // returns the outcome of the game.
        MatchResult count() const noexcept;

        // Returns the next player to play
        inline Player whos_next() const noexcept {
            return next_player;
        }
    };

    // Manager of a game
    class GameMan {
        // The steps from the beginning of the game till now.
        // (0, 0) means a skip.
        std::vector<std::pair<int, int>> annotation;
        // Invariant: board is consistent with annotation
        Board board;
        // true if the previous move is a skip
        bool prev_skip;
        // The match result
        MatchResult result;
    public:
        // Creates a new manager instance, with annotation and board set to the initial position.
        GameMan();

        // Serializes the annotation to an ostream.
        friend std::ostream& operator<< (std::ostream& ostr, const GameMan& game);

        // Extracts annotation from an istream.
        // If the read fails, sets failbit of istr; if the annotation is buggy,
        // throws std::runtime_error. In both cases, game is not modified.
        friend std::istream& operator>> (std::istream& istr, GameMan& game);

        // Returns a const ref to the board since that couldn't violate our invariant
        const Board& view_board() const noexcept;

        // Places a piece at (x, y).
        // If get_result() != InProgress, throws std::logic_error.
        // If (x, y) is out of range, throws std::out_of_range.
        void place(int x, int y);

        // Skips the current player's turn.
        // On the second consecutive call to skip(), sets match result.
        // If the skip is not legitimate, throws std::logic_error.
        // If get_result() != InProgress, throws std::logic_error.
        void skip();

        // Gets the match result.
        MatchResult get_result() const noexcept;

        // Resets the board to initial state and clears the annotation.
        void reset() noexcept;
    };
}
#endif