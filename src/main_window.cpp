#include "reversi_widgets.h"
#include <nana/gui/filebox.hpp>
#include <nana/gui/widgets/checkbox.hpp>
#include <nana/gui/widgets/label.hpp>
#include <iostream>
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

    // Fills in rg and ckbox for one side. For use only in the following function.
    static void populate_checkbox_helper(
        nana::form& fm, nana::radio_group& rg,
        std::vector<std::unique_ptr<nana::checkbox>>& ckbox
    ) {
        ckbox.emplace_back(new nana::checkbox(fm.handle(), "UserInput"));
        ckbox.emplace_back(new nana::checkbox(fm.handle(), "MCTSe"));
        ckbox.emplace_back(new nana::checkbox(fm.handle(), "RandomChoice"));
        for (auto& ptr : ckbox)
            rg.add(*ptr);
        // Defaults to user input and avoids a bunch of npos issues.
        ckbox.front()->check(true);
    }

    // Launches the modal window and outputs the user's selection into
    // black_name and white_name.
    void MainWindow::newgame_dialog() {
        std::string black_name, white_name;
        // The dialog box
        nana::form diag(*this, { 700, 200 });
        diag.caption("New game");
        // The black and white sides' checkboxes and radio groups.
        nana::radio_group rg_black, rg_white;
        std::vector<std::unique_ptr<nana::checkbox>> ckbox_black, ckbox_white;
        populate_checkbox_helper(diag, rg_black, ckbox_black);
        populate_checkbox_helper(diag, rg_white, ckbox_white);
        // Labels describing the options
        nana::label lbb(diag, "Black:"), lbw(diag, "White:");
        // The confirmation button
        nana::button butt_ok(diag), butt_cancel(diag);
        butt_ok.caption("OK");
        butt_ok.events().click([&] {
            using nana::radio_group;
            mGameMan->load_black_engine(
                make_engine_from_description(
                    ckbox_black.at(rg_black.checked())->caption(),
                    *this
                )
            );
            mGameMan->load_white_engine(
                make_engine_from_description(
                    ckbox_white.at(rg_white.checked())->caption(),
                    *this
                )
            );
            mGameMan->start_new();
            diag.close();
        });
        butt_cancel.caption("Cancel");
        butt_cancel.events().click([&] {
            mGameMan->resume_game();
            diag.close();
        });
        diag.events().unload([&]{
            mGameMan->resume_game();
        });
        // The placer for the dialog box
        nana::place plc(diag);
        plc.div("<weight=15%><vert <rgb><rgw><<><butt gap=50 arrange=[75,75]><>><weight=15%>");
        plc["rgb"] << lbb;
        for (auto& c : ckbox_black)
            plc["rgb"] << *c;
        plc["rgw"] << lbw;
        for (auto& c : ckbox_white)
            plc["rgw"] << *c;
        plc["butt"] << butt_ok << butt_cancel;
        plc.collocate();
        diag.show();
        // Experiment shows that after the call to diag.show(), the internal lock
        // of nana is released.
        mGameMan->pause_game();
        // Modal dialog box
        nana::API::modal_window(diag);
    }

    void MainWindow::menu_start_new_game() {
        std::cerr << "menu_start_new_game\n";
        if (ask_for_save())
            save_game();
        // important: The nana library includes a built-in mutex for every widget,
        // so we can't call pause_game() here, otherwise the GUI thread and the game
        // manager thread might deadlock.
        // But that mutex ensures that the game will not continue if we don't close the
        // dialog.
        newgame_dialog();
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