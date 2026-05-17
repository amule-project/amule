# amuleweb (Web Interface)

`amuleweb` is a lightweight HTTP server that provides a browser-based interface to a running `amuled` instance. It translates web requests into EC protocol commands.

## When to Use

Use `amuleweb` when you want to control `amuled` from **any web browser** — useful for remote access without installing a native client.

## Starting amuleweb

```sh
amuleweb --host=127.0.0.1 --port=4712 --password=<your-ec-password>
```

By default, the web interface listens on port **4711**. Open `http://<host>:4711` in your browser.

## Configuration Options

| Option | Description |
|---|---|
| `--host` | Hostname of the `amuled` instance |
| `--port` | EC port of `amuled` (default 4712) |
| `--password` | EC password |
| `--webport` | Port for the HTTP server (default 4711) |
| `--template` | Web template directory to use |

## Supported Operations

- View and manage active downloads and uploads.
- Run searches and add results to the download queue.
- Add files via `ed2k://` links.
- Browse shared files.
- View basic statistics.

## Security

By default, `amuleweb` has no authentication of its own. Restrict access using a firewall or reverse proxy with authentication if the web interface is exposed beyond localhost.
