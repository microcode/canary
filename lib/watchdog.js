const watchdog = require('../build/Release/watchdog');

function start(options) {
	options = Object.assign({
		timeout: 10000,
		ping: 1000,
		terminate: true,
		print: true
	}, options);

	watchdog.start(options);

	return {
		options,
		timer: setInterval(() => {
			watchdog.ping();
		}, options.ping)
	};
}

function stop(context) {
	if (!context || !context.timer) {
		throw new Error("No valid context provided");
	}

	clearInterval(context.timer);
	context.timer = undefined;

	return watchdog.stop();
}

exports.start = start;
exports.stop = stop;
