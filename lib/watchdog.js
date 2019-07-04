const watchdog = require('../build/Release/watchdog');

let _running = undefined;

function start(options) {
	options = Object.assign({
		timeout: 10000,
		ping: 1000,
		terminate: true,
		print: true
	}, options);

	watchdog.start(options);

	_running = {
		options,
		timer: setInterval(() => {
			watchdog.ping();
		}, options.ping)
	};
}

function stop() {
	const triggered = watchdog.stop();

	clearInterval(_running.timer);
	_running = undefined;

	return triggered;
}

exports.start = start;
exports.stop = stop;
