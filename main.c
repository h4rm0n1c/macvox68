#include <Types.h>

#include "ui_app.h"

int main(void)
{
    ui_app_init();

    while (ui_app_pump_events())
    {
        /* Future: tcp_poll(); speech_pump(); */
    }

    return 0;
}
