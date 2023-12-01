#include "reversi_widgets.h"
#include "engi.h"
#include <nana/gui/widgets/menubar.hpp>
#include <nana/gui.hpp>

int main() {
    using namespace Reversi;
    MainWindow mw("board.bmp");
    mw.mGameMan->load_black_engine(std::make_unique<RandomChoice>());
    mw.mGameMan->load_white_engine(std::make_unique<RandomChoice>());
    mw.mGameMan->start_new();
    nana::exec();
}