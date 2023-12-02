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

    static nana::color pick_color(const Board& b, int i, int j) {
        return b.at(i, j) == Square::Black ?
            nana::colors::black : b(i, j) == Square::White ?
            nana::colors::white : (i + j) & 1 ?
            nana::color(240, 217, 181) : nana::color(181, 136, 99);
    }
    
    void BoardWidget::update(const Board& b, int x, int y) {
        // First we remove the old cross.
        if (mGraphCross.first) {
            auto [i, j] = mGraphCross;
            draw_cross(i, j, pick_color(b, i, j));
        }
        for (int i = 1; i <= 8; i++) {
            for (int j = 1; j <= 8; j++) {
                if (b(i, j) != mGraphContent[i][j]) {
                    draw_piece(i, j, pick_color(b, i, j));
                    mGraphContent[i][j] = b(i, j);
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

    BoardWidget::BoardWidget(nana::window handle, const std::string& file_name, int sq_size) :
        nana::picture(handle), mSquareSize(sq_size), mBoardImage(file_name), mGraphics({
            (unsigned)sq_size * 8,
            (unsigned)sq_size * 8
        })
    {
        events().click([this](const nana::arg_click& arg) {
            try {
                std::lock_guard lk(mPromMutex);
                mUIProm.set_value(to_board_coord(arg.mouse_args));
            } catch (const std::future_error&) {
                // This means that there is no active request waiting for the click.
                // Just ignore it.
            }
        });
        mBoardImage.stretch(
            nana::rectangle{ {0, 0}, mBoardImage.size() },
            mGraphics,
            nana::rectangle{ {0, 0}, mGraphics.size() }
        );
        update(Board(), 0, 0);
    }

    void BoardWidget::listen_click(std::promise<std::pair<int, int>> prom) {
        std::lock_guard lk(mPromMutex);
        mUIProm = std::move(prom);
    }

    SkipButton::SkipButton(nana::window handle) : nana::button(handle) {
        caption("Skip");
        events().click([this]{
            try {
                std::lock_guard lk(mPromMutex);
                mUIProm.set_value();
            } catch (const std::future_error&) {
                // do nothing
            }
        });
    }

    void SkipButton::listen_click(std::promise<void> prom) {
        std::lock_guard lk(mPromMutex);
        mUIProm = std::move(prom);
    }

    UserInputEngine::UserInputEngine(BoardWidget& bw, SkipButton& skb) :
        mBoardWidget(bw), mSkipButton(skb)
    {}

    std::pair<int, int> UserInputEngine::do_make_move() {
        using namespace std::chrono_literals;
        // We can safely access mBoard since it's protected by the mutex
        if (mBoard.get_placable().size()) {
            // Hand off the promise to the board. Since the board might return
            // invalid data, we need to get a loop.
            while (true) {
                std::promise<std::pair<int, int>> prom;
                auto fut = prom.get_future();
                mBoardWidget.listen_click(std::move(prom));
                // Check for cancel requests every 100ms
                while (fut.wait_for(100ms) == std::future_status::timeout) {
                    if (mCancel.load(std::memory_order_acquire))
                        throw OperationCanceled();
                }
                // Now the future is "ready".
                // The manager is destroyed before the widgets, so the engine
                // is destroyed before the widgets, so this doesn't throw
                // abandoned_promise.
                auto result = fut.get();
                // Check if this is a legal move
                if (std::find(mBoard.get_placable().cbegin(), mBoard.get_placable().cend(),
                    result) != mBoard.get_placable().cend()
                ) {
                    return result;
                }
                // This move is not legal. We tell the board widget to keep on listening.
            }
        } else {
            if (mSkipButton.get_auto_skip())
                return {0, 0};
            // Hand off the promise to the skip button.
            std::promise<void> prom;
            auto fut = prom.get_future();
            mSkipButton.listen_click(std::move(prom));
            while (fut.wait_for(100ms) == std::future_status::timeout) {
                // Check for explicit cancel requests every 100ms
                if (mCancel.load(std::memory_order_acquire))
                    throw OperationCanceled();
            }
            // Now the future is ready, which means the user has clicked the button
            return { 0, 0 };
        }
    }

    MainWindow::MainWindow(const std::string& board_img) :
        mGameMan(GameMan::create(*this)),
        mSkipButton(*this),
        mBoardWidget(*this, board_img),
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
        nana::msgbox("Unimplemented!").show();
    }

    void MainWindow::menu_toggle_auto_skip(nana::menu::item_proxy& ip) {
        mSkipButton.set_auto_skip(ip.checked());
    }

    void MainWindow::update_board(const Board& b, std::pair<int, int> last_move) {
        mBoardWidget.update(b, last_move.first, last_move.second);
    }
}