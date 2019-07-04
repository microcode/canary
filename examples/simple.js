const watchdog = require('..');

process.stdout.write("Application will run for 10000ms and then exit.\n");

function sleep(ms) { return new Promise(resolve => setTimeout(resolve, ms)); }

async function run() {
	const context = watchdog.start({ timeout: 5000 });

	for (let i = 0; i < 10; ++i)
	{
		process.stdout.write(`Loop ${i+1}/10\r`);
		await sleep(1000);
	}

	watchdog.stop(context);

	process.stdout.write("\nDone.\n");
}

run();
