#include "reversi_widgets.h"
#include "engi.h"
#include <nana/gui/widgets/menubar.hpp>
#include <nana/gui.hpp>

int main() {
    using namespace Reversi;
    MainWindow mw("board.bmp");
    RandomChoice rc1, rc2;
    mw.mGameMan.load_black_engine(&rc1);
    mw.mGameMan.load_white_engine(&rc2);
    mw.mGameMan.start_new();
    nana::exec();
}