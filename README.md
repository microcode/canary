canary
======

Native watchdog to monitor event loop deadlocks and terminate execution.

Usage
-----

```
const watchdog = require('@microcode/canary');

watchdog.start();
while (condition) {
    dothing();
}
watchdog.stop();
```

How does it work
----------------

The module consists of two parts

1) Javascript - while watchdog is active, sends a ping to the native addon. If the event loop for some reason stalls, the pings stops being sent.
2) Native - while active a separate thread is running, comparing the last ping with the current timestamp. If the duration exceeds the configured timeout, the watchdog triggers.

API
---

**start(options)**

Starts the watchdog

options:
```
    timeout - For how long (in milliseconds) the watchdog should monitor before triggering (default 10000ms)
    ping - How often the ping should be sent to notify the watchdog that the event loop is still running. (default 1000ms)
    terminate - If application should terminate when watchdog triggers (default true)
```

Returns the watchdog context, or undefined if watchdog failed to initialize.

**stop(context)**

Stops the watchdog from monitoring the application

context - Context returned by **watchdog.start()**.
