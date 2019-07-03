const assert = require('assert').strict;
const watchdog = require('../lib/watchdog');

function sleep(ms) {
    return new Promise(resolve => setTimeout(resolve, ms));
}

describe('watchdog', function () {
    this.timeout(5000);

    it('should not trigger for a small stall', async function () {
        const context = watchdog.start({
            timeout: 1000,
            ping: 500,
            terminate: false
        });

        await sleep(2000);

        const triggered = watchdog.stop(context);

        assert.equal(triggered, false, "Should not trigger");
    });

    it('should trigger for a large stall', async function () {
        const context = watchdog.start({
            timeout: 500,
            ping: 3000,
            terminate: false
        });

        await sleep(2000);

        const triggered = watchdog.stop(context);

        assert.equal(triggered, true, "Should trigger");
    });

    it('should not allow it to be started twice', async function() {
        let context1 = undefined;
        let context2 = undefined;
        let in_error = false;

        try {
            context1 = watchdog.start({
                terminate: false
            });

            context2 = watchdog.start({
                terminate: false
            });
        } catch (e) {
            in_error = e.message === "Watchdog already running";
        }

        if (context1) {
            watchdog.stop(context1);
        }
        if (context2) {
            watchdog.stop(context2);
        }

        assert.notEqual(context1, undefined, "Not equal");
        assert.equal(context2, undefined, "Equal");
        assert.equal(in_error, true, "Should have thrown the correct Error");
    });
});
