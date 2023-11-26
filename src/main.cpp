#include "reversi_widgets.h"

int main() {
    using namespace Reversi;
    nana::form fm;
    fm.caption("Reversi");

    GameMan gm;
    BoardWidget board_widget(fm, gm, "board.bmp");
    
    // Placer magic
    nana::place plc(fm);
    plc.div("<><vert weight=800 <><board weight=800><>><>");
    plc["board"] << board_widget;
    plc.collocate();
    fm.show();
    nana::exec();
}