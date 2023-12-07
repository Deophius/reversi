#include "mctse.h"
#include <random>
#include <cassert>
#include <cmath>
#include <unordered_set>
#include <iostream>
#include <stack>

namespace Reversi {
    std::function<int()> MCTS::mRandGen = []{
        std::mt19937 mt;
        std::uniform_int_distribution dist(0, 256);
        return std::bind(dist, mt);
    }();

    // We remember that black win == 1
    int MCTS::rollout(Board b) {
        // The placable squares.
        std::vector<std::pair<int, int>> plc;
        plc.reserve(64);
        // If the last move was a skip
        bool prev_skip = false;
        while (true) {
            plc = b.get_placable();
            if (plc.size()) {
                const auto [x, y] = plc[mRandGen() % plc.size()];
                b.place(x, y);
                prev_skip = false;
            } else {
                if (prev_skip) {
                    switch (b.count()) {
                        case MatchResult::Black: return 1;
                        case MatchResult::White: return -1;
                        case MatchResult::Draw: return 0;
                    }
                }
                b.skip();
                prev_skip = true;
            }
        }
    }

    void MCTS::add_next(const Board& b) {
        // If the engine is reused, it's possible that the the extension gets a
        // position that has been evaluated before. So we use insert which doesn't
        // override existing items.
        assert(mNodes.contains(b));
        if (!mNodes[b].is_leaf)
            // There has been a round of expansion.
            return;
        const auto plc = b.get_placable();
        mNodes[b].is_leaf = false;
        if (plc.empty()) {
            Board b2 = b;
            b2.skip();
            // Since Board is now trivially copyable, there's no point moving.
            mNodes.emplace(b2, Node());
            return;
        }
        // There are valid moves besides the skip
        for (const auto& [x, y] : plc) {
            Board b2 = b;
            b2.place(x, y);
            mNodes.emplace(b2, Node());
        }
    }

    Board MCTS::select_child(const Board& b) {
        // Adjust this constant for explore/exploit ratio
        static constexpr double c = 0.5;
        assert(mNodes.contains(b) && !mNodes[b].is_leaf);
        assert(mNodes[b].n);
        const auto plc = b.get_placable();
        if (plc.empty()) {
            // Obviously there is only one choice
            Board b2 = b;
            b2.skip();
            return b2;
        }
        const double log_parent = std::log(mNodes[b].n);
        Board ans = b;
        // The current best result.
        double best = -std::numeric_limits<double>::infinity();
        // FIXME: I'm not sure if moving the if inside the for loop will cause the program
        // to run much slower.
        if (b.whos_next() == Player::Black) {
            for (const auto& [x, y] : plc) {
                Board b2 = b;
                b2.place(x, y);
                const Node& node = mNodes[b2];
                // Unexplored nodes are the most important
                if (node.n == 0)
                    return b2;
                const double curr = double(node.v) / node.n + c * std::sqrt(log_parent / node.n);
                if (curr > best) {
                    best = curr;
                    ans = std::move(b2);
                }
            }
        } else {
            for (const auto& [x, y] : plc) {
                Board b2 = b;
                b2.place(x, y);
                const Node& node = mNodes[b2];
                // Unexplored nodes are the most important
                if (node.n == 0)
                    return b2;
                const double curr = 1.0 - double(node.v) / node.n + c * std::sqrt(log_parent / node.n);
                if (curr > best) {
                    best = curr;
                    ans = std::move(b2);
                }
            }
        }
        return ans;
    }

    std::pair<int, int> MCTS::do_make_move() {
        using namespace std::chrono;
        time_point tp_end = steady_clock::now() + seconds(1);
        const auto legal_moves = mBoard.get_placable();
        if (legal_moves.empty())
            return {0, 0};
        // First we add the initial position to the table. It's the root node
        // of the MCTS tree.
        // If the instance has explored this position in previous games, we just
        // take that and build our computation on top of it.
        mNodes.emplace(mBoard, Node());
        // The stack for backtracing
        std::stack<Board> st;
        // To avoid the situation mentioned above. Without the vis the 
        std::unordered_set<Board> vis;
        unsigned cnt = 0;
        while (steady_clock::now() < tp_end && !mCancel.load(std::memory_order_acquire)) {
            ++cnt;
            vis.clear();
            Board curr = mBoard;
            int step_cnt = 0;
            while (!mNodes[curr].is_leaf && !vis.contains(curr)) {
                st.push(curr);
                vis.insert(curr);
                curr = select_child(curr);
                ++step_cnt;
                assert(step_cnt <= 128);
            }
            // The stack for backtracking includes the leaf node, too.
            st.push(curr);
            add_next(curr);
            int rollout_result = 0;
            for (int i = 0; i < mRolloutCnt; i++)
                rollout_result += rollout(curr);
            while (!st.empty()) {
                Node& node = mNodes[st.top()];
                node.n += mRolloutCnt;
                node.v += rollout_result;
                st.pop();
            }
        }
        std::cerr << cnt << " cycles done, " << mNodes.size() << " nodes in table\n";
        if (mCancel.load(std::memory_order_acquire))
            throw OperationCanceled();
        std::pair<int, int> ans;
        int max_visits = -1;
        for (const auto& [x, y] : legal_moves) {
            Board b2 = mBoard;
            b2.place(x, y);
            if (const int curr_visits = mNodes[b2].n; curr_visits > max_visits) {
                max_visits = curr_visits;
                ans = { x, y };
            }
        }
        return ans;
    }
}