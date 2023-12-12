#include "reversi_widgets.h"
#include "engi.h"
#include "mctse.h"
#include <nana/gui/widgets/menubar.hpp>
#include <nana/gui.hpp>

int main() {
    using namespace Reversi;
    MainWindow mw("board.bmp");
    mw.newgame_dialog(true);
    mw.show();
    nana::exec();
}