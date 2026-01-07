#include "network.h"

#include "network_backend.h"

#include <Gestalt.h>

static NetworkCallbacks        gCallbacks;
static const NetworkBackend   *gBackend = NULL;

static Boolean network_supports_open_transport(void)
{
    OSErr err;
    long  response = 0;

#ifdef gestaltOpenTpt
    err = Gestalt(gestaltOpenTpt, &response);
    if (err == noErr && response != 0)
        return true;
#endif

    err = Gestalt(FOUR_CHAR_CODE('otan'), &response);
    if (err == noErr && response != 0)
        return true;

    err = Gestalt(FOUR_CHAR_CODE('otvr'), &response);
    if (err == noErr && response != 0)
        return true;

    return false;
}

static void network_select_backend(void)
{
    if (!gBackend)
    {
        if (network_supports_open_transport())
            gBackend = network_backend_ot();
        else
            gBackend = network_backend_classic();
    }
}

void network_init(void)
{
    network_select_backend();

    if (gBackend && gBackend->init)
        (void)gBackend->init();
}

void network_shutdown(void)
{
    if (gBackend && gBackend->shutdown)
        gBackend->shutdown();
}

Boolean network_start_server(const char *hostString, UInt16 port)
{
    network_select_backend();

    if (gBackend && gBackend->start)
        return gBackend->start(hostString, port, &gCallbacks);

    return false;
}

void network_stop_server(void)
{
    if (gBackend && gBackend->stop)
        gBackend->stop();
}

Boolean network_is_running(void)
{
    if (gBackend && gBackend->is_running)
        return gBackend->is_running();

    return false;
}

void network_poll(void)
{
    if (gBackend && gBackend->poll)
        gBackend->poll();
}

void network_set_handlers(NetworkDataHandler onData, NetworkLogHandler onLog)
{
    gCallbacks.onData = onData;
    gCallbacks.onLog = onLog;
}
