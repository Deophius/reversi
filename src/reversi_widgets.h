#ifndef REVERSI_BOARD_WIDGET_H
#define REVERSI_BOARD_WIDGET_H
#include <nana/gui.hpp>
#include <nana/gui/widgets/picture.hpp>
#include "game.h"

namespace Reversi {
    // This widget is responsible for showing the board info.
    class BoardWidget : public nana::picture {
        // The reference to the game manager
        GameMan& mGameMan;

        // The size of each square
        const int mSquareSize;

        // The image of the board
        nana::paint::image mBoardImage;

        // The list of placable squares
        std::vector<std::pair<int, int>> mPlacable;

        // Helper function to convert the pixel language to board language.
        std::pair<int, int> to_board_coord(const nana::arg_mouse* arg);

        // Converts board repr to pixel representation.
        std::pair<int, int> get_center(int x, int y);

        // Draws a piece with color c at (x, y)
        void draw_piece(nana::paint::graphics& graph, int x, int y, nana::color c);

        // Draws a cross at (x, y) with color c, as the "last move indicator"
        void draw_cross(nana::paint::graphics& graph, int x, int y, nana::color c);

        // Updates the GUI widget according to data from mGameMan.
        // The last move was (x, y)
        void update(int x, int y);
    public:
        // Constructs the board widget with the handle and associated game manager.
        // The board is loaded from a bitmap file named by `file_name`.
        // The size of each square in pixels is sq_size.
        BoardWidget(nana::window handle, GameMan& gm, const std::string& file_name, int sq_size = 100);

        virtual ~BoardWidget() noexcept = default;
    };
}

#endif