#include "engi.h"

namespace Reversi {
    Engine::Engine() : mThread(&Engine::mainloop, this) {}

    Engine::~Engine() noexcept {
        // First we notify the thread to exit
        {
            std::lock_guard lk(mMutex);
            mSemaphore = Sema::Exit;
        }
        mCondVar.notify_one();
        mThread.join();
    }

    void Engine::mainloop() {
        while (true) {
            std::unique_lock lk(mMutex);
            mCondVar.wait(lk, [this]{ return mSemaphore != Sema::None; });
            // Now we have the mutex locked, and semaphore != none
            if (mSemaphore == Sema::Exit)
                break;
            // semaphore == Sema::compute
            do_make_move();
        }
    }

    void Engine::enter_move(int x, int y) {
        std::lock_guard lk(mMutex);
        if (x && y)
            mBoard.place(x, y);
        else
            mBoard.skip();
    }

    void Engine::change_position(Board new_pos) {
        std::lock_guard lk(mMutex);
        mBoard = std::move(new_pos);
    }

    void Engine::request_compute(std::promise<std::pair<int, int>> prom) {
        {
            std::lock_guard lk(mMutex);
            mPromise = std::move(prom);
            mSemaphore = Sema::Compute;
        }
        mCondVar.notify_one();
    }

    void RandomChoice::do_make_move() {
        mPromise.set_value(
            mBoard.get_placable().size() ?
            mBoard.get_placable().at(mRandomGen() % mBoard.get_placable().size()) :
            std::pair(0, 0)
        );
    }
}