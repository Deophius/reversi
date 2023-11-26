// Interface for the engine
#ifndef REVERSI_ENGI_H
#define REVERSI_ENGI_H
#include "game.h"
#include <random>
#include <chrono>
#include <functional>
#include <thread>

namespace Reversi {
    // An interface for the engine.
    class Engine {
    protected:
        // Reference to the game manager.
        GameMan& mGameMan;
    public:
        Engine(GameMan& gm) noexcept : mGameMan(gm) {}

        // Virtual destructors
        virtual ~Engine() noexcept = default;

        // Starts a new game, with the engine playing `color` side.
        // GameMan calls this after resetting its board.
        virtual void start_new(Player color) = 0;

        // Enter opponent's move. Since the engine should be wrapped inside
        // some interface, the input should be legal.
        // (0, 0) means a skip.
        // GameMan also calls this after its board is updated.
        virtual void enter_move(int x, int y) = 0;

        // Computes a move and plays it. (0, 0) means a skip.
        virtual void make_move() = 0;
    };

    // For testing purposes. This engine randomly selects an available move.
    class RandomChoice : public Engine {
        Board mBoard;
        Player mColor;
        std::function<int()> mRandomGen;
    public:
        RandomChoice(GameMan& gm) : Engine(gm) {
            std::mt19937 mt(std::chrono::system_clock::now().time_since_epoch().count());
            std::uniform_int_distribution dist(0, 256);
            mRandomGen = std::bind(dist, mt);
            mGameMan.listen("eng", [this](int x, int y) {
                if (x)
                    mBoard.place(x, y);
                else
                    mBoard.skip();
                if (mGameMan.view_board().whos_next() == mColor && mGameMan.get_result() == MatchResult::InProgress)
                    std::thread([this]{ make_move(); }).detach();
            });
        }

        virtual ~RandomChoice() noexcept = default;

        virtual void start_new(Player c) override {
            mBoard = Board();
            mColor = c;
            if (c == Player::Black)
                std::thread([this]{ make_move(); }).detach();
        }

        virtual void enter_move(int x, int y) override {
            if (x == 0 && y == 0)
                mBoard.skip();
            else
                mBoard.place(x, y);
        }

        virtual void make_move() override {
            // Emulate a slow computation
            std::this_thread::sleep_for(std::chrono::seconds(1));
            const auto& avail = mBoard.get_placable();
            if (avail.size()) {
                const auto [x, y] = avail[mRandomGen() % avail.size()];
                mGameMan.place(x, y);
            }
            else
                mGameMan.skip();
        }
    };
}

#endif