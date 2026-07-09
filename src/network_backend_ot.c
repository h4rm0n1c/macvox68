#include "network_backend.h"

#include <OpenTransport.h>
#include <OpenTptInternet.h>
#include <string.h>
#include <stdio.h>

#ifndef kOTAnyInetAddress
#define kOTAnyInetAddress 0
#endif

#define NETWORK_CLIENT_SLOTS 4

typedef struct NetworkClient
{
    EndpointRef endpoint;
    Boolean     active;
} NetworkClient;

static Boolean                    gInitialized = false;
static Boolean                    gRunning     = false;
static EndpointRef                gListener    = NULL;
static NetworkClient              gClients[NETWORK_CLIENT_SLOTS];
static const NetworkCallbacks    *gCallbacks   = NULL;

static TCall       gListenCall;
static InetAddress gRemoteAddress;

static void network_log(const char *line)
{
    if (gCallbacks && gCallbacks->onLog && line)
        gCallbacks->onLog(line);
}

static UInt32 network_parse_ipv4(const char *text, Boolean *outValid)
{
    UInt32 octets[4] = {0, 0, 0, 0};
    UInt32 host = kOTAnyInetAddress;

    if (outValid)
        *outValid = false;

    if (text && sscanf(text, "%lu.%lu.%lu.%lu", &octets[0], &octets[1], &octets[2], &octets[3]) == 4)
    {
        Boolean ok = (Boolean)(octets[0] <= 255 && octets[1] <= 255 && octets[2] <= 255 && octets[3] <= 255);
        if (ok)
        {
            host = (UInt32)((octets[0] << 24) | (octets[1] << 16) | (octets[2] << 8) | octets[3]);
            if (outValid)
                *outValid = true;
        }
    }

    return host;
}

static void network_clear_client(NetworkClient *c)
{
    if (!c)
        return;

    if (c->endpoint)
    {
        OTCloseProvider(c->endpoint);
        c->endpoint = NULL;
    }
    c->active = false;
}

static void network_close_all_clients(void)
{
    short i;
    for (i = 0; i < NETWORK_CLIENT_SLOTS; ++i)
        network_clear_client(&gClients[i]);
}

static NetworkClient *network_allocate_client(void)
{
    short i;
    for (i = 0; i < NETWORK_CLIENT_SLOTS; ++i)
    {
        if (!gClients[i].active)
            return &gClients[i];
    }
    return NULL;
}

static void network_prepare_call_buffers(void)
{
    memset(&gListenCall, 0, sizeof(gListenCall));
    memset(&gRemoteAddress, 0, sizeof(gRemoteAddress));

    gListenCall.addr.maxlen = sizeof(gRemoteAddress);
    gListenCall.addr.len = 0;
    gListenCall.addr.buf = (UInt8 *)&gRemoteAddress;
}

static Boolean network_ot_init(void)
{
    if (!gInitialized)
    {
        InitOpenTransport();
        gInitialized = true;
    }
    return gInitialized;
}

static void network_ot_shutdown(void)
{
    network_close_all_clients();
    if (gListener)
    {
        OTCloseProvider(gListener);
        gListener = NULL;
    }

    gRunning = false;
    gInitialized = false;
}

static Boolean network_ot_start(const char *hostString, UInt16 port, const NetworkCallbacks *callbacks)
{
    OTConfigurationRef config;
    OSStatus status;
    InetAddress address;
    TBind bindReq;
    Boolean parsedHost = false;
    UInt32 host = network_parse_ipv4(hostString, &parsedHost);

    gCallbacks = callbacks;

    network_close_all_clients();

    if (!network_ot_init())
        return false;

    if (gListener)
    {
        OTCloseProvider(gListener);
        gListener = NULL;
    }

    network_prepare_call_buffers();

    config = OTCreateConfiguration("tcp");
    gListener = OTOpenEndpoint(config, 0, NULL, &status);
    if (!gListener || (status != kOTNoError))
    {
        network_log("Failed to open TCP listener endpoint.");
        return false;
    }

    (void)OTSetNonBlocking(gListener);

    OTInitInetAddress(&address, port, parsedHost ? host : kOTAnyInetAddress);

    bindReq.addr.maxlen = sizeof(InetAddress);
    bindReq.addr.len = sizeof(InetAddress);
    bindReq.addr.buf = (UInt8 *)&address;
    bindReq.qlen = NETWORK_CLIENT_SLOTS;

    status = OTBind(gListener, &bindReq, NULL);
    if (status != kOTNoError)
    {
        network_log("Open Transport bind failed.");
        network_ot_shutdown();
        return false;
    }

    network_log("Open Transport TCP listener ready.");
    gRunning = true;
    return true;
}

static void network_ot_stop(void)
{
    network_close_all_clients();

    if (gListener)
    {
        OTCloseProvider(gListener);
        gListener = NULL;
    }

    gRunning = false;
}

static Boolean network_ot_is_running(void)
{
    return gRunning;
}

static void network_log_disconnect(NetworkClient *client)
{
    if (!client)
        return;

    if (gCallbacks && gCallbacks->onLog)
        gCallbacks->onLog("Client disconnected.");
}

static Boolean network_handle_disconnect(NetworkClient *client)
{
    TDiscon discon;
    OSStatus status;

    if (!client || !client->endpoint)
        return false;

    memset(&discon, 0, sizeof(discon));
    status = OTRcvDisconnect(client->endpoint, &discon);
    (void)status;

    network_log_disconnect(client);
    network_clear_client(client);
    return true;
}

static void network_accept_pending(void)
{
    OSStatus status;

    if (!gListener)
        return;

    while (true)
    {
        status = OTListen(gListener, &gListenCall);
        if (status == kOTNoError)
        {
            NetworkClient *slot = network_allocate_client();
            if (slot)
            {
                OSStatus acceptStatus;
                slot->endpoint = OTOpenEndpoint(OTCreateConfiguration("tcp"), 0, NULL, &acceptStatus);
                if (slot->endpoint && acceptStatus == kOTNoError)
                {
                    (void)OTSetNonBlocking(slot->endpoint);
                    acceptStatus = OTAccept(gListener, slot->endpoint, &gListenCall);
                    if (acceptStatus == kOTNoError)
                    {
                        slot->active = true;
                        network_log("Accepted TCP connection.");
                    }
                    else
                    {
                        network_clear_client(slot);
                    }
                }
                else
                {
                    network_clear_client(slot);
                }
            }
            else
            {
                network_log("Connection backlog full; rejecting new client.");
            }
        }
        else if (status == kOTNoDataErr)
        {
            break;
        }
        else
        {
            break;
        }
    }
}

static void network_poll_client(NetworkClient *client)
{
    char buffer[NETWORK_READ_BUFFER];
    OSStatus err = kOTNoError;
    long readCount;

    if (!client || !client->active || !client->endpoint)
        return;

    readCount = OTRcv(client->endpoint, buffer, (long)(NETWORK_READ_BUFFER - 1), &err);
    if (readCount > 0)
    {
        buffer[readCount] = '\0';
        if (gCallbacks && gCallbacks->onData)
            gCallbacks->onData(buffer, readCount);
    }

    if (err == kOTLookErr)
    {
        OTResult look = OTLook(client->endpoint);
        if (look == T_DISCONNECT)
            (void)network_handle_disconnect(client);
    }
    else if (err != kOTNoError && err != kOTNoDataErr)
    {
        network_clear_client(client);
    }
}

static void network_ot_poll(void)
{
    short i;

    if (!gRunning)
        return;

    network_accept_pending();

    for (i = 0; i < NETWORK_CLIENT_SLOTS; ++i)
        network_poll_client(&gClients[i]);
}

static const NetworkBackend kOpenTransportBackend = {
    network_ot_init,
    network_ot_shutdown,
    network_ot_start,
    network_ot_stop,
    network_ot_is_running,
    network_ot_poll,
    "OpenTransport"
};

const NetworkBackend *network_backend_ot(void)
{
    return &kOpenTransportBackend;
}
