#ifndef REVERSI_BOARD_H
#define REVERSI_BOARD_H
#include <array>
#include <iosfwd>
#include <stdexcept>
#include <vector>
#include <functional>
#include <map>

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

    // Violation of reversi rules
    struct ReversiError : public std::runtime_error {
        using std::runtime_error::runtime_error;
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
        // Since this is a GUI application, we need to allow the user to register
        // event callbacks. In this case, when the one player makes a move.
        std::map<std::string, std::function<void(int, int)>> listeners;
    public:
        // Creates a new manager instance, with annotation and board set to the initial position.
        GameMan() {
            annotation.reserve(Board::MAX_FILES * Board::MAX_RANK * 2);
            reset();
            listen("annotator", [this](int x, int y){ this->annotation.push_back({ x, y }); });
        }

        // Serializes the annotation to an ostream.
        friend std::ostream& operator<< (std::ostream& ostr, const GameMan& game);

        // Extracts annotation from an istream.
        // If the read fails, sets failbit of istr; if the annotation is buggy,
        // throws ReversiError. In both cases, game is not modified.
        friend std::istream& operator>> (std::istream& istr, GameMan& game);

        // Returns a const ref to the board since that couldn't violate our invariant
        inline const Board& view_board() const noexcept {
            return board;
        }

        // Places a piece at (x, y).
        // If get_result() != InProgress, throws std::logic_error.
        // If the board thinks (x, y) is not placable (including out of range), throws ReversiError.
        void place(int x, int y);

        // Skips the current player's turn.
        // On the second consecutive call to skip(), sets match result.
        // If get_result() != InProgress, throws std::logic_error.
        // If the skip is not legitimate, throws ReversiError.
        void skip();

        // Gets the match result.
        inline MatchResult get_result() const noexcept {
            return result;
        }

        // Resets the board to initial state and clears the annotation.
        // Note that callbacks are not deleted.
        inline void reset() noexcept {
            annotation.clear();
            board = Board();
            prev_skip = false;
            result = MatchResult::InProgress;
        }

        // Adds an event listener to *this. The callback is called every time
        // one side makes a move or skips.
        //
        // The callback should receive (x, y), the previous move played as argument.
        // It's assumed that the callback can access *this in some way, so *this is
        // not provided as part of the argument.
        // As usual, if it's a skip, then (0, 0) is passed in.
        //
        // For easy unregistration, a name is required for every listener.
        // If the `name` is already used, the new callback isn't registered, and
        // the method returns `false`. Otherwise returns `true`.
        bool listen(std::string name, std::function<void(int, int)> cb);

        // Unregisters a listener referred to by `name`. If there is such a listener,
        // erases it from the callback list and returns true; if there isn't, simply
        // returns false.
        bool unhook(const std::string& name);
    };
}
#endif