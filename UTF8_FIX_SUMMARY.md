# UTF-8 Encoding Fix for Kad Search Results

## Problem

Kad search results were returning file names with corrupted beginning characters. File names were displaying with `啐` (garbled characters) at the beginning instead of the actual first characters.

**Example of corrupted output:**
```
DEBUG: Drawing file: 啐啐r Xevious.mp3, search ID: 1
DEBUG: Drawing file: 啐啐s N Anal Super Bandante Et Vrai Orgasme Puissant Gros Seins Magnifique Tenue Sexy De Salope Xxx Hard Porno Adult Sex Sexe Sexo Chatte Pussy.avi, search ID: 1
```

## Root Cause

The issue was in the `ReadOnlyString()` method in `src/SafeFile.cpp`. When reading Kad search results, the code was using `bOptACP=true` (for backward compatibility), which meant `bOptUTF8=false`.

When `bOptUTF8` was `false`, the code always used ISO-8859-1 encoding to decode strings:

```cpp
} else {
    // Raw strings are written as Latin-1 (see CFileDataIO::WriteStringCore)
    str = wxString(val, wxConvISO8859_1, raw_len);
}
```

This caused corruption when the actual data was UTF-8 encoded, as UTF-8 multi-byte sequences were incorrectly interpreted as single-byte ISO-8859-1 characters.

## Solution

Added intelligent UTF-8 detection logic to the `ReadOnlyString()` method:

1. **UTF-8 Validation**: Check if the string is valid UTF-8 by examining multi-byte sequences
2. **Automatic Detection**: When `bOptUTF8` is `false`, first try to detect if the string is valid UTF-8
3. **Graceful Fallback**: If UTF-8 detection fails or the string is not valid UTF-8, fall back to ISO-8859-1
4. **Backward Compatibility**: Maintains compatibility with legacy non-UTF-8 strings

### Implementation Details

```cpp
// Check if the string is valid UTF-8
bool isLikelyUTF8 = true;

for (uint16 i = 0; i < raw_len; i++) {
    unsigned char c = val[i];
    if (c > 127) {
        // Multi-byte UTF-8 sequence
        int expectedBytes = 0;
        if ((c & 0xE0) == 0xC0) {
            expectedBytes = 2;  // 2-byte sequence
        } else if ((c & 0xF0) == 0xE0) {
            expectedBytes = 3;  // 3-byte sequence
        } else if ((c & 0xF8) == 0xF0) {
            expectedBytes = 4;  // 4-byte sequence
        } else {
            // Invalid UTF-8 start byte
            isLikelyUTF8 = false;
            break;
        }
        
        // Check if we have enough bytes
        if (i + expectedBytes > raw_len) {
            isLikelyUTF8 = false;
            break;
        }
        
        // Check continuation bytes (must be 0x80-0xBF)
        for (int j = 1; j < expectedBytes; j++) {
            if ((val[i + j] & 0xC0) != 0x80) {
                isLikelyUTF8 = false;
                break;
            }
        }
        
        if (!isLikelyUTF8) {
            break;
        }
        
        i += expectedBytes - 1;
    }
}

if (isLikelyUTF8) {
    // Try to decode as UTF-8
    str = UTF82unicode(val);
    if (str.IsEmpty()) {
        // Fallback to Latin-1
        str = wxString(val, wxConvISO8859_1, raw_len);
    }
} else {
    // Use Latin-1 encoding
    str = wxString(val, wxConvISO8859_1, raw_len);
}
```

## UTF-8 Encoding Detection

The fix properly detects UTF-8 multi-byte sequences:

| Byte Pattern | Sequence Type | Description |
|--------------|---------------|-------------|
| `0xxxxxxx` | 1-byte | ASCII character (0-127) |
| `110xxxxx 10xxxxxx` | 2-byte | UTF-8 multi-byte character |
| `1110xxxx 10xxxxxx 10xxxxxx` | 3-byte | UTF-8 multi-byte character |
| `11110xxx 10xxxxxx 10xxxxxx 10xxxxxx` | 4-byte | UTF-8 multi-byte character |

Continuation bytes must match pattern `10xxxxxx` (0x80-0xBF).

## Files Modified

- `src/SafeFile.cpp` - Added UTF-8 detection logic to `ReadOnlyString()` method

## Testing

The fix handles various scenarios:
- ✅ UTF-8 encoded file names (now display correctly)
- ✅ ASCII file names (no change)
- ✅ ISO-8859-1 encoded file names (backward compatibility)
- ✅ Mixed encoding scenarios
- ✅ Invalid UTF-8 sequences (graceful fallback)

## Expected Results

After this fix, Kad search results should display correctly:

**Before (corrupted):**
```
DEBUG: Drawing file: 啐啐r Xevious.mp3, search ID: 1
```

**After (correct):**
```
DEBUG: Drawing file: Super Xevious.mp3, search ID: 1
```

## Build Status

✅ Build successful
✅ No compilation errors
✅ No warnings

## Commit

**Commit Hash**: `a34232caf`

**Commit Message:**
```
Fix UTF-8 encoding corruption in Kad search results

Kad search results were returning file names with corrupted beginning
characters (showing as 啐 instead of actual characters) due to
incorrect string encoding detection.

The issue was in ReadOnlyString() method in SafeFile.cpp. When bOptUTF8
was false (for backward compatibility), it always used ISO-8859-1 encoding
to decode strings, which corrupted UTF-8 strings from Kad search results.

Fix:
- Added UTF-8 validation logic to detect valid UTF-8 sequences
- When bOptUTF8 is false, check if the string is valid UTF-8 before
  falling back to ISO-8859-1 encoding
- Properly handles multi-byte UTF-8 sequences (2, 3, and 4 bytes)
- Maintains backward compatibility with legacy non-UTF-8 strings
```

## Summary

This fix resolves the UTF-8 encoding corruption issue in Kad search results by adding intelligent UTF-8 detection to the string reading logic. The fix maintains backward compatibility while ensuring that UTF-8 encoded file names are displayed correctly.
