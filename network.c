#include "network.h"

#include "network_backend.h"

static NetworkCallbacks        gCallbacks;
static const NetworkBackend   *gBackend = NULL;

static void network_select_backend(void)
{
    if (!gBackend)
        gBackend = network_backend_ot();
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
