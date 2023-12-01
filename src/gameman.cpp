#include "game.h"
#include "engi.h"
#include "reversi_widgets.h"
#include <iostream>

namespace Reversi {
    void GameMan::mainloop() {
        while (true) {
            std::unique_lock lk(mMutex);
            // Because we just acquired the mutex, the call to empty() is thread safe.
            if (mSemaQueue.empty())
                mCondVar.wait(lk, [this]{ return !mSemaQueue.empty(); });
            // Now we have the mutex.
            auto cmd = mSemaQueue.front();
            mSemaQueue.pop();
            switch (cmd & 0xFF) {
                case 0: // Exit
                    return;
                case 3: {
                    const int x = (cmd >> 8) & 0xFF, y = (cmd >> 16) & 0xFF,
                        id = cmd >> 24;
                    if (id != (int)mGameID || !mGameInProgress)
                        break;
                    mDirty = true;
                    mAnnotation.emplace_back(x, y);
                    // We need to inform both sides of the last move.
                    // Because we hold the engines as unique pointers, and we're still
                    // running in the joining thread, we infer that the dtor hasn't
                    // finished running and the engines haven't been destructed.
                    mWhiteSide->enter_move({ x, y });
                    mBlackSide->enter_move({ x, y });
                    std::cerr << "Manager access engine\n";
                    if (x) {
                        mBoard.place(x, y);
                        mPrevSkip = false;
                    } else {
                        if (mPrevSkip) {
                            // Game finished.
                            mGameInProgress = false;
                            ++mGameID;
                            mMainWindow.announce_game_result(mBoard.count());
                            break;
                        }
                        mBoard.skip();
                        mPrevSkip = true;
                    }
                    if (!mGameInProgress)
                        break;
                    // The game is still in progress, we can proceed to the next move.
                    if (mBoard.whos_next() == Player::White)
                        mWhiteSide->request_compute(shared_from_this(), mGameID);
                    else
                        mBlackSide->request_compute(shared_from_this(), mGameID);
                }
            }
        }
    }

    GameMan::GameMan(MainWindow& mw, PrivateTag) :
        mMainWindow(mw), mThread(&GameMan::mainloop, this),
        mWhiteSide(nullptr), mBlackSide(nullptr)
    {
        mAnnotation.reserve(128);
    }

    std::shared_ptr<GameMan> GameMan::create(MainWindow& mw) {
        return std::make_shared<GameMan>(mw, PrivateTag());
    }

    GameMan::~GameMan() noexcept {
        std::cerr << "~GameMan()\n";
        {
            std::lock_guard lk(mMutex);
            mSemaQueue.push(0);
            mGameInProgress = false;
        }
        mCondVar.notify_one();
        mThread.join();
    }

    void GameMan::load_black_engine(std::unique_ptr<Engine> e) {
        std::lock_guard lk(mMutex);
        mBlackSide = std::move(e);
    }

    void GameMan::load_white_engine(std::unique_ptr<Engine> e) {
        std::lock_guard lk(mMutex);
        mWhiteSide = std::move(e);
    }

    void GameMan::start_new() {
        std::lock_guard lk(mMutex);
        if (!(mWhiteSide && mBlackSide))
            throw ReversiError("Two sides should both have their engine loaded.");
        if (mWhiteSide == mBlackSide)
            throw ReversiError("You can't let one engine play two sides.");
        // Reset the game related data
        mAnnotation.clear();
        mBoard = Board();
        mPrevSkip = mDirty = false;
        ++mGameID;
        mGameInProgress = true;
        mBlackSide->request_compute(shared_from_this(), mGameID);
    }

    void GameMan::enter_move(std::pair<int, int> mov, unsigned char gid) {
        std::unique_lock lk(mMutex);
        if (gid == mGameID) {
            mSemaQueue.push(3 + (mov.first << 8) + (mov.second << 16) + ((unsigned)(gid) << 24));
            lk.unlock();
            mCondVar.notify_one();
        }
    }

    void GameMan::pause_game() {
        std::lock_guard lk(mMutex);
        if (!mGameInProgress)
            return;
        mWhiteSide->request_cancel();
        mBlackSide->request_cancel();
        ++mGameID;
        mGameInProgress = false;
    }

    void GameMan::resume_game() {
        std::lock_guard lk(mMutex);
        if (mGameInProgress)
            return;
        mGameInProgress = true;
        (mBoard.whos_next() == Player::Black ? mBlackSide : mWhiteSide)
            ->request_compute(shared_from_this(), mGameID);
    }
}