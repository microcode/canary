const watchdog = require('../build/Release/watchdog');

function start(options) {
	options = Object.assign({
		timeout: 10000,
		ping: 1000,
		terminate: true
	}, options);

	const id = watchdog.start(options);
	if (!id) {
		throw new Error("Failed to start watchdog");
	}

	const context = {
		options,
		id
	};

	context.timer = setInterval(() => {
		watchdog.ping(id);
	}, options.ping);

	return context;
}

function stop(context) {
	if (!context || !context.id || !context.timer) {
		throw new Error("No valid context provided");
	}

	const triggered = watchdog.stop(context.id);
	clearInterval(context.timer);

	context.id = undefined;
	context.timer = undefined;

	return triggered;
}

exports.start = start;
exports.stop = stop;
