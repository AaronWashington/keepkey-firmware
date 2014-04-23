#include <cassert>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>

#include <app.h>
#include <crypto.h>
#include <core.h>
#include <platform.h>

#include "bitcoin.h"
#include "display_manager.h"
#include "keepkey_manager.h"
#include "wallet.h"

static const std::string wallet_outfilename("keepkey_wallet.dat");

int main(int argc, char *argv[]) {

    cd::App app;
    AbortIfNot(app.init("KeepKey"), false, "Failed to init KeepKey app.\n");

    cd::KeepkeyManager kkmgr;
    AbortIfNot(app.register_runnable(&kkmgr), false,
            "Failed to register %s\n", kkmgr.get_name().c_str());

    cd::DisplayManager dmgr;
    AbortIfNot(app.register_runnable(&dmgr), false,
            "Failed to register %s\n", dmgr.get_name().c_str());

    app.run_forever();

    Assert("Don't get here.\n");


    return 0;
}
