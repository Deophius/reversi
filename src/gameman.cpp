#include "game.h"
#include <doctest.h>
#include <istream>
#include <ostream>
#include <sstream>

namespace Reversi {
    std::ostream& operator<< (std::ostream& ostr, const GameMan& game) {
        for (auto [x, y] : game.mAnnotation)
            ostr << char('a' - 1 + x) << y << ' ';
        // Deliminator
        ostr << "#\n";
        if (ostr.good())
            game.mDirtyFile = false;
        return ostr;
    }

    std::istream& operator>> (std::istream& istr, GameMan& game) {
        GameMan tmp;
        std::string s;
        while (istr >> s) {
            if (s == "#")
                break;
            if (s.length() != 2) {
                istr.setstate(std::ios::failbit);
                return istr;
            }
            const int x = s[0] - 'a' + 1, y = s[1] - '0';
            // The input might be evil and contains extra steps after game termination.
            // So we need a try-catch block to translate all exceptions into ReversiError.
            try {
                tmp.place(x, y);
            } catch (std::logic_error const&) {
                throw ReversiError("Extra steps after end of game!");
            } catch (const ReversiError&) {
                throw;
            }
        }
        std::swap(game, tmp);
        // The game is saved there.
        game.mDirtyFile = false;
        return istr;
    }

    void GameMan::place(int x, int y) {
        if (get_result() != MatchResult::InProgress)
            throw std::logic_error("Operations after end of game");
        if (mBoard.is_placable(x, y)) {
            mBoard.place(x, y);
            mPrevSkip = false;
            mDirtyFile = true;
            for (auto&& cb : mListeners)
                cb.second(x, y);
        }
        else
            throw ReversiError("Illegal move");
    }

    void GameMan::skip() {
        if (get_result() != MatchResult::InProgress)
            throw std::logic_error("Operations after end of game");
        if (mBoard.get_placable().size())
            throw ReversiError("Illegal skip");
        mBoard.skip();
        mDirtyFile = true;
        // If this is the second skip in a row, count and sets the game result.
        if (mPrevSkip) {
            mResult = mBoard.count();
        }
        mPrevSkip = true;
        for (auto&& cb : mListeners)
            cb.second(0, 0);
    }

    void GameMan::place_skip(std::pair<int, int> pos) {
        if (pos.first)
            place(pos.first, pos.second);
        else
            skip();
    }

    bool GameMan::listen(std::string name, std::function<void(int, int)> cb) {
        return mListeners.insert({ std::move(name), std::move(cb) }).second;
    }

    bool GameMan::unhook(const std::string& name) {
        return mListeners.erase(name);
    }

    TEST_CASE("serialization") {
        GameMan game;
        CHECK(!game.is_dirty());
        game.place(4, 6);
        game.place(3, 6);
        game.place(3, 5);
        game.place(5, 6);
        CHECK(game.is_dirty());
        std::ostringstream ostr;
        ostr << game;
        CHECK(!game.is_dirty());
        CHECK(ostr);
        CHECK(ostr.str() == "d6 c6 c5 e6 #\n");
        std::istringstream istr(ostr.str());
        game.reset();
        CHECK(!game.is_dirty());
        istr >> game;
        CHECK(!game.is_dirty());
        CHECK(istr);
        CHECK(game.view_board()(5, 5) == Square::White);
    }

    TEST_CASE("exceptions") {
        GameMan game;
        CHECK_THROWS_AS(game.skip(), ReversiError);
        CHECK(game.view_board().whos_next() == Player::Black);
        CHECK_THROWS_AS(game.place(1, 1), ReversiError);
        CHECK(game.view_board().whos_next() == Player::Black);
        CHECK_THROWS_AS(game.place(9, 9), ReversiError);
        CHECK(game.view_board().whos_next() == Player::Black);
        game.place(4, 6);
        CHECK(game.view_board().whos_next() == Player::White);
        std::ostringstream ostr;
        ostr << game;
        CHECK(ostr.str() == "d6 #\n");
        // Let's check that even after two skips, the game is still in progress
        CHECK_THROWS(game.skip());
        CHECK_THROWS(game.skip());
        CHECK(game.get_result() == MatchResult::InProgress);
    }

    TEST_CASE("listener add/remove") {
        GameMan a;
        CHECK(a.listen("other", [](int, int){}));
        CHECK(!a.listen("annotator", [](int, int){}));
        CHECK(!a.unhook("nonexistent"));
        CHECK(a.unhook("other"));
        REQUIRE(a.unhook("annotator"));
        GameMan b;
        b.reset();
        CHECK(b.unhook("annotator"));
    }
}