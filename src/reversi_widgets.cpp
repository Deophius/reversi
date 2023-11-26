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

    void BoardWidget::draw_piece(nana::paint::graphics& graph, int x, int y, nana::color c) {
        // The center of the circle
        const auto [centerx, centery] = get_center(x, y);
        // Radius of circle
        const int rad = 0.3 * mSquareSize;
        const int rad2 = rad * rad;
        for (int ix = centerx - rad; ix <= centerx + rad; ix++) {
            for (int iy = centery - rad; iy <= centery + rad; iy++) {
                if ((ix - centerx) * (ix - centerx) + (iy - centery) * (iy - centery) <= rad2)
                    graph.set_pixel(ix, iy, c);
            }
        }
    }

    void BoardWidget::draw_cross(nana::paint::graphics& graph, int x, int y, nana::color c) {
        const auto [centerx, centery] = get_center(x, y);
        const int offset = 0.1 * mSquareSize;
        graph.line({ centerx - offset, centery }, { centerx + offset, centery }, c);
        graph.line({ centerx, centery - offset }, { centerx, centery + offset }, c);
    }
    
    void BoardWidget::update(int x, int y) {
        nana::paint::graphics gra({
            (unsigned)mSquareSize * 8,
            (unsigned)mSquareSize * 8
        });
        mBoardImage.stretch(
            nana::rectangle{ {0, 0}, mBoardImage.size() },
            gra,
            nana::rectangle{ {0, 0}, gra.size() }
        );
        for (int i = 1; i <= 8; i++) {
            for (int j = 1; j <= 8; j++) {
                switch (mGameMan.view_board()(i, j)) {
                    case Square::Black:
                        draw_piece(gra, i, j, nana::colors::black);
                        break;
                    case Square::White:
                        draw_piece(gra, i, j, nana::colors::white);
                        break;
                }
            }
        }
        if (x && y)
            draw_cross(gra, x, y, nana::colors::green);
        gra.save_as_file("tmp.bmp");
        // Calls super()'s method.
        this->load(nana::paint::image("tmp.bmp"));
        // Loads the new placable list.
        mPlacable = mGameMan.view_board().get_placable();
    }

    BoardWidget::BoardWidget(nana::window handle, GameMan& gm, const std::string& file_name, int sq_size) :
        nana::picture(handle), mGameMan(gm), mSquareSize(sq_size), mBoardImage(file_name)
    {
        // Register our update.
        mGameMan.listen("boardwidget", [this](int x, int y){ this->update(x, y); });
        mPlacable.reserve(64);
        // Update once for initial position
        update(0, 0);
        // When the user clicks, we need to respond.
        // FIXME: Current logic is pvp.
        events().click([this](const nana::arg_click& arg) {
            const auto pos = to_board_coord(arg.mouse_args);
            if (std::find(mPlacable.cbegin(), mPlacable.cend(), pos) != mPlacable.cend())
                mGameMan.place(pos.first, pos.second);
        });
    }
}