# Sharing Files

aMule shares files with other peers on the eD2k and Kademlia networks. Sharing is reciprocal — uploading to others earns credits that improve your own download queue position.

## Setting Up Shared Folders

1. Open **Preferences → Directories**.
2. Add one or more folders to the **Shared Directories** list.
3. aMule will hash all files in those folders and announce them to the network.

## The Shared Files Window

The **Shared Files** tab lists every file currently being shared. For each file you can see:

- File name, size, and ed2k hash
- Number of requests received
- Number of times it has been transferred
- Priority setting

## Upload Priorities

Each shared file can be assigned a priority (**Low**, **Normal**, **High**, **Release**). Higher-priority files are served first when upload slots are limited.

## Upload Slots

aMule manages upload slots automatically based on your configured upload speed limit. Setting this limit to roughly 80% of your actual upstream bandwidth keeps uploads stable and prevents download speed degradation.

## Credits System

aMule uses a credit system: peers that have uploaded data to you previously receive a better position in your upload queue, encouraging reciprocal sharing.
