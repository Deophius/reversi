#include "game.h"
#include "engi.h"
#include "reversi_widgets.h"

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
                    mWhiteSide->enter_move({ x, y });
                    mBlackSide->enter_move({ x, y });
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
                        mWhiteSide->request_compute(this, mGameID);
                    else
                        mBlackSide->request_compute(this, mGameID);
                }
            }
        }
    }

    GameMan::GameMan(MainWindow& mw) : mMainWindow(mw), mThread(&GameMan::mainloop, this) {
        mAnnotation.reserve(128);
    }

    GameMan::~GameMan() noexcept {
        {
            std::lock_guard lk(mMutex);
            mSemaQueue.push(0);
        }
        mCondVar.notify_one();
        mThread.join();
    }

    void GameMan::load_black_engine(Engine* e) {
        std::lock_guard lk(mMutex);
        mBlackSide = e;
    }

    void GameMan::load_white_engine(Engine* e) {
        std::lock_guard lk(mMutex);
        mWhiteSide = e;
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
        mBlackSide->request_compute(this, mGameID);
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
            ->request_compute(this, mGameID);
    }
}