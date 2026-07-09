#ifndef NETWORK_BACKEND_H
#define NETWORK_BACKEND_H

#include "network.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NetworkCallbacks
{
    NetworkDataHandler onData;
    NetworkLogHandler  onLog;
} NetworkCallbacks;

typedef struct NetworkBackend
{
    Boolean (*init)(void);
    void (*shutdown)(void);
    Boolean (*start)(const char *hostString, UInt16 port, const NetworkCallbacks *callbacks);
    void (*stop)(void);
    Boolean (*is_running)(void);
    void (*poll)(void);
    const char *name;
} NetworkBackend;

const NetworkBackend *network_backend_ot(void);
const NetworkBackend *network_backend_classic(void);

#ifdef __cplusplus
}
#endif

#endif /* NETWORK_BACKEND_H */
