# amuled (Daemon)

`amuled` is the headless aMule daemon. It runs the full aMule engine without any GUI, making it suitable for servers, NAS devices, or any system where a graphical display is unavailable or undesirable.

## When to Use

Use `amuled` when you want aMule to run **continuously in the background**, independently of any user session. Connect to it remotely using [`amulegui`](amulegui.md), [`amuleweb`](amuleweb.md), or [`amulecmd`](amulecmd.md).

## Starting the Daemon

```sh
amuled
```

On first run, `amuled` generates `~/.aMule/amule.conf`. Edit this file to set an EC password before connecting with a remote client.

## Configuration

The key settings for headless operation live in `~/.aMule/amule.conf`:

```ini
[ExternalConnect]
AcceptExternalConnections=1
ECPassword=<md5 hash of your password>
ECPort=4712
```

## Running as a System Service

Most distributions allow you to manage `amuled` with systemd. Create a unit file that runs `amuled` as a dedicated unprivileged user.

## See Also

- [Configuration](../getting-started/configuration.md) for initial setup details.
- [EC Protocol](../advanced/ec-protocol.md) for the binary protocol used by remote clients.
