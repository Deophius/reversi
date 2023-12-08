#ifndef REVERSI_BOARD_WIDGET_H
#define REVERSI_BOARD_WIDGET_H
#include <nana/gui.hpp>
#include <nana/gui/widgets/picture.hpp>
#include <nana/gui/widgets/button.hpp>
#include <nana/gui/widgets/menubar.hpp>
#include "game.h"
#include "engi.h"
#include <future>

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

        // Used to push data to the user input engine. Reuses are detected by
        // the exception thrown by set_value.
        std::promise<std::pair<int, int>> mUIProm;

        // As the name implies, the mutex protects only mUIProm and synchronizes
        // the engine thread and the GUI thread.
        std::mutex mPromMutex;

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

        // Sends in a promise for the click.
        void listen_click(std::promise<std::pair<int, int>> prom);
    };

    // The skip button.
    class SkipButton : public nana::button {
        // True if the skips will be performed automatically
        std::atomic_bool mAutoSkip = true;

        // To communicate with the user input engine
        std::promise<void> mUIProm;

        // Protects mUIProm
        std::mutex mPromMutex;
    public:
        SkipButton(nana::window handle);

        // Sets the auto skip status to `flag`
        inline void set_auto_skip(bool flag) noexcept {
            mAutoSkip.store(flag, std::memory_order_release);
        }

        // Queries the autoskip.
        inline bool get_auto_skip() const noexcept {
            return mAutoSkip.load(std::memory_order_acquire);
        }

        // Sets the promise.
        void listen_click(std::promise<void> prom);
    };

    // The user input engine that combines the skip button and the board.
    class UserInputEngine : public Engine {
        // Links to the two widgets.
        BoardWidget& mBoardWidget;
        SkipButton& mSkipButton;

        virtual std::pair<int, int> do_make_move() override;

    public:
        UserInputEngine(BoardWidget& bw, SkipButton& skb);

        virtual ~UserInputEngine() noexcept = default;

        virtual std::string get_name() override;
    };

    // The main window of our application
    struct MainWindow : public nana::form {
        // The components of the application
        SkipButton mSkipButton;
        BoardWidget mBoardWidget;
        // Important: Let the game manager be destroyed first, since its mainloop
        // potentially accesses the two control widgets.
        std::shared_ptr<GameMan> mGameMan;
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

        // Creates a popup window asking the user to select two sides of the
        // new game. The results are stored in the two out params.
        void new_game_pick_engine(std::string& black_name, std::string& white_name);

        // Starts a new game. For use in the menu.
        void menu_start_new_game();

        // Loads a game chosen in the filebox. For use in the menu.
        void menu_load_game();

        // Sets the automatic skip functionality. For use in the menu
        void menu_toggle_auto_skip(nana::menu::item_proxy& ip);

        // Updates the GUI according to the board b. The last move was
        // given to draw the cross.
        void update_board(const Board& b, std::pair<int, int> last_move);

        // If the associated game manager's annotation is dirty, asks the user
        // whether to save it.
        // Returns true if the user decides to save the game.
        bool ask_for_save();

        // Saves the game in a file the user selects.
        void save_game();
    };
}

#endif