#ifndef REVERSI_BOARD_WIDGET_H
#define REVERSI_BOARD_WIDGET_H
#include <nana/gui.hpp>
#include <nana/gui/widgets/picture.hpp>
#include <nana/gui/widgets/button.hpp>
#include "game.h"

namespace Reversi {
    // This widget is responsible for showing the board info.
    class BoardWidget : public nana::picture {
        // The reference to the game manager
        GameMan& mGameMan;

        // The size of each square
        const int mSquareSize;

        // The bitmap image of the board
        nana::paint::image mBoardImage;

        // The graphics object where the drawing takes place.
        nana::paint::graphics mGraphics;

        // 9*9 array that records the previous state, as drawn in graphics.
        std::array<std::array<Square, 9>, 9> mGraphContent;

        // The position of the green cross in graphics.
        std::pair<int, int> mGraphCross;

        // The color the user plays
        Player mColor;

        // Helper function to convert the pixel language to board language.
        std::pair<int, int> to_board_coord(const nana::arg_mouse* arg);

        // Converts board repr to pixel representation.
        std::pair<int, int> get_center(int x, int y);

        // Draws a piece with color c at (x, y)
        void draw_piece(int x, int y, nana::color c);

        // Draws a cross at (x, y) with color c, as the "last move indicator"
        void draw_cross(int x, int y, nana::color c);

        // Updates the GUI widget according to data from mGameMan.
        // The last move was (x, y)
        void update(int x, int y);

        // Redraws the GUI widget. This should be used when a new game is started.
        // Doesn't draw the last move indicator.
        void redraw();
    public:
        // Constructs the board widget with the handle and associated game manager.
        // The board is loaded from a bitmap file named by `file_name`.
        // The size of each square in pixels is sq_size.
        BoardWidget(nana::window handle, GameMan& gm, const std::string& file_name, int sq_size = 100);

        virtual ~BoardWidget() noexcept = default;

        // Starts a new game and the user plays the side with color c.
        // Expects that the associated GameMan has been reset prior to this call.
        void start_new(Player color);
    };

    // The skip button.
    class SkipButton : public nana::button {
        // Ref to game manager
        GameMan& mGameMan;
        // Side played by user
        Player mColor;
    public:
        SkipButton(nana::window handle, GameMan& gm);

        // Starts a new game with this widget as a part of the side playing `c`.
        void start_new(Player c);
    };
}

#endif