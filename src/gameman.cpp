#include "game.h"
#include "engi.h"
#include "reversi_widgets.h"
#include <iostream>

namespace Reversi {
    void GameMan::mainloop() {
        while (true) {
            std::unique_lock lk(mQueueMutex);
            // Because we just acquired the mutex, the call to empty() is thread safe.
            if (mSemaQueue.empty())
                mCondVar.wait(lk, [this]{ return !mSemaQueue.empty(); });
            // Now we have the mutex.
            auto cmd = mSemaQueue.front();
            mSemaQueue.pop();
            // Accept input while we are processing
            lk.unlock();
            switch (cmd & 0xFF) {
                case 0: // Exit
                    return;
                case 3:
                    handle_place(cmd);
                    break;
            }
        }
    }

    void GameMan::handle_place(unsigned cmd) {
        std::unique_lock lk(mDataMutex);
        const int x = (cmd >> 8) & 0xFF, y = (cmd >> 16) & 0xFF,
            id = cmd >> 24;
        if (id != (int)mGameID || !mGameInProgress)
            return;
        mDirty = true;
        mAnnotation.emplace_back(x, y);
        // We need to inform both sides of the last move.
        // Because we hold the engines as unique pointers, and we're still
        // running in the joining thread, we infer that the dtor hasn't
        // finished running and the engines haven't been destructed.
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
                // The nana library contains an internal lock and we can't change
                // a reference, so let's just drop this lock
                lk.unlock();
                mMainWindow.announce_game_result(mBoard.count());
                return;
            }
            mBoard.skip();
            mPrevSkip = true;
        }
        // Since manager is contained in the MainWindow structure,
        // it's safe to call the GUI process.
        std::cerr << "Waiting for update\n";
        // Same as above
        lk.unlock();
        mMainWindow.update_board(mBoard, { x, y });
        lk.lock();
        std::cerr << "Update done!\n";
        if (!mGameInProgress)
            return;
        // The game is still in progress, we can proceed to the next move.
        if (mBoard.whos_next() == Player::White)
            mWhiteSide->request_compute(mGameID);
        else
            mBlackSide->request_compute(mGameID);
    }

    void GameMan::take_back() {
        // The new board
        Board b2;
        {
            std::lock_guard lk(mDataMutex);
            // A takeback takes back all the moves if the total number of moves is less than 2.
            for (std::size_t i = 0; i < std::max(mAnnotation.size(), std::size_t(2)) - 2; ++i) {
                if (mAnnotation[i].first == 0)
                    b2.skip();
                else
                    b2.place(mAnnotation[i].first, mAnnotation[i].second);
            }
        }
        pause_game();
        {
            std::lock_guard lk(mDataMutex);
            mBoard = b2;
            // Remember to modify the annotation, too.
            if (!mAnnotation.empty())
                mAnnotation.pop_back();
            if (!mAnnotation.empty())
                mAnnotation.pop_back();
            // Then we notify the engines of the change.
            mBlackSide->change_position(b2);
            mWhiteSide->change_position(b2);
        }
        mMainWindow.update_board(b2, mAnnotation.empty() ? std::pair(0, 0) : mAnnotation.back());
        resume_game();
    }

    GameMan::GameMan(MainWindow& mw, PrivateTag) :
        mWhiteSide(nullptr), mBlackSide(nullptr), mMainWindow(mw),
        mThread(&GameMan::mainloop, this)
    {
        mAnnotation.reserve(128);
    }

    std::shared_ptr<GameMan> GameMan::create(MainWindow& mw) {
        return std::make_shared<GameMan>(mw, PrivateTag());
    }

    GameMan::~GameMan() noexcept {
        std::cerr << "~GameMan()\n";
        {
            std::lock_guard lk(mQueueMutex);
            mSemaQueue.push(0);
        }
        pause_game();
        mCondVar.notify_one();
        mThread.join();
    }

    void GameMan::load_black_engine(std::unique_ptr<Engine> e) {
        std::lock_guard lk(mDataMutex);
        mBlackSide = std::move(e);
        if (mBlackSide)
            mBlackSide->link_game_man(weak_from_this());
    }

    void GameMan::load_white_engine(std::unique_ptr<Engine> e) {
        std::lock_guard lk(mDataMutex);
        mWhiteSide = std::move(e);
        if (mWhiteSide)
            mWhiteSide->link_game_man(weak_from_this());
    }

    bool GameMan::engines_loaded() {
        std::lock_guard lk(mDataMutex);
        return mWhiteSide && mBlackSide;
    }

    void GameMan::start_new() {
        std::lock_guard lk(mDataMutex);
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
        mMainWindow.update_board(mBoard, {0, 0});
        mBlackSide->request_compute(mGameID);
    }

    void GameMan::enter_move(std::pair<int, int> mov, unsigned char gid) {
        std::unique_lock lk(mQueueMutex);
        if (gid == mGameID) {
            mSemaQueue.push(3 + (mov.first << 8) + (mov.second << 16) + ((unsigned)(gid) << 24));
            lk.unlock();
            mCondVar.notify_one();
        }
    }

    void GameMan::pause_game() {
        std::cout << "Waiting for pause_game...\n";
        std::lock_guard lk(mDataMutex);
        std::cout << "Pause game lock acquired!\n";
        if (!mGameInProgress)
            return;
        mWhiteSide->request_cancel();
        mBlackSide->request_cancel();
        ++mGameID;
        mGameInProgress = false;
    }

    void GameMan::resume_game() {
        std::lock_guard lk(mDataMutex);
        if (mGameInProgress)
            return;
        mGameInProgress = true;
        (mBoard.whos_next() == Player::Black ? mBlackSide : mWhiteSide)
            ->request_compute(mGameID);
    }

    bool GameMan::is_dirty() {
        std::lock_guard lk(mDataMutex);
        return mDirty;
    }

    nlohmann::json GameMan::to_json() {
        std::lock_guard lk(mDataMutex);
        using nlohmann::json;
        json ans;
        ans["annotation"] = json::array();
        for (const auto& [x, y] : mAnnotation)
            ans["annotation"].push_back(json::array({ x, y }));
        ans["black"] = mBlackSide->get_name();
        ans["white"] = mWhiteSide->get_name();
        // Since we have saved, the game is no longer dirty
        mDirty = false;
        return ans;
    }

    void GameMan::read_annotation(const nlohmann::json& js) {
        using nlohmann::json;
        using namespace std::string_literals;
        try {
            Board b;
            std::vector<std::pair<int, int>> anno;
            anno.reserve(js.at("annotation").size());
            for (const auto& move : js["annotation"]) {
                anno.emplace_back(move[0], move[1]);
                if (move[0] == 0 && move[1] == 0) {
                    if (!b.is_skip_legal())
                        throw ReversiError("Invalid skip in annotation!");
                    b.skip();
                } else {
                    if (!b.is_placable(move[0], move[1]))
                        throw ReversiError("Invalid place in annotation");
                    b.place(move[0], move[1]);
                }
            }
            // If we have survivied until now, that means the data is OK.
            pause_game();
            // Acquire the lock just to be safe.
            std::lock_guard lk(mDataMutex);
            mBoard = std::move(b);
            mAnnotation = std::move(anno);
        } catch (const json::exception& ex) {
            throw ReversiError("Error parsing JSON: "s + ex.what());
        }
    }

    void GameMan::from_json(const nlohmann::json& js) {
        using namespace std::string_literals;
        std::unique_ptr<Engine> new_black, new_white;
        try {
            new_black = make_engine_from_description(js.at("black"), mMainWindow);
            new_white = make_engine_from_description(js.at("white"), mMainWindow);
        } catch (const nlohmann::json::exception& ex) {
            throw ReversiError("Error parsing JSON: "s + ex.what());
        }
        read_annotation(js);
        // The game is now paused and the board and the annotation have been updated.
        load_black_engine(std::move(new_black));
        load_white_engine(std::move(new_white));
        // Let's ignore the possibility of these functions throwing.
        mMainWindow.update_board(mBoard, {0, 0});
        mWhiteSide->change_position(mBoard);
        mBlackSide->change_position(mBoard);
        resume_game();
    }
}