#include "reversi_widgets.h"
#include <algorithm>
#include <fstream>
#include <nana/gui/filebox.hpp>

namespace Reversi {
    std::pair<int, int> BoardWidget::to_board_coord(const nana::arg_mouse* arg) {
        return {
            arg->pos.x / mSquareSize + 1,
            8 - arg->pos.y / 100
        };
    }

    std::pair<int, int> BoardWidget::get_center(int x, int y) {
        return {
            -mSquareSize / 2 + x * mSquareSize,
            Board::MAX_FILES * mSquareSize + mSquareSize / 2 - y * mSquareSize
        };
    }

    void BoardWidget::draw_piece(int x, int y, nana::color c) {
        // The center of the circle
        const auto [centerx, centery] = get_center(x, y);
        // Radius of circle
        const int rad = 3 * mSquareSize / 10;
        const int rad2 = rad * rad;
        for (int ix = centerx - rad; ix <= centerx + rad; ix++) {
            for (int iy = centery - rad; iy <= centery + rad; iy++) {
                if ((ix - centerx) * (ix - centerx) + (iy - centery) * (iy - centery) <= rad2)
                    mGraphics.set_pixel(ix, iy, c);
            }
        }
    }

    void BoardWidget::draw_cross(int x, int y, nana::color c) {
        const auto [centerx, centery] = get_center(x, y);
        const int offset = mSquareSize / 10;
        mGraphics.line({ centerx - offset, centery }, { centerx + offset, centery }, c);
        mGraphics.line({ centerx, centery - offset }, { centerx, centery + offset }, c);
    }
    
    void BoardWidget::update(int x, int y) {
        // First we remove the old cross. There must be some piece on that square.
        if (mGraphCross.first) {
            auto [i, j] = mGraphCross;
            draw_cross(i, j, mGameMan.view_board().at(i, j) == Square::Black ?
                nana::colors::black : nana::colors::white
            );
        }
        for (int i = 1; i <= 8; i++) {
            for (int j = 1; j <= 8; j++) {
                switch (mGameMan.view_board()(i, j)) {
                case Square::Black:
                    if (mGraphContent[i][j] != Square::Black) {
                        draw_piece(i, j, nana::colors::black);
                        mGraphContent[i][j] = Square::Black;
                    }
                    break;
                case Square::White:
                    if (mGraphContent[i][j] != Square::White) {
                        draw_piece(i, j, nana::colors::white);
                        mGraphContent[i][j] = Square::White;
                    }
                    break;
                case Square::Empty:
                    if (mGraphContent[i][j] != Square::Empty) {
                        draw_piece(i, j, (i + j) & 1 ?
                            // FIXME: Remove this magic "static coding"
                            nana::color(240, 217, 181) : nana::color(181, 136, 99)
                        );
                        mGraphContent[i][j] = Square::Empty;
                    }
                }
            }
        }
        // If this is a skip, we still need to set the last cross to (0, 0)
        mGraphCross = { x, y };
        if (x && y)
            draw_cross(x, y, nana::colors::green);
        mGraphics.save_as_file("tmp.bmp");
        // Calls super()'s method.
        this->load(nana::paint::image("tmp.bmp"));
    }

    void BoardWidget::redraw() {
        // Reset the graphic and its content records.
        for (auto& arr : mGraphContent)
            arr.fill(Square::Empty);
        mBoardImage.stretch(
            nana::rectangle{ {0, 0}, mBoardImage.size() },
            mGraphics,
            nana::rectangle{ {0, 0}, mGraphics.size() }
        );
        mGraphCross = { 0, 0 };
        // Now we can use update().
        update(0, 0);
    }

    BoardWidget::BoardWidget(nana::window handle, GameMan& gm, const std::string& file_name, int sq_size) :
        nana::picture(handle), mGameMan(gm), mSquareSize(sq_size), mBoardImage(file_name), mGraphics({
            (unsigned)sq_size * 8,
            (unsigned)sq_size * 8
        })
    {
        // Register our update.
        mGameMan.listen("boardwidget", [this](int x, int y){ this->update(x, y); });
        // When the user clicks, we need to respond.
        events().click([this](const nana::arg_click& arg) {
            if (mGameMan.view_board().whos_next() != mColor
                || mGameMan.get_result() != MatchResult::InProgress
            )
                return;
            const auto pos = to_board_coord(arg.mouse_args);
            decltype(auto) placable = mGameMan.view_board().get_placable();
            if (std::find(placable.cbegin(), placable.cend(), pos) != placable.cend())
                mGameMan.place(pos.first, pos.second);
        });
    }

    void BoardWidget::start_new(Player color) {
        mColor = color;
        redraw();
    }

    SkipButton::SkipButton(nana::window handle, GameMan& gm) :
        nana::button(handle), mGameMan(gm)
    {
        gm.listen("skipbutt", [this](int, int) {
            bool has_skip = mGameMan.view_board().whos_next() == mColor 
                && mGameMan.get_result() == MatchResult::InProgress
                && mGameMan.view_board().get_placable().size() == 0;
            enabled(has_skip);
            if (mAutoSkip && has_skip)
                mGameMan.skip();
        });
        events().click([this]{
            mGameMan.skip();
        });
        caption("Skip");
    }

    void SkipButton::start_new(Player c) {
        mColor = c;
        // Possibly, the new situation on board is one where we can skip
        // (if loaded from a file)
        enabled(mGameMan.view_board().whos_next() == mColor 
            && mGameMan.get_result() == MatchResult::InProgress
            && mGameMan.view_board().get_placable().size() == 0
        );
    }

    MainWindow::MainWindow(const std::string& board_img) :
        mSkipButton(*this, mGameMan),
        mBoardWidget(*this, mGameMan, board_img),
        mMenubar(*this),
        mEngine(nullptr),
        mPlacer(*this)
    {
        // Adds an event listener to display a congratulation message.
        mGameMan.listen("congrat_msgbox", [this](int, int){ check_game_result(0, 0); });
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
        mMenubar.at(0).append("Quit", [](nana::menu::item_proxy&){
            nana::API::exit_all();
        });
        mMenubar.push_back("&Game");
        mMenubar.at(1).append("Automatic skips", [this](nana::menu::item_proxy& ip) {
            menu_toggle_auto_skip(ip);
        });
        mMenubar.at(1).check_style(0, nana::menu::checks::highlight);
        mMenubar.at(1).checked(0, true);
    }

    void MainWindow::start_new(Player engine_color) {
        if (!mEngine) {
            (nana::msgbox(*this, "Error starting new game") << "No engine").show();
            return;
        }
        mEngineColor = engine_color;
        mGameMan.reset();
        mEngine->start_new(engine_color);
        mBoardWidget.start_new(Player(1 - (unsigned char)engine_color));
        mSkipButton.start_new(Player(1 - (unsigned char)engine_color));
    }

    void MainWindow::check_game_result(int, int) {
        switch (mGameMan.get_result()) {
        case MatchResult::Draw:
            nana::msgbox(*this, "Game ended in draw").show();
            break;
        case MatchResult::White:
        case MatchResult::Black:
            if ((mGameMan.get_result() == MatchResult::Black) xor (mEngineColor == Player::Black))
                (nana::msgbox(*this, "You win!") << "Congratulations!").show();
            else
                (nana::msgbox(*this, "You lost!") << "Better luck next time!").show();
            break;
        }
    }

    void MainWindow::menu_start_new_game() {
        using nana::msgbox;
        auto choice = (msgbox(*this, "New game", msgbox::yes_no_cancel)
            << "Yes for black, no for white, or cancel")
            .show();
        switch (choice) {
        case msgbox::pick_yes:
            start_new(Player::White);
            break;
        case msgbox::pick_no:
            start_new(Player::Black);
            break;
        }
    }

    void MainWindow::menu_toggle_auto_skip(nana::menu::item_proxy& ip) {
        mSkipButton.set_auto_skip(ip.checked());
    }

    bool MainWindow::save_game() {
        using namespace std::string_literals;
        const auto paths = nana::filebox(*this, false)
            .add_filter("Reversi annotation", "*.rvs")
            .show();
        if (paths.size()) {
            std::ofstream fout(paths.front());
            fout << mGameMan;
            if (!fout)
                (nana::msgbox(*this, "File IO error") << "An error occurred when saving the game!")
                    .icon(nana::msgbox::icon_error)
                    .show();
        }
        return mGameMan.is_dirty();
    }

    bool MainWindow::load_game() {
        const auto paths = nana::filebox(*this, true)
            .add_filter("Reversi annotation", "*.rvs")
            .show();
        if (paths.empty())
            return false;
        std::ifstream fin(paths.front());
        try {
            fin >> mGameMan;
        } catch (const ReversiError& ex) {
            (nana::msgbox(*this, "Error parsing annotations") << ex.what())
                .icon(nana::msgbox::icon_error)
                .show();
            return false;
        }
        if (!fin) {
            (nana::msgbox(*this, "Error reading file") << "Format error.")
                .icon(nana::msgbox::icon_error)
                .show();
            return false;
        }
        return true;
    }
}