#include "network_backend.h"

#include <Dialogs.h>

static const NetworkCallbacks *gCallbacks = NULL;

static void network_classic_log(const char *line)
{
    if (gCallbacks && gCallbacks->onLog && line)
        gCallbacks->onLog(line);
}

static Boolean network_classic_init(void)
{
    return true;
}

static void network_classic_shutdown(void)
{
}

static Boolean network_classic_start(const char *hostString, UInt16 port, const NetworkCallbacks *callbacks)
{
    short    itemHit = 0;
    OSStatus status;

    (void)hostString;
    (void)port;

    gCallbacks = callbacks;

    status = StandardAlert(
        kAlertStopAlert,
        "\pThis Application does not support Classic Networking, please install or activate Open Transport in the Network Software Selector Application.",
        NULL,
        NULL,
        &itemHit);

    (void)itemHit;
    (void)status;

    network_classic_log("Classic Networking detected; TCP server disabled.");

    return false;
}

static void network_classic_stop(void)
{
}

static Boolean network_classic_is_running(void)
{
    return false;
}

static void network_classic_poll(void)
{
}

static const NetworkBackend kClassicBackend = {
    network_classic_init,
    network_classic_shutdown,
    network_classic_start,
    network_classic_stop,
    network_classic_is_running,
    network_classic_poll,
    "ClassicNetworking",
};

const NetworkBackend *network_backend_classic(void)
{
    return &kClassicBackend;
}
