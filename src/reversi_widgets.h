#ifndef REVERSI_BOARD_WIDGET_H
#define REVERSI_BOARD_WIDGET_H
#include <nana/gui.hpp>
#include <nana/gui/widgets/picture.hpp>
#include <nana/gui/widgets/button.hpp>
#include <nana/gui/widgets/menubar.hpp>
#include "game.h"
#include "engi.h"

namespace Reversi {
    // This widget is responsible for showing the board info.
    class BoardWidget : public nana::picture {
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

    public:
        // Constructs the board widget with the handle and associated game manager.
        // The board is loaded from a bitmap file named by `file_name`.
        // The size of each square in pixels is sq_size.
        BoardWidget(nana::window handle, const std::string& file_name, int sq_size = 100);

        virtual ~BoardWidget() noexcept = default;

        // Updates the GUI widget according to data passed in
        // The last move was (x, y)
        void update(const Board& b, int x, int y);
    };

    // The skip button.
    class SkipButton : public nana::button {
        // True if the skips will be performed automatically
        bool mAutoSkip = true;
    public:
        SkipButton(nana::window handle);

        // Sets the auto skip status to `flag`
        inline void set_auto_skip(bool flag) {
            mAutoSkip = flag;
        }
    };

    // The main window of our application
    struct MainWindow : public nana::form {
        // The components of the application
        GameMan mGameMan;
        SkipButton mSkipButton;
        BoardWidget mBoardWidget;
        // Menubar
        nana::menubar mMenubar;
        // Placer magic
        nana::place mPlacer;
        // The color the engine plays
        Player mEngineColor;

        // Constructs the main window, with board img `board_img`.
        MainWindow(const std::string& board_img);

        // Broadcasts the result.
        void announce_game_result(MatchResult res);

        // Starts a new game. For use in the menu.
        void menu_start_new_game();

        // Sets the automatic skip functionality. For use in the menu
        void menu_toggle_auto_skip(nana::menu::item_proxy& ip);

        // Updates the GUI according to the board b. The last move was
        // given to draw the cross.
        void update_board(const Board& b, std::pair<int, int> last_move);
    };
}

#endif