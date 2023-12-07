#include "reversi_widgets.h"
#include <nana/gui/filebox.hpp>
#include <fstream>

namespace Reversi {
    MainWindow::MainWindow(const std::string& board_img) :
        mSkipButton(*this),
        mBoardWidget(*this, board_img),
        mGameMan(GameMan::create(*this)),
        mMenubar(*this),
        mPlacer(*this)
    {
        // Sets GUI related stuff
        caption("Reversi");
        mPlacer.div("<><vert weight=800 <><board weight=800><<><skip><>>><>");
        mPlacer["board"] << mBoardWidget;
        mPlacer["skip"] << mSkipButton;
        mPlacer.collocate();
        show();
        // All our functionality will go in the menubar.
        mMenubar.push_back("&File");
        mMenubar.at(0).append("New game", [this](nana::menu::item_proxy&) {
            menu_start_new_game();
        });
        mMenubar.at(0).append("Save game", [this](nana::menu::item_proxy&) {
            save_game();
        });
        mMenubar.at(0).append("Load game", [this](nana::menu::item_proxy&) {
            menu_load_game();
        });
        mMenubar.at(0).append("Quit", [](nana::menu::item_proxy&){
            nana::API::exit_all();
        });
        mMenubar.push_back("&Game");
        mMenubar.at(1).append("Automatic skips", [this](nana::menu::item_proxy& ip) {
            menu_toggle_auto_skip(ip);
        });
        mMenubar.at(1).check_style(0, nana::menu::checks::highlight);
        mMenubar.at(1).checked(0, true);
        // When the window closes, asks the user to save.
        events().unload([this](const nana::arg_unload& arg) {
            if (ask_for_save())
                save_game();
        });
    }

    void MainWindow::announce_game_result(MatchResult res) {
        switch (res) {
        case MatchResult::Draw:
            nana::msgbox(*this, "Game ended in draw").show();
            break;
        case MatchResult::White:
            nana::msgbox(*this, "White wins").show();
            break;
        case MatchResult::Black:
            nana::msgbox(*this, "Black wins").show();
            break;
        }
    }

    void MainWindow::menu_start_new_game() {
        if (ask_for_save())
            save_game();
        auto picked = (nana::msgbox(*this, "New game", nana::msgbox::yes_no_cancel)
            << "Yes for black, no for white")
            .icon(nana::msgbox::icon_question)
            .show();
        if (picked == nana::msgbox::pick_cancel)
            return;
        // Otherwise a new game is indeed started.
        mGameMan->pause_game();
        if (picked == nana::msgbox::pick_yes) {
            // GUI plays black
            mGameMan->load_black_engine(
                std::make_unique<UserInputEngine>(mBoardWidget, mSkipButton)
            );
            mGameMan->load_white_engine(std::make_unique<RandomChoice>());
        } else {
            mGameMan->load_white_engine(
                std::make_unique<UserInputEngine>(mBoardWidget, mSkipButton)
            );
            mGameMan->load_black_engine(std::make_unique<RandomChoice>());
        }
        mGameMan->start_new();
    }

    void MainWindow::menu_load_game() {
        if (ask_for_save())
            save_game();
        auto paths = nana::filebox(*this, true)
            .add_filter("Reversi game (*.json)", "*.json")
            .add_filter("All files (*.*)", "*.*")
            .show();
        if (paths.empty())
            // user cancelled.
            return;
        nlohmann::json js;
        try {
            std::ifstream fin(paths[0]);
            fin >> js;
        } catch (const std::exception& ex) {
            (nana::msgbox(*this, "Error reading file") << ex.what())
                .icon(nana::msgbox::icon_error)
                .show();
            return;
        }
        try {
            mGameMan->from_json(js);
        } catch (const ReversiError& ex) {
            (nana::msgbox(*this, "Error parsing file content") << ex.what())
                .icon(nana::msgbox::icon_error)
                .show();
            return;
        }
    }

    void MainWindow::menu_toggle_auto_skip(nana::menu::item_proxy& ip) {
        mSkipButton.set_auto_skip(ip.checked());
    }

    void MainWindow::update_board(const Board& b, std::pair<int, int> last_move) {
        mBoardWidget.update(b, last_move.first, last_move.second);
    }

    bool MainWindow::ask_for_save() {
        if (!mGameMan->is_dirty())
            return false;
        return (nana::msgbox(*this, "Save file?", nana::msgbox::yes_no)
            << "Your game has changed, do you want to save it?")
            .icon(nana::msgbox::icon_question)
            .show() == nana::msgbox::pick_yes;
    }

    // Gets the default file name when saving.
    static inline std::string get_def_name() {
        using std::chrono::system_clock;
        using namespace std::literals;
        std::time_t now_time = system_clock::to_time_t(system_clock::now());
        std::tm* now_tm = std::localtime(&now_time);
        char buf[16];
        if (strftime(buf, sizeof(buf), "%m-%d-%H%M%S", now_tm) == 0) {
            nana::msgbox("Buffer not large enough in get_def_name()").show();
            std::terminate();
        }
        return "game"s + buf + ".json";
    }

    void MainWindow::save_game() {
        // Unconditionally save
        auto paths = nana::filebox(*this, false)
            .add_filter("Reversi game (*.json)", "*.json")
            .add_filter("All files (*.*)", "*.*")
            .init_file(get_def_name())
            .show();
        if (paths.empty())
            return;
        std::ofstream fout(paths.front());
        fout << mGameMan->to_json();
    }
}