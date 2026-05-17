# Online Signature (amulesig)

The **Online Signature** feature writes a plain-text status file (`amulesig.dat`) that third-party tools can read to display aMule's current status — for example, in IRC clients, desktop widgets, or web pages.

## Enabling

Enable the feature in **Preferences → Statistics → Online Signature** and set the output file path (default: `~/.aMule/amulesig.dat`).

## File Format

The file contains 16 lines, each holding one value:

| Line | Field | Example |
|---|---|---|
| 1 | Connection status (0=disconnected, 1=connecting, 2=connected) | `2` |
| 2 | Server name | `eDonkey Server No1` |
| 3 | Server IP address | `176.103.48.36` |
| 4 | Server port | `4242` |
| 5 | ID type (`H`=HighID, `L`=LowID) | `H` |
| 6 | Current download speed (kB/s) | `150.3` |
| 7 | Current upload speed (kB/s) | `20.1` |
| 8 | Users on current server | `100000` |
| 9 | Files on current server | `8000000` |
| 10 | Number of shared files | `42` |
| 11 | Your nickname | `my_nick` |
| 12 | Total downloaded (bytes) | `10737418240` |
| 13 | Total uploaded (bytes) | `2147483648` |
| 14 | aMule version string | `aMule 3.0.0` |
| 15 | Session uptime (seconds) | `3600` |
| 16 | Kademlia status (0=off, 1=connecting, 2=running, 3=firewalled) | `2` |

## Notes

- The file is updated at a fixed interval while aMule is running.
- When aMule exits, the file is removed or zeroed to indicate offline status.
