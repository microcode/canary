const assert = require('assert').strict;
const watchdog = require('..');

function sleep(ms) {
    return new Promise(resolve => setTimeout(resolve, ms));
}

describe('watchdog', function () {
    this.timeout(5000);

    it('should not trigger for a small stall', async function () {
        watchdog.start({
            timeout: 1000,
            ping: 500,
            terminate: false
        });

        await sleep(2000);

        const triggered = watchdog.stop();
        assert.equal(triggered, false, "Should not trigger");
    });

    it('should trigger for a large stall', async function () {
        watchdog.start({
            timeout: 500,
            ping: 3000,
            terminate: false,
            print: false
        });

        await sleep(2000);

        const triggered = watchdog.stop();
        assert.equal(triggered, true, "Should trigger");
    });

    it('should not allow it to be started twice', async function() {
        let wd1 = false, wd2 = false;
        let in_error = false;

        try {
            watchdog.start({ terminate: false });
            wd1 = true;
            watchdog.start({ terminate: false });
            wd2 = true;
        } catch (e) {
            in_error = e.message === "Watchdog already running";
        } finally {
            watchdog.stop();
        }

        assert.equal(wd1, true, "Watchdog 1 should have been started");
        assert.equal(wd2, false, "Watchdog 2 should not have been started");
        assert.equal(in_error, true, "Should have thrown the correct Error");
    });

    it('should throw an error if attempting to stop without first starting', async function () {
        let in_error = false;
        try {
            watchdog.stop();
        } catch (e) {
            in_error = e.message === "Watchdog is not running";
        }
        assert.equal(in_error, true, "Should have thrown the correct Error");
    });
});
