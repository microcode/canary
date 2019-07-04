canary
======

Native watchdog to monitor event loop deadlocks and terminate execution.

Usage
-----

```
const watchdog = require('@microcode/canary');

watchdog.start();
while (condition) {
    await doThing();
}
watchdog.stop();
```

How does it work
----------------

This module launches a native thread that expects the JavaScript event loop to send a ping from time to time. If the event loop gets too busy to do this for a substantial period of time, the watchdog will consider the application deadlocked.

Currently the watchdog can then kill the application, or just store that a deadlock occured and return this when stopping the watchdog.

API
---

## start(options)

Starts the watchdog.

- `timeout` - Timeout threshold for the watchdog in milliseconds (default: `10000`)
- `ping` - Event loop ping interval in milliseconds (default: `1000`)
- `terminate` - If set to true, the application will exit when triggered (default: `true`)
- `print` - If set to true, the application will print a message when triggered (default: `true`)

Throws an exception if the watchdog fails to start.

## stop()

Stops the watchdog.

Returns a boolean indicating if the watchdog was triggered or not.
