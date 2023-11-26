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
        // FIXME: Current logic is pvp.
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
            enabled(mGameMan.view_board().whos_next() == mColor 
                && mGameMan.get_result() == MatchResult::InProgress
                && mGameMan.view_board().get_placable().size() == 0
            );
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
}