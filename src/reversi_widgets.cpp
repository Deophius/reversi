#include "reversi_widgets.h"
#include <algorithm>

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
        mCurrBoard = b;
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
        // mGraphics.paste(nana::API::root(*this), nana::rectangle{ pos(), mGraphics.size() }, 0, 0);
        mDrawing.update();
    }

    void BoardWidget::redraw() {
        update(mCurrBoard, mGraphCross.first, mGraphCross.second);
    }

    BoardWidget::BoardWidget(nana::window handle, const std::string& file_name, int sq_size) :
        nana::picture(handle), mSquareSize(sq_size), mGraphics({
            (unsigned)sq_size * 8,
            (unsigned)sq_size * 8
        }),
        mDrawing(*this)
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
        mDrawing.draw([this](nana::paint::graphics& dest_graphic) {
            mGraphics.paste(dest_graphic, 0, 0);
        });
        nana::paint::image board_image(file_name);
        board_image.stretch(
            nana::rectangle{ {0, 0}, board_image.size() },
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
                // The UIE isn't loaded or it's not waiting for a skip. Inform the user.
                (nana::msgbox("Not now") << "You can't skip now!")
                    .icon(nana::msgbox::icon_information)
                    .show();
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
        if (!mBoard.is_skip_legal()) {
            mSkipButton.enabled(false);
            // Hand off the promise to the board. Since the board might return
            // invalid data, we need to get a loop.
            // is_placable[i][j] is true if we can put a piece on (i, j)
            bool is_placable[9][9];
            for (int i = 1; i <= 8; i++) {
                for (int j = 1; j <= 8; j++)
                    is_placable[i][j] = mBoard.is_placable(i, j);
            }
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
                if (is_placable[result.first][result.second])
                    return result;
                // This move is not legal. We tell the board widget to keep on listening.
            }
        } else {
            if (mSkipButton.get_auto_skip())
                return {0, 0};
            mSkipButton.enabled(true);
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

    std::string UserInputEngine::get_name() {
        return "UserInput";
    }
}