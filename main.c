#include <Types.h>

#include "ui_app.h"
#include "network.h"
#include "speech.h"

int main(void)
{
    network_init();
    speech_init();

    ui_app_init();

    while (ui_app_pump_events())
    {
        speech_pump();
    }

    speech_shutdown();
    network_shutdown();

    return 0;
}
