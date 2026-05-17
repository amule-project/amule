# Downloading Files

aMule downloads files from the eD2k and Kademlia networks. Files are identified by their **ed2k hash** (an MD4 hash of the file content) and can be added via `ed2k://` links or search results.

## Adding Downloads

- **Via ed2k link**: Click an `ed2k://` link in your browser (requires the `ed2k` link handler to be installed), or paste the link using *File → Add Link*.
- **Via search**: Run a search in the **Search** tab and double-click a result to start downloading.

## The Transfers Window

Active downloads appear in the **Transfers** tab. Each entry shows:

- File name and size
- Download progress bar (color-coded by source availability)
- Current download speed and number of active sources
- Priority setting

## Priorities

You can set per-file priority to **Low**, **Normal**, **High**, or **Auto**. Higher-priority files are given preference in source negotiation.

## Pausing and Canceling

Right-click any download to pause, resume, or cancel it. Partial progress is preserved when you pause; canceling removes the `.part` file.

## AICH Integrity Verification

aMule uses the **AICH** (Advanced Intelligent Corruption Handler) hash set to detect and re-download corrupted chunks automatically, without discarding the entire file.
