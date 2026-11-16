#include <Types.h>

#include "ui/ui_app.h"
#include "network.h"
#include "speech.h"

int main(void)
{
    ui_app_init();

    while (ui_app_pump_events())
    {
        /* The UI event loop polls networking and our speech channel. */
    }

    (void)speech_shutdown();
    network_shutdown();

    return 0;
}
