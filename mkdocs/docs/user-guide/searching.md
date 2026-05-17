# Searching

aMule can search for files across the **eD2k server network** and the **Kademlia DHT**.

## Running a Search

1. Open the **Search** tab.
2. Enter a keyword in the search box.
3. Select the search type:
   - **Global** — queries the currently connected eD2k server.
   - **Kad** — searches the Kademlia DHT (decentralized, no server required).
   - **Filename / Hash / Ed2kLink** — for direct lookups.
4. Optionally filter by file type, minimum/maximum size, or availability.
5. Click **Start**.

## Search Results

Results appear in a list showing file name, size, source count, and availability. Double-click a result to add it to your download queue.

## Boolean Expressions

The search box supports simple boolean expressions:

- `space` between words = AND
- `|` between words = OR
- `-word` to exclude a term

## Limitations

- eD2k server searches are limited to the files indexed by your current server.
- Kad searches are broader but may return fewer results for rare files.
- Searches time out after a fixed window; re-run if results are sparse.
