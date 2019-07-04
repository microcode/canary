const watchdog = require('..');

process.stdout.write("Application will exit after 5000ms due to a deadlock in the event loop.\n");

function badsleep(ms) {
	const d = new Date().getTime() + ms;
	while (d > new Date().getTime());
}

function run() {
	const context = watchdog.start({ timeout: 5000 });

	for (let i = 0; i < 10; ++i)
	{
		process.stdout.write(`Loop ${i+1}/10\r`);
		badsleep(1000);
	}

	watchdog.stop(context);

	process.stdout.write("\nDone.\n");
}

run();
