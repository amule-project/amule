# aMule External Connections Protocol — v2.0

> follow the white rabbit

## Preface

EC is under heavy construction; the protocol itself is considered stable
and you can rely on it, but opcodes, tagnames, tag content formats, and
values are still changing. If you decide to implement an application
using aMule EC, include `ECcodes.h` for the values, check this document
often, or read the source itself ([`src/ExternalConn.cpp`](../src/ExternalConn.cpp)
is a good start).


## Section 1 — Protocol definition

The EC protocol consists of two layers: a low-level **transmission
layer**, and a high-level **application layer**.


### Section 1.1 — Transmission layer

The transmission layer is completely independent of the application
layer and holds only transport-related information.

It consists of a single `uint32`, referenced below as **flags**, which
describes flags for the current send/receive operation. This is the
only value in the whole protocol that is transmitted **LSB first**, with
zero bytes omitted (an empty transmission flags value is sent as
`0x20`, not `0x20 0x00 0x00 0x00`).

#### Bit description

| Bit(s)            | Name                       | Meaning |
| ----------------- | -------------------------- | ------- |
| `0`               | Compression (`EC_FLAG_ZLIB`) | When set, zlib compression is applied to the application layer's data. |
| `1`               | Compressed numbers (`EC_FLAG_UTF8_NUMBERS`) | When set (presumably on small packets that aren't worth zlib-compressing), all numbers used in the protocol are encoded as a wide char converted to UTF-8 to avoid sending zero bytes. |
| `2`               | Has ID                     | When set, a `uint32` follows the flags — the packet ID. The response must echo the same ID. The only requirement is that IDs be unique within one session (or at least don't repeat for a reasonably long time). |
| `3`               | Reserved                   | Set to 0. |
| `4`               | Large tag count (`EC_FLAG_LARGE_TAG_COUNT`) | When set, indicates the sender uses the sentinel-extended `TAGCOUNT` encoding (see Section 1.2). Both sides must have advertised `EC_TAG_CAN_LARGE_TAG_COUNT` in their auth packet for the bit to appear in any subsequent flags. Without it, the historical 16-bit `TAGCOUNT` is used, capping any tag at 0xFFFE children for safe interoperation with old peers. |
| `5`               | Always 1                   | Distinguishes from older (pre-rc8) clients. |
| `6`               | Always 0                   | Distinguishes from older (pre-rc8) clients. |
| `7`, `15`, `23`   | Extension                  | Indicates that the next byte of flags is present. |
| `8`–`14`, `16`–`22`, `24`–`32` | Reserved      | Set to 0. |

#### Example

```
0x30 0x23 <appdata>
```

Client uses no extensions on this packet, and indicates that it can
accept zlib compression and compressed numbers.

#### Notes

* In the *accepts* value, the predefined flags (bits `5` and `6`) must
  be set to their predefined values — this can act as a sort of sanity
  check.
* Bits marked **Reserved** must always be set to 0.


### Section 1.2 — Application layer

Data transmission is done in **packets**. A packet is a special tag —
no data of its own, no tag-length field, but always with a `tagCount`
field. All numbers in the application layer are transmitted in **network
byte order** (MSB first).

A packet contains:

```c
[ec_opcode_t]   OPCODE
[uint16]        TAGCOUNT
<[uint32]       EXTENDED_TAGCOUNT>?
                <tags>
```

* **OPCODE** indicates the operation or what the data fields contain.
  Type `ec_opcode_t`, currently `uint8`.
* **TAGCOUNT** is the number of first-level tags in this packet,
  followed by the tags themselves.
* **EXTENDED_TAGCOUNT** (optional, only when `EC_FLAG_LARGE_TAG_COUNT`
  is in effect, see Section 1.1): if `TAGCOUNT == 0xFFFF`, a `uint32`
  follows carrying the actual count. This sentinel-extended encoding
  lifts the historical 65535-tag ceiling so that responses for large
  shared-file libraries (etc.) can carry their full size. Senders
  emit the `0xFFFF` marker only for `count >= 0xFFFF`, and only when
  the receiver has advertised `EC_TAG_CAN_LARGE_TAG_COUNT` in the
  auth handshake. Otherwise `TAGCOUNT` is a plain `uint16` and counts
  larger than `0xFFFE` are silently truncated to `0xFFFE` to avoid
  ambiguity (the value `0xFFFF` is reserved as the sentinel).

A tag contains:

```c
[ec_tagname_t]  TAGNAME
[ec_tagtype_t]  TAGTYPE
[ec_taglen_t]   TAGLEN
<[uint16]       TAGCOUNT>?
                <sub-tags>
                <tag data>
```

* `ec_tagname_t` is `uint16`, `ec_tagtype_t` is `uint8`, `ec_taglen_t`
  is `uint32` (current values; subject to change).
* **TAGNAME** identifies the tag content — see `ECcodes.h`.
* **TAGTYPE** identifies the data type of this tag — see `ECPacket.h`.
* **TAGLEN** is the total tag length, *including* sub-tag lengths but
  *excluding* the size of the `TAGNAME`, `TAGTYPE`, and `TAGLEN` fields
  themselves. The lowest bit of `TAGNAME` is **not** part of the name
  itself (see below) — clear it before comparing.

Tags may contain sub-tags. A `TAGCOUNT` field is present only when the
tag has sub-tags; presence is indicated by the **lowest bit of
`TAGNAME`** being set. When a tag contains sub-tags, the sub-tags are
sent before the tag's own data. Tag-data length can be calculated by
subtracting all sub-tags' total length from `TAGLEN`.

When `EC_FLAG_LARGE_TAG_COUNT` is in effect, the sub-tag `TAGCOUNT`
field uses the same sentinel-extended encoding as the packet-level
`TAGCOUNT` (a `uint32` follows when the `uint16` reads `0xFFFF`).


## Section 2 — Data types

### Integer types

Integer types (`uint8`, `uint16`, `uint32`, …) are always transmitted
in network byte order (MSB first).

### Strings

Strings are always **UTF-8**, including the trailing zero byte. All
strings coming from the server are untranslated, but their translations
are included in aMule's translation database (`amule.mo`).

### Boolean

This one is tricky:

* **When reading**, the tag's *presence* means `true`; *absence* means
  `false`.
* **When writing**, booleans should always be present — if absent the
  receiver treats it as *unchanged*. The tag must hold a `uint8`: `0`
  is `false`, non-zero is `true`.

Boolean values are mostly used in reading/writing preferences.

### MD5 hashes

Always MSB first.

### Floating-point numbers

`float` and `double` types are converted to their *string*
representation and sent as strings. The decimal point is always `.`
(dot), independent of the current locale.


## Section 3 — Clarifying things

If the above seemed too technical, keep reading. If you understood it
on first read, you can safely skip this section.

Have you seen an XML file? Then think of an EC packet as binary XML.
Otherwise, think of it as a tree: exactly one root, possibly many
branches and leaves. We'll use the tree analogy below.

About the flags (which are part of the transmission layer): when
developing an EC application, this is the last thing you want to care
about, and that's fine. Just keep sending `0x20` as flags, and aMule
will never want to use any of the extensions described in
[Section 1.1](#section-11--transmission-layer). You only have to
*tolerate* the *accepts* value aMule sends in its first reply.

The example packets below are real-life EC packets, transcribed to
textual form.

### Example 1 — Authentication

This is the very first packet you send, otherwise aMule may drop the
connection.

```
EC_OP_AUTH_REQ (0x02)
    +-- EC_TAG_CLIENT_NAME            (0x06) (optional)
    +-- EC_TAG_PASSWD_HASH            (0x04)
    +-- EC_TAG_PROTOCOL_VERSION       (0x0c)
    +-- EC_TAG_CLIENT_VERSION         (0x08) (optional)
    +-- EC_TAG_VERSION_ID             (0x0e) (required for CVS versions, must
                                              not be present for releases)
    +-- EC_TAG_CAN_ZLIB               (0x0c) (optional, advertises capability)
    +-- EC_TAG_CAN_UTF8_NUMBERS       (0x0d) (optional, advertises capability)
    +-- EC_TAG_CAN_NOTIFY             (0x0e) (optional, advertises capability)
    +-- EC_TAG_CAN_LARGE_TAG_COUNT    (0x11) (optional, advertises capability)
```

Each `EC_TAG_CAN_*` is an empty tag advertising support for the
corresponding wire-format extension. The server may use the matching
flag (`EC_FLAG_ZLIB`, `EC_FLAG_UTF8_NUMBERS`, `EC_FLAG_LARGE_TAG_COUNT`,
…) only when both sides have advertised the capability — clients that
omit a `CAN` tag get the historical wire format for that feature.

For symmetry, the server echoes the `CAN` tags it accepts in its
`EC_OP_AUTH_OK` response so the client knows which extensions are
actually negotiated for this connection. A client that only sees
`EC_TAG_SERVER_VERSION` in the reply (no `CAN` tags) is talking to an
older daemon and must avoid sending packets that need the corresponding
extensions.

What gets transmitted (all numbers hexadecimal, the `0x` prefix omitted
for readability):

```
20                                FLAGS — using ECv2
02                                EC_OP_AUTH_REQ
  00 05                           Number of children (tags)
    00 06                         EC_TAG_CLIENT_NAME
      0?                          EC_TAGTYPE_STRING
      00 00 00 09                 Length 9
      61 4d 75 6c 65 63 6d 64 00  "aMulecmd" + trailing zero
    00 08                         EC_TAG_CLIENT_VERSION
      0?                          EC_TAGTYPE_STRING
      00 00 00 04                 Length 4
      43 56 53 00                 "CVS"
    00 0c                         EC_TAG_PROTOCOL_VERSION
      0?                          EC_TAGTYPE_UINT??
      00 00 00 02/4/8             Length 2/4/8 (16/32/64-bit value follows)
      00? 00? 01 f2               0x0200 (current protocol version for CVS)
    00 04                         EC_TAG_PASSWD_HASH
      0?                          EC_TAGTYPE_HASH
      00 00 00 10                 Length 16
      5d 41 40 2a bc 4b 2a 76     16 bytes md5sum of EC password
      b9 71 9d 91 10 17 c5 92
    00 0e                         EC_TAG_VERSION_ID
      0?                          EC_TAGTYPE_CUSTOM
      00 00 00 21                 Length 33
      62 66 39 64 64 32 36 35     33 bytes of unique CVS version ID
      32 36 34 35 31 36 63 39     (CVS only — size, content, anything
      34 35 38 36 38 66 61 39     can change without notice; for releases
      30 38 66 62 37 64 39 38     this tag MUST NOT be present)
      00
```

The reply, hopefully:

```
30                                FLAGS — server sends an "accepts" flag
23                                the "accepts" flag itself; just take
                                  care that your program tolerates it
04                                EC_OP_AUTH_OK
  00 01                           Number of children
    00 76                         EC_TAG_SERVER_VERSION
      0?                          EC_TAGTYPE_STRING
      00 00 00 04                 Length 4
      43 56 53 00                 "CVS"
```

### Example 2 — Simple stats request

```
EC_OP_STAT_REQ
    +-- EC_TAG_DETAIL_LEVEL (with EC_DETAIL_CMD value)
```

```
20                                FLAGS
0a                                EC_OP_STAT_REQ
  00 01                           TagCount: 1
    00 10                         EC_TAG_DETAIL_LEVEL
      0?                          EC_TAGTYPE_UINT8
      00 00 00 01                 Length 1
      00                          0 = EC_DETAIL_CMD
```

The reply (assuming core is connected to a server):

```
EC_OP_STATS
    +-- EC_TAG_STATS_UL_SPEED
    +-- EC_TAG_STATS_DL_SPEED
    +-- EC_TAG_STATS_UL_SPEED_LIMIT
    +-- EC_TAG_STATS_DL_SPEED_LIMIT
    +-- EC_TAG_STATS_CURR_UL_COUNT
    +-- EC_TAG_STATS_TOTAL_SRC_COUNT
    +-- EC_TAG_STATS_CURR_DL_COUNT
    +-- EC_TAG_STATS_TOTAL_DL_COUNT
    +-- EC_TAG_STATS_UL_QUEUE_LEN
    +-- EC_TAG_STATS_BANNED_COUNT
    +-- EC_TAG_CONNSTATE
        +-- EC_TAG_SERVER
            +-- EC_TAG_SERVER_NAME
```

The interesting part of the reply packet:

```
20                                FLAGS
0c                                EC_OP_STATS
  00 0b                           Number of first-level tags: 11
    00 14 [...]                   EC_TAG_STATS_UL_SPEED
    00 16 [...]                   EC_TAG_STATS_DL_SPEED
    00 18 [...]                   EC_TAG_STATS_UL_SPEED_LIMIT
    00 1a [...]                   EC_TAG_STATS_DL_SPEED_LIMIT
    00 1c [...]                   EC_TAG_STATS_CURR_UL_COUNT
    00 22 [...]                   EC_TAG_STATS_TOTAL_SRC_COUNT
    00 1e [...]                   EC_TAG_STATS_CURR_DL_COUNT
    00 20 [...]                   EC_TAG_STATS_TOTAL_DL_COUNT
    00 26 [...]                   EC_TAG_STATS_UL_QUEUE_LEN
    00 24 [...]                   EC_TAG_STATS_BANNED_COUNT
    00 13                         EC_TAG_CONNSTATE — odd tagname means
                                  the tag has children. The true tagname
                                  is <found>-1: EC_TAG_CONNSTATE = 0x0012,
                                  and 0x0013 - 1 = 0x0012, so this is it.
                                  Odd-tagname tags also carry a tagcount
                                  field.
      0?                          EC_TAGTYPE_UINT32
      00 00 00 26                 TagLen: 38 (own content + children with headers)
      00 01                       TagCount: 1
        00 61                     EC_TAG_SERVER (has children)
          0?                      EC_TAGTYPE_IPV4
          00 00 00 1a             TagLen: 27 (own content 6 + child content 14 + child header 7)
          00 01                   TagCount: 1
            00 62                 EC_TAG_SERVER_NAME
              0?                  EC_TAGTYPE_STRING
              00 00 00 0e         TagLen: 14
              52 61 7a 6f 72 62 61 63  Content: "Razorback 2.0"
              6b 20 32 2e 30 00
          c3 f5 f4 f3 12 35       EC_TAG_SERVER content: Server IP:Port
                                  (195.245.244.243:4661)
      90 cc 83 52                 EC_TAG_CONNSTATE content: current UserID
```

Hopefully these examples clarified opcodes, tags, and nested tags.


## Section 4 — Notable tag types

This section documents the data types of selected tags where the type
isn't immediately obvious or has changed across protocol versions.

### Connection preferences (`EC_TAG_PREFS_CONNECTIONS = 0x1300`)

| Tag                                | Code     | Type     | Description |
| ---------------------------------- | -------- | -------- | ----------- |
| `EC_TAG_CONN_DL_CAP`               | `0x1301` | `uint32` | Download line capacity (kB/s) |
| `EC_TAG_CONN_UL_CAP`               | `0x1302` | `uint32` | Upload line capacity (kB/s) |
| `EC_TAG_CONN_MAX_DL`               | `0x1303` | `uint32` | Max download speed (kB/s) |
| `EC_TAG_CONN_MAX_UL`               | `0x1304` | `uint32` | Max upload speed (kB/s) |
| `EC_TAG_CONN_SLOT_ALLOCATION`      | `0x1305` | `uint32` | Upload slot allocation |
| `EC_TAG_CONN_MAX_FILE_SOURCES`     | `0x1309` | `uint16` | Max sources per file |
| `EC_TAG_CONN_MAX_CONN`             | `0x130A` | `uint16` | Max connections |

> **Note**: `EC_TAG_CONN_MAX_DL`, `EC_TAG_CONN_MAX_UL`, and
> `EC_TAG_CONN_SLOT_ALLOCATION` were widened from `uint16` to `uint32`
> to support speeds above 65534 kB/s (~524 Mbps) required on modern
> gigabit connections. EC clients reading these tags should use
> `GetInt()` (which handles any integer width); clients sending them
> should encode them as 32-bit values.
