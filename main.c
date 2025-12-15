#include <Types.h>

extern void ui_app_init(void);
extern Boolean ui_app_pump_events(void);

int main(void)
{
    ui_app_init();

    while (ui_app_pump_events())
    {
        /* Future: tcp_poll(); speech_pump(); */
    }

    return 0;
}
