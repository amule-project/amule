# amulecmd (Command-Line Interface)

`amulecmd` is an interactive command-line client for controlling a running `amuled` instance via the EC protocol.

## When to Use

Use `amulecmd` for **scripting, automation, or remote management** over SSH without a GUI or browser.

## Connecting

```sh
amulecmd --host=127.0.0.1 --port=4712 --password=<your-ec-password>
```

## Common Commands

Once connected, the interactive prompt accepts the following commands:

| Command | Description |
|---|---|
| `Status` | Show current connection and transfer status |
| `Download <ed2k-link>` | Add a file to the download queue |
| `Show DL` | List active downloads |
| `Show UL` | List active uploads |
| `Show Servers` | List known eD2k servers |
| `Connect` | Connect to the eD2k network |
| `Disconnect` | Disconnect from the eD2k network |
| `Quit` | Exit `amulecmd` |

Run `Help` inside the prompt for the full command reference.

## Non-Interactive Mode

`amulecmd` can execute a single command and exit, useful for scripts:

```sh
amulecmd --host=127.0.0.1 --password=secret -c "Status"
```

## See Also

- [EC Protocol](../advanced/ec-protocol.md) — the underlying binary protocol.
