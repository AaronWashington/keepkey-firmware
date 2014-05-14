#include <cassert>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <malloc.h>
#include <memory>
#include <string>
#include <sstream>
#include <vector>

#include <app.h>
#include <crypto.h>
#include <core.h>
#include <platform.h>

#include <board_factory.h>
#include <display_manager.h>
#include <keepkey_manager.h>

<<<<<<< HEAD
=======
#include "EvalKeepKeyBoard.h"


void
test_board(
        void
)
{
    EvalKeepKeyBoard* board = new EvalKeepKeyBoard();

    while(1)
    {}
}
>>>>>>> display_dev


int 
main(
        int argc, 
        char *argv[]
) 
{
<<<<<<< HEAD
    std::shared_ptr<KeepKeyBoard> board = cd::make_keepkey_board();
=======
    // Test the board.  This will block indefinitely.
    test_board();
>>>>>>> display_dev

    cd::App app;
    AbortIfNot(app.init("KeepKey"), false, "Failed to init KeepKey app.\n");

    cd::KeepkeyManager kkmgr;
    kkmgr.init();
    AbortIfNot(app.register_runnable(&kkmgr), false,
            "Failed to register %s\n", kkmgr.get_name().c_str());

    cd::DisplayManager dmgr;
    AbortIfNot(app.register_runnable(&dmgr), false,
            "Failed to register %s\n", dmgr.get_name().c_str());

    struct mallinfo mi = mallinfo();

    app.run_forever();

    Assert("Don't get here.\n");


    return 0;
}
