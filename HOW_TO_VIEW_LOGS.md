# How to View Codec Detection Logs in aMule

## Where to Find the Logs

The codec detection logs are displayed in the **aMule Log** window/tab in the main aMule GUI.

### Steps to View Logs

1. **Launch aMule**
   - Start aMule from your terminal or application menu

2. **Find the Log Tab**
   - Look for a tab or window labeled "Log" or "Server Log" in the main aMule window
   - The logs are displayed in a notebook with 4 pages:
     - **General Log** (page 0) - This is where codec logs appear
     - **ED2K Log** (page 1)
     - **Server Log** (page 2)
     - **Kad Log** (page 3)

3. **Perform a Kad Search**
   - When you perform a Kad search, you should see codec detection messages for each file name processed

### Example Log Output

When you perform a Kad search, you should see messages like:

```
Codec detected: UTF-8+BOM for string of length 45
Codec detected: UTF-8 for string of length 32
Codec detected: GBK (confidence 65%) for string of length 28
Codec detected: UTF-16LE for string of length 56
Codec detected: UTF-16BE for string of length 42
Codec fallback: LOCALE for string of length 38
Codec fallback: ISO-8859-1 for string of length 24
```

### What Each Message Means

- **`Codec detected: UTF-8+BOM`** - UTF-8 encoding with Byte Order Mark detected
- **`Codec detected: UTF-8`** - UTF-8 encoding detected
- **`Codec detected: GBK (confidence XX%)`** - GBK encoding detected (Chinese), with confidence percentage
- **`Codec detected: GB2312 (confidence XX%)`** - GB2312 encoding detected (Chinese)
- **`Codec detected: SHIFT_JIS (confidence XX%)`** - Shift-JIS encoding detected (Japanese)
- **`Codec detected: UTF-16LE`** - UTF-16 Little Endian detected
- **`Codec detected: UTF-16BE`** - UTF-16 Big Endian detected
- **`Codec fallback: LOCALE`** - Used system locale encoding (LANG/LANGUAGE environment variables)
- **`Codec fallback: ISO-8859-1`** - Used Latin-1 encoding as final fallback

### If You Don't See Logs

If you don't see the codec logs:

1. **Check the Log Tab**
   - Make sure you're looking at the correct tab (General Log, not ED2K/Server/Kad)

2. **Scroll Up**
   - The logs might have scrolled off the screen - scroll up to see earlier messages

3. **Check Log Level**
   - The logs use `AddLogLineN` which should work in all builds
   - No special debug mode or settings required

4. **Run aMule from Terminal**
   - If the GUI log window doesn't show the logs, try running aMule from a terminal:
     ```bash
     ./amule
     ```
   - The logs should also appear in the terminal output

### Troubleshooting

**Logs not appearing:**
- Make sure you've rebuilt aMule with the latest changes
- Check that you're looking at the General Log tab
- Try scrolling up in the log window

**Too many logs:**
- The codec logs are generated for every file name processed
- This can create a lot of log output during searches
- If it's overwhelming, you can comment out the `AddLogLineN` calls in `src/SafeFile.cpp`

### Current Implementation

The codec detection is implemented in `src/SafeFile.cpp` in the `ReadOnlyString()` function. The encoding detection priority is:

1. Check for BOMs (UTF-8, UTF-16 LE, UTF-16 BE)
2. Try UTF-8 (most common)
3. Use ICU to detect encoding (confidence >= 30%)
4. Try UTF-16 LE/BE
5. System locale fallback
6. Latin-1 final fallback

Each successful detection is logged using `AddLogLineN()` which outputs to the General log.
