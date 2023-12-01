#ifndef REVERSI_GAME_H
#define REVERSI_GAME_H
#include <array>
#include <mutex>
#include <stdexcept>
#include <vector>
#include <thread>
#include <memory>
#include <queue>

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

    // interface fwd
    class Engine;
    class MainWindow;

    // Manager of a game
    class GameMan : public std::enable_shared_from_this<GameMan> {
        // The steps from the beginning of the game till now.
        // (0, 0) means a skip.
        std::vector<std::pair<int, int>> mAnnotation;
        // Invariant: board is consistent with annotation
        Board mBoard;
        // true if the previous move is a skip
        bool mPrevSkip = false;
        // The two engines that play against each other
        std::unique_ptr<Engine> mWhiteSide, mBlackSide;
        // Keeps track if *this has been modified after the last save.
        bool mDirty = false;
        // An incrementing count of games played. This is used to distinguish
        // between different games.
        unsigned char mGameID = 0;
        // true if a game is in progress. This is used by the mainloop to determine
        // whether to get more moves from the engine.
        bool mGameInProgress = false;
        // The main GUI window. We have to take the responsibility to redraw the
        // GUI because the GUI thread is blocked by exec() waiting for events.
        // FIXME: Current impl doesn't contain call to GUI functions.
        MainWindow& mMainWindow;
        // The game manager thread.
        std::thread mThread;
        // Commands are sent here. The least significant 8 bits are the command.
        // Available:
        //   0 -- Exit the thread
        //   1 -- Cancel engine computation, for example, to start a new game, or
        //        to load a new position. (unimplemented!)
        //   2 -- Starts asking the engines to compute the move and push the game
        //        forward, automatically drawing the GUI (unimplemented!)
        //   3 -- (sent by engines) x = 9~16 bits, y = 17~24bits, places at (x, y)
        //        game id = 25~32 bits
        std::queue<unsigned int> mSemaQueue;
        // Mutex that protects almost everything in this class.
        std::mutex mMutex;
        // Condition variable to notify the mainloop thread when there are new
        // entries in the queue to process.
        std::condition_variable mCondVar;

        // The mainloop of mThread.
        void mainloop();

        struct PrivateTag {};

    public:
        GameMan(MainWindow& mw, PrivateTag);

        // Constructs a new game manager linked to the main window `mw`.
        static std::shared_ptr<GameMan> create(MainWindow& mw);

        // Disallow copying and moving
        GameMan(const GameMan&) = delete;

        GameMan(GameMan&&) = delete;

        GameMan& operator= (const GameMan&) = delete;

        GameMan& operator= (GameMan&&) = delete;

        virtual ~GameMan() noexcept;

        // Loads the white engine and returns the previous engine.
        // Must be called when there's no game in progress.
        void load_white_engine(std::unique_ptr<Engine> e);

        // Loads the black engine and returns the previous one.
        // Must be called when there's no game in progress.
        void load_black_engine(std::unique_ptr<Engine> e);

        // (GUI thread)
        // Starts a new game in the mainloop.
        // If any side doesn't have an engine loaded, throws Reversi error.
        void start_new();

        // (Engine thread)
        // Enters the next move.
        void enter_move(std::pair<int, int> mov, unsigned char game_id);

        // (GUI thread)
        // Pauses the current game. Increments the game id counter to refuse
        // input from the engines.
        void pause_game();

        // (GUI thread)
        // Resumes the current game.
        void resume_game();
    };
}
#endif