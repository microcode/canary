const watchdog = require('..');

console.log("Application should exit after 5000ms due to a deadlock in the event loop.");

watchdog.start({
    timeout: 5000
});

while (true);

watchdog.stop();