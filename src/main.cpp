#include "reversi_widgets.h"
#include "engi.h"

int main() {
    using namespace Reversi;
    nana::form fm;
    fm.caption("Reversi");

    GameMan gm;
    BoardWidget board_widget(fm, gm, "board.bmp");
    SkipButton skip_button(fm, gm);
    RandomChoice engine(gm);
    engine.start_new(Player::Black);
    board_widget.start_new(Player::White);
    skip_button.start_new(Player::White);

    // Placer magic
    nana::place plc(fm);
    plc.div("<><vert weight=800 <><board weight=800><<><skip><>>><>");
    plc["board"] << board_widget;
    plc["skip"] << skip_button;
    plc.collocate();
    fm.show();
    nana::exec();
}