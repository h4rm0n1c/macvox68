# Open Transport keepalive / half-open behavior

## Does Open Transport suffer half-open TCP connections?
Yes. Open Transport uses a normal TCP stack, so it inherits the usual half-open risk when one peer dies silently. The OT 1.1.1 tunable list includes keepalive controls (for example `tcp_keepalive_interval` and the related kill/detached intervals), which exist specifically to age out abandoned sessions rather than leaving them stuck forever.

## What we can do in-app
* Treat half-opens as possible even on System 7: poll endpoints and close them when they stop responding.
* If we want proactive probes, request TCP keepalive on each accepted endpoint using Open Transport option management so the stack sends keepalive probes and tears down unreachable peers automatically.
* Keep the current `OTLook` + disconnect handling: it already clears clients when the stack reports a disconnect, but adding keepalives will help catch the case where the remote end disappears without a FIN/RST.

## References
* Open Transport 1.1.1 tunables list keepalive controls such as `tcp_keepalive_interval`, `tcp_keepalives_kill`, and `tcp_keepalive_detached_interval`, indicating the stack provides keepalive probing and cleanup knobs.
