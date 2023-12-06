// Monte-Carlo tree search engine
#ifndef REVERSI_MTCSE_H
#define REVERSI_MTCSE_H
#include "engi.h"
#include <unordered_map>

namespace Reversi {
    class MTCS : public Engine {
        // The random generator used by rollout.
        static std::function<int()> mRandGen;

        static constexpr int mRolloutCnt = 10;

        struct Node {
            // v is the value for black
            long long v = 0, n = 0;
            bool is_leaf = true;
        };

        // The list of nodes. This can be retained across searches.
        std::unordered_map<Board, Node> mNodes;

        // Purely random rollout of the position b. Returns the results.
        static int rollout(Board b);

        // Adds all the possible next moves to the mNodes dictionary, then
        // clears the "is_leaf" flag for b.
        // Requires that b is in mNodes.
        void add_next(const Board& b);

        // Assuming that the node pointed to by `b` is not a leaf node,
        // Selects a child node to investigate.
        Board select_child(const Board& b);

        virtual std::pair<int, int> do_make_move() override;

    public:
        MTCS() = default;

        virtual ~MTCS() noexcept = default;

        virtual inline std::string get_name() override {
            return "MTCSe";
        }
    };
}

#endif