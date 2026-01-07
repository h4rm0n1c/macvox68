#include <Types.h>

#include "ui/ui_app.h"
#include "network.h"

int main(void)
{
    ui_app_init();

    while (ui_app_pump_events())
    {
        /* Future: tcp_poll(); speech_pump(); */
    }

    network_shutdown();

    return 0;
}
