# aMule — User Guide

This document covers the end-user side of aMule: how to launch each
tool, what to configure on first run, and where the safety footguns
are. For an overview of the project see the
[top-level README](../README.md). For build / install instructions see
[INSTALL.md](INSTALL.md).


## What you got

aMule is shipped as several binaries that share the same on-disk state
under `~/.aMule/`:

| Binary      | What it is | When to use it |
| ----------- | ---------- | -------------- |
| `amule`     | The all-in-one GUI client. Daemon and UI in one process. | Day-to-day use on a desktop. |
| `amuled`    | The headless daemon — same engine as `amule` minus the UI. | When you want aMule running on a NAS / VPS / always-on box and connect to it remotely. |
| `amulegui`  | A remote GUI that talks to a running `amuled` over the [EC protocol](EC_Protocol.md). | Drive a remote `amuled` from your desktop. |
| `amuleweb`  | A small HTTP server that exposes a running `amuled` to a browser. | Drive a remote `amuled` from a phone or another machine without installing anything. |
| `amulecmd`  | An interactive CLI that talks to a running `amuled`. | Scripts, headless administration, troubleshooting. |
| `ed2k`      | A tiny helper that hands `ed2k://` URLs to a running aMule. | Click an `ed2k://` link in a browser and have aMule pick it up. |

Pick `amule` if you're not sure which to use — it's the all-in-one.


## First-run checklist

aMule ships with reasonable defaults and is usable as-is. Three
configuration steps are still worth doing on day one:

### 1. Open the ports — get a HighID

aMule needs **TCP 4662** and **UDP 4665** + **UDP 4672** reachable from
the internet to be a full peer on the network. If they're not, you'll
get a *LowID*, which makes you reachable from far fewer sources.

* **Behind a router**: forward the three ports to the machine running
  aMule (or use UPnP — `Preferences → Connection → UPnP enabled`).
* **Behind a firewall**: allow inbound on those ports.

The wiki has detailed walkthroughs for [getting a HighID][highid] and
[firewall rules][firewall].

[highid]: https://github.com/amule-org/amule/wiki/Get_HighID
[firewall]: https://github.com/amule-org/amule/wiki/Firewall

### 2. Set realistic upload / download limits

Under `Preferences → Connection`, set the limits to roughly **80 % of
your actual line speed** to avoid saturating the upstream and starving
your own traffic.

The values are in **kilobytes per second** (kB/s). ISP advertised speed
is usually in **megabits per second** (Mbps); divide by 8 for kB/s.

> Example: a 100 Mbps / 20 Mbps fibre line → roughly 10000 kB/s
> downstream and 2000 kB/s upstream. Set the *limits* to about 8000
> down / 1600 up to stay below the line cap.

### 3. Pick what you share

`Preferences → Shared Folders` controls what you offer to the network.
Defaults are conservative; **don't share blanket filesystem trees**.

> **Never share** your entire home directory or a system root.
> `/etc`, `/var`, `/lib`, `/boot`, `/usr` and similar must stay
> private. Don't share folders containing private files (SSH keys,
> tax documents, password files, etc.).

A focused share — the Incoming dir, plus one or two media folders —
is the safe default. If you share more than ~200 files, some servers
may drop you because of a per-client file limit; lots of small files
beats one giant index.


## Running aMule headless (`amuled`)

If you want aMule running on a NAS, VPS, or always-on home server, use
`amuled`. First-time setup:

```sh
# Generate a config dir and EC password (the daemon needs a password
# for amulegui / amuleweb / amulecmd to connect)
amuled                                # interactive once; Ctrl-C
$EDITOR ~/.aMule/amule.conf           # set [ExternalConn]
                                      # AcceptExternalConnections=1
                                      # ECPassword=<md5 of your password>

# Then run it daemonised
amuled --full-daemon
```

Connect to it from another machine with `amulegui`, `amuleweb`, or
`amulecmd` using the same EC password.

The wiki has detailed walkthroughs for
[amuled setup](https://github.com/amule-org/amule/wiki/Amuled),
[amuleweb](https://github.com/amule-org/amule/wiki/Amuleweb), and
[amulecmd](https://github.com/amule-org/amule/wiki/Amulecmd).


## Reading the transfers window

aMule's per-file progress bar uses colour to communicate availability:

| Colour              | Meaning |
| ------------------- | ------- |
| **Black**           | Parts you already have |
| **Red**             | Parts missing in *all* known sources — nobody on the network can give them to you right now |
| **Blue (shades)**   | Parts available in known sources; darker = higher availability |
| **Yellow**          | Part being downloaded *right now* |
| **Green** (top bar) | Total file completion |

Per-source bars (when you expand a download):

| Colour      | Meaning |
| ----------- | ------- |
| **Black**   | Parts you're still missing |
| **Silver**  | Parts this source is also missing |
| **Green**   | Parts you already have |
| **Yellow**  | Part being uploaded *to you* right now |

A red part isn't necessarily lost — it just means none of your *current*
sources have it. Switching servers, joining Kad, or simply waiting often
turns red into blue/black over hours.


## Common file types

| Group     | Extensions |
| --------- | ---------- |
| Audio     | `mp3` `m4a` `aac` `flac` `ogg` `opus` `wav` `wma` `ape` |
| Video     | `mkv` `mp4` `webm` `mov` `avi` `mpg` `mpeg` `m4v` `vob` `wmv` |
| Archive   | `zip` `7z` `rar` `tar.gz` `tar.bz2` `tar.xz` `tar.zst` `cbz` `cbr` |
| Disc image| `iso` `img` `bin/cue` `nrg` `mds/mdf` `ccd/sub` |
| Documents | `pdf` `epub` `mobi` `djvu` `chm` `azw3` |
| Images    | `jpg` `jpeg` `png` `webp` `avif` `gif` `tif` `heic` |
| Software  | `exe` `msi` `dmg` `pkg` `deb` `rpm` `AppImage` `flatpak` |

aMule's search also supports filtering by these categories — click the
type dropdown in the search panel.


## Troubleshooting

* **"LowID"** — your ports aren't reachable. See the
  [HighID guide][highid].
* **Generic icon in the launcher** — the GTK icon-theme cache hasn't
  refreshed since install. See the [icon-cache section in
  INSTALL.md](INSTALL.md#linux-only-icon-cache-step).
* **AppImage doesn't appear in the application menu** — on first
  launch the AppImage will prompt to integrate itself; if you declined
  ("Don't ask again"), the wiki documents the manual install of the
  `.desktop` file.
* **Tray icon invisible on GNOME** — you need an SNI host. On vanilla
  GNOME install
  [`gnome-shell-extension-appindicator`](https://extensions.gnome.org/extension/615/appindicator-support/);
  Ubuntu enables this by default.

For anything else, the wiki and forum are the best places to look:

* Wiki: <https://github.com/amule-org/amule/wiki>
* Forum: <https://github.com/amule-org/amule/discussions>
* GitHub Issues: <https://github.com/amule-org/amule/issues>


## Safety / legal

aMule is an interface to the eD2k and Kad networks. The aMule
developers have no control over what other peers transfer through this
medium and cannot be held liable for non-personal copyright
infringement or other illegal activity by third parties. Share
responsibly.
