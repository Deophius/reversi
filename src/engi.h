// Interface for the engine
#ifndef REVERSI_ENGI_H
#define REVERSI_ENGI_H
#include "game.h"
#include <chrono>
#include <functional>
#include <thread>
#include <future>
#include <atomic>
#include <condition_variable>

namespace Reversi {
    // An interface for the async engine.
    class Engine {
        // The semaphore that is given by the main thread.
        // Exit is only set by the dtor of the class.
        // Compute means the manager wants to the engine to compute the next move.
        // By design the manager thread should be asleep when computation is being
        // carried out.
        enum class Sema {
            None, Exit, Compute
        } mSemaphore = Sema::None;
        // The game id passed in by `request_computation()`.
        unsigned char mGameID = 0;
        // The pointer to game manager passed in by request_computation.
        std::weak_ptr<GameMan> mGameMan;
    protected:
        // The board, accessible by derived classes.
        Board mBoard;
        // Atomic bool that signals a cancellation.
        // We can't use the mutex for sync because the mutex is held by the class
        // thread when computation is being done.
        std::atomic_bool mCancel = false;

        // Interesting exception that can be used to cancel the do_make_move().
        struct OperationCanceled {};
    private:
        // The mutex guards mSemaphore and mBoard, mGameID and mGameMan
        std::mutex mMutex;
        // This condition variable works with mMutex.
        // The thread that runs mainloop() waits on this cond var for notification
        // from the manager's thread.
        std::condition_variable mCondVar;
        // The thread handle to the main thread (as opposed to worker threads
        // derived classes may spawn).
        std::thread mThread;

        // The mainloop of the thread. Waits on the condition var to handle cmds.
        // I don't think there's any way to override this correctly.
        void mainloop();

        // important: Customization point
        // Computes the move.
        // Ran by the thread owned by this class. This function can safely access
        // the members protected by the class's mutex.
        // The function should be aware that a cancellation request may come at any
        // time and should respect that by throwing OperationCanceled.
        virtual std::pair<int, int> do_make_move() = 0;

    public:
        // Constructs the engine, launching the main thread.
        Engine();

        // Prohibit copying and moving.
        Engine(const Engine&) = delete;

        Engine& operator= (const Engine&) = delete;

        Engine(Engine&&) = delete;

        Engine& operator= (Engine&&) = delete;

        // Virtual destructor. Joins the thread handle, so it's very important
        // to set this as a virtual function.
        virtual ~Engine() noexcept;

        // Enter a move. Since the engine should be wrapped inside
        // some interface, the input should be legal.
        // (0, 0) means a skip.
        // This should be called by the game manager thread.
        void enter_move(std::pair<int, int> mov);

        // (Game manager thread) The position changed so much that it's
        // simpler to pass in a brand new board.
        void change_position(Board new_pos);

        // (Game manager thread) Requests computation of next move.
        // This doesn't block.
        void request_compute(unsigned char gid);

        // (Game man) Requests cancellation of current or the next computation.
        // This does not block.
        void request_cancel();

        // (Game man) links to a game manager.
        void link_game_man(std::weak_ptr<GameMan> gm);

        // important: Customization point.
        // Returns the name of the type. (Some sort of manual RTTI)
        // Cannot be set to static function since this needs to access the vtable.
        virtual std::string get_name() = 0;
    };

    // For testing purposes. This engine randomly selects an available move.
    class RandomChoice : public Engine {
        std::function<std::size_t()> mRandomGen;

        // Overrides the customization point
        virtual std::pair<int, int> do_make_move() override;
    public:
        // Default constructor.
        RandomChoice();

        virtual ~RandomChoice() noexcept = default;

        virtual std::string get_name() override;
    };

    // Fwd for the MainWindow class.
    class MainWindow;

    // Constructs a new std::unique_ptr<Engine> that points
    // to an object of the correct derived type of Engine.
    // Throws ReversiError if the engine's name isn't recognized.
    std::unique_ptr<Engine> make_engine_from_description(
        const std::string& name, MainWindow& mw);
}

#endif