// Interface for the engine
#ifndef REVERSI_ENGI_H
#define REVERSI_ENGI_H
#include "game.h"
#include <random>
#include <chrono>
#include <functional>

namespace Reversi {
    // An interface for the engine.
    class Engine {
    public:
        // Virtual destructors
        virtual ~Engine() noexcept = default;

        // Starts a new game, with the engine playing `color` side.
        virtual void start_new(Player color) = 0;

        // Enter opponent's move. Since the engine should be wrapped inside
        // some interface, the input should be legal.
        // (0, 0) means a skip.
        virtual void enter_move(int x, int y) = 0;

        // Computes a move and returns it. (0, 0) means a skip.
        virtual std::pair<int, int> make_move() = 0;
    };

    // For testing purposes. This engine randomly selects an available move.
    class RandomChoice : public Engine {
        Board board;
        Player color;
        std::function<int()> rng;
    public:
        RandomChoice() noexcept {
            std::mt19937 mt(std::chrono::system_clock::now().time_since_epoch().count());
            std::uniform_int_distribution dist(0, 256);
            rng = std::bind(dist, mt);
        }

        virtual ~RandomChoice() noexcept = default;

        virtual void start_new(Player c) override {
            board = Board();
            color = c;
        }

        virtual void enter_move(int x, int y) override {
            if (x == 0 && y == 0)
                board.skip();
            else
                board.place(x, y);
        }

        virtual std::pair<int, int> make_move() override {
            auto avail = board.get_placable();
            if (avail.size())
                return avail[rng() % avail.size()];
            else
                return { 0, 0 };
        }
    };
}

#endif