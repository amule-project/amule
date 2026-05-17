# aMule (GUI Client)

`amule` is the all-in-one graphical client. It bundles the core aMule engine together with a wxWidgets-based GUI into a single executable.

## When to Use

Use `amule` when you want a **local, self-contained desktop application**. It is the simplest way to get started — no separate daemon process is needed.

## Starting aMule

```sh
amule
```

On first launch, aMule runs a setup wizard to configure your download folder, bandwidth limits, and nickname.

## Features

- Full GUI for all aMule functionality: downloads, uploads, search, shared files, statistics, and chat.
- System tray icon for background operation.
- Built-in statistics graphs.
- ed2k:// link handling (requires the `ed2k` utility to be installed as a URI handler).

## Limitations

- The process must remain running for transfers to continue.
- For server or always-on use, consider [`amuled`](amuled.md) instead.
