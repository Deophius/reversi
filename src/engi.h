// Interface for the engine
#ifndef REVERSI_ENGI_H
#define REVERSI_ENGI_H
#include "game.h"
#include <chrono>
#include <functional>
#include <thread>
#include <future>
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
    protected:
        // The board, accessible by derived classes.
        Board mBoard;
        // The promise where the result of do_make_move() should be put. Because the
        // UIE needs to move this into 
        std::promise<std::pair<int, int>> mPromise;
    private:
        // The mutex guards mPromise, mSemaphore and mBoard.
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
        // Computes the move and puts it into the promise held by mPromise.
        // Ran by the thread owned by this class. This function can safely access
        // the members protected by the class's mutex.
        virtual void do_make_move() = 0;

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

        // Enter opponent's move. Since the engine should be wrapped inside
        // some interface, the input should be legal.
        // (0, 0) means a skip.
        // This should be called by the game manager thread.
        void enter_move(int x, int y);

        // (Game manager thread) The position changed so much that it's
        // simpler to pass in a brand new board.
        void change_position(Board new_pos);

        // (Game manager thread) Requests computation of next move.
        // The result is obtained by the future associated with the promise
        // moved in.
        void request_compute(std::promise<std::pair<int, int>> prom);
    };

    // For testing purposes. This engine randomly selects an available move.
    class RandomChoice : public Engine {
        std::function<std::size_t()> mRandomGen;

        // Overrides the customization point
        virtual void do_make_move() override;
    public:
        // Default constructor.
        RandomChoice();

        virtual ~RandomChoice() noexcept = default;
    };
}

#endif