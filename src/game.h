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
        std::array<unsigned char, ARR_SIZE> mSquares;
        // The next player to play
        Player mNextPlayer;
        // Caches the result of get_placable(). It's a responsiblity of all methods
        // that can modify the board to keep this up to date.
        std::vector<std::pair<int, int>> mPlacableCache;

        // Sets (x, y) to sq.
        // No range checking.
        void set(int x, int y, Square sq) noexcept;

        // Updates the placable cache
        void update_placable_cache();

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
        // This function is not cached.
        bool is_placable(int x, int y) const noexcept;

        // Returns a vector of all (x, y) pairs at which the player can put his
        // next piece. Simple wrapper around is_placable.
        const std::vector<std::pair<int, int>>& get_placable() const;

        // Places a piece at (x, y).
        // Doesn't check whether this is valid.
        // If (x, y) is out of range, throws std::out_of_range.
        void place(int x, int y);

        // Skips the current player's turn.
        // Doesn't check that the skip is legitimate.
        inline void skip() noexcept {
            mNextPlayer = static_cast<Player>(1 - static_cast<unsigned char>(mNextPlayer));
            update_placable_cache();
        }

        // Assuming that both sides have nowhere to go, counts the material and
        // returns the outcome of the game.
        MatchResult count() const noexcept;

        // Returns the next player to play
        inline Player whos_next() const noexcept {
            return mNextPlayer;
        }
    };

    // Manager of a game
    class GameMan {
        // The steps from the beginning of the game till now.
        // (0, 0) means a skip.
        std::vector<std::pair<int, int>> mAnnotation;
        // Invariant: board is consistent with annotation
        Board mBoard;
        // true if the previous move is a skip
        bool mPrevSkip;
        // The match result
        MatchResult mResult;
        // Since this is a GUI application, we need to allow the user to register
        // event callbacks. In this case, when the one player makes a move.
        std::map<std::string, std::function<void(int, int)>> mListeners;
        // For saving purposes. If `annotation` has changed since the last save,
        // this is set to true.
        mutable bool mDirtyFile = false;
    public:
        // Creates a new manager instance, with annotation and board set to the initial position.
        GameMan() {
            mAnnotation.reserve(Board::MAX_FILES * Board::MAX_RANK * 2);
            reset();
            listen("annotator", [this](int x, int y){ this->mAnnotation.push_back({ x, y }); });
        }

        // Serializes the annotation to an ostream.
        // Sets the dirty flag to false if the write is successful.
        friend std::ostream& operator<< (std::ostream& ostr, const GameMan& game);

        // Extracts annotation from an istream.
        // If the read fails, throws runtime_error; if the annotation is buggy,
        // throws ReversiError. In both cases, game is not modified.
        friend std::istream& operator>> (std::istream& istr, GameMan& game);

        // Returns a const ref to the board since that couldn't violate our invariant
        inline const Board& view_board() const noexcept {
            return mBoard;
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

        // Places or skips (skips when (x, y) == (0, 0))
        void place_skip(std::pair<int, int> pos);

        // Gets the match result.
        inline MatchResult get_result() const noexcept {
            return mResult;
        }

        // Resets the board to initial state and clears the annotation.
        // Note that callbacks are not deleted.
        inline void reset() noexcept {
            mAnnotation.clear();
            mBoard = Board();
            mPrevSkip = false;
            mResult = MatchResult::InProgress;
            // There's no point saving the starting position
            mDirtyFile = false;
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

        // Checks if the annotations are dirty and needs saving.
        inline bool is_dirty() const noexcept {
            return mDirtyFile;
        }
    };
}
#endif