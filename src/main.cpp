#include "reversi_widgets.h"
#include "engi.h"
#include "mctse.h"
#include <nana/gui/widgets/menubar.hpp>
#include <nana/gui.hpp>

#if __has_include(<windows.h>)
// We're on windows
#include <windows.h>
#endif

int main() {
    using namespace Reversi;
    #if __has_include(<windows.h>)
    ::ShowWindow(::GetConsoleWindow(), SW_HIDE);
    #endif
    MainWindow mw("board.bmp");
    mw.newgame_dialog(true);
    mw.show();
    nana::exec();
}