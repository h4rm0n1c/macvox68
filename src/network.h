#ifndef NETWORK_H
#define NETWORK_H

#include <Types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Size of the temporary buffer used to move TCP data into the UI. */
#define NETWORK_READ_BUFFER 256

typedef void (*NetworkDataHandler)(const char *data, Size length);
typedef void (*NetworkLogHandler)(const char *line);

void network_init(void);
void network_shutdown(void);

Boolean network_start_server(const char *hostString, UInt16 port);
void network_stop_server(void);
Boolean network_is_running(void);

void network_poll(void);
void network_set_handlers(NetworkDataHandler onData, NetworkLogHandler onLog);

#ifdef __cplusplus
}
#endif

#endif /* NETWORK_H */
