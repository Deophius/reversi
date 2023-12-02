#include "engi.h"
#include <random>
#include <iostream>

namespace Reversi {
    Engine::Engine() : mThread(&Engine::mainloop, this) {}

    Engine::~Engine() noexcept {
        std::cerr << "~Engine()\n";
        // First we notify the thread to exit
        {
            std::lock_guard lk(mMutex);
            mSemaphore = Sema::Exit;
        }
        mCondVar.notify_one();
        mThread.join();
        std::cerr << "Engine thread joined\n";
    }

    void Engine::mainloop() {
        while (true) {
            std::unique_lock lk(mMutex);
            mCondVar.wait(lk, [this]{ return mSemaphore != Sema::None; });
            // Now we have the mutex locked, and semaphore != none
            if (mSemaphore == Sema::Exit)
                break;
            // semaphore == Sema::compute
            try {
                auto result = do_make_move();
                if (!mCancel.load(std::memory_order_acquire)) {
                    // important: Thread safety
                    // Case 1: exception thrown
                    //   All further calls will continue to throw the exception,
                    //   so no pointer access will be made, so no dangling pointers.
                    // Case 2: exception not thrown
                    //   Then we have a shared pointer that prevents the manager from being destroyed.
                    //   This means it's safe to access the manager.
                    std::shared_ptr<GameMan>(mGameMan)->enter_move(result, mGameID);
                }
            } catch (OperationCanceled) {
            } catch (const std::bad_weak_ptr&) {
                // The related object has already been destructed.
            }
            // Reset the flags
            mCancel.store(false, std::memory_order_release);
            mSemaphore = Sema::None;
        }
    }

    void Engine::enter_move(std::pair<int, int> mov) {
        std::lock_guard lk(mMutex);
        if (mov.first)
            mBoard.place(mov.first, mov.second);
        else
            mBoard.skip();
    }

    void Engine::change_position(Board new_pos) {
        std::lock_guard lk(mMutex);
        mBoard = std::move(new_pos);
    }

    void Engine::request_compute(unsigned char gid) {
        {
            std::lock_guard lk(mMutex);
            mCancel.store(false, std::memory_order_release);
            mGameID = gid;
            mSemaphore = Sema::Compute;
        }
        mCondVar.notify_one();
    }

    void Engine::link_game_man(std::weak_ptr<GameMan> gm) {
        std::lock_guard lk(mMutex);
        mGameMan = gm;
    }

    void Engine::request_cancel() {
        mCancel.store(true, std::memory_order_release);
    }

    RandomChoice::RandomChoice() {
        std::mt19937 mt(std::chrono::system_clock::now().time_since_epoch().count());
        std::uniform_int_distribution dist(0, 256);
        mRandomGen = std::bind(dist, mt);
    }

    std::pair<int, int> RandomChoice::do_make_move() {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        return mBoard.get_placable().size() ?
            mBoard.get_placable().at(mRandomGen() % mBoard.get_placable().size()) :
            std::pair(0, 0);
    }
}