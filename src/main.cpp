#include "reversi_widgets.h"
#include "engi.h"
#include <nana/gui/widgets/menubar.hpp>
#include <nana/gui.hpp>

int main() {
    using namespace Reversi;
    MainWindow mw("board.bmp");
    mw.load_engine(std::make_unique<RandomChoice>(mw.mGameMan));
    mw.start_new(Player::White);
    nana::exec();
}