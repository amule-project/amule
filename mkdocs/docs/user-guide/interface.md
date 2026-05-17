# Interface Overview

The aMule GUI is organized into several tabs, each accessible from the toolbar at the top of the main window.

## Main Tabs

| Tab | Description |
|---|---|
| **My Info** | Shows your connection status, ID type (HighID/LowID), and current server |
| **Servers** | Lists known eD2k servers; connect, add, or remove servers |
| **Transfers** | Active downloads and uploads with progress and speed |
| **Search** | Search the eD2k network and Kademlia for files |
| **Shared Files** | Browse and manage the files you are sharing |
| **Messages** | Peer-to-peer chat with other aMule/eMule users |
| **Statistics** | Graphs for bandwidth, sessions, and network activity |
| **Kad** | Kademlia DHT status and bootstrap controls |

## Transfer Colors

The download progress bar in the **Transfers** tab uses colors to indicate which parts of a file have been found:

| Color | Meaning |
|---|---|
| Black | Part not available from any source |
| Red | Part available from at least one source |
| Blue | Part available from multiple sources |
| Yellow | Part currently being downloaded |
| Green | Part already downloaded and verified |

## Status Bar

The status bar at the bottom displays current download speed, upload speed, connection status, and the number of users on the network.
