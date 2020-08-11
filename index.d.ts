interface Options {
    timeout?: number
    ping?: number
    terminate?: boolean
    print?: boolean
}

export function start(options: Options): void;
export function stop(): void;
