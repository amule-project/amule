# EC Protocol

The **External Connections (EC) Protocol** is the binary wire protocol used by `amulegui`, `amuleweb`, and `amulecmd` to communicate with a running `amuled` instance.

## Overview

The protocol operates over a TCP connection (default port **4712**) and consists of two layers:

- **Transmission layer** — a 4-byte flags header followed by the packet body, optionally ZLIB-compressed.
- **Application layer** — packets composed of an opcode and a tree of typed tags (similar in concept to binary XML).

## Packet Structure

```
[ flags: uint32 (LSB-first) ]
[ packet body ]
  ├── opcode: uint8
  ├── tag count: uint16 (or uint32 with large-tag-count flag)
  └── tags[]
        ├── tag name
        ├── tag type
        ├── tag length
        ├── tag value
        └── (optional) sub-tags[]
```

## Flags

| Bit | Meaning |
|---|---|
| 0 | ZLIB-compressed body |
| 1 | Use UTF-8 encoded numbers |
| 2 | Packet contains an opcode |
| 3 | Large tag count (uint32) |

## Authentication

The client initiates authentication with `EC_OP_AUTH_REQ`, sending the client name, version, and an MD5 hash of the password. The server responds with `EC_OP_AUTH_OK` or `EC_OP_AUTH_FAIL`.

## Data Types

| Type | Encoding |
|---|---|
| Integer | MSB-first (big-endian) |
| String | UTF-8, length-prefixed |
| Boolean | Presence of tag = true |
| MD5 hash | 16 bytes raw |
| Float | Encoded as UTF-8 string |

## See Also

The full protocol specification with example hex-annotated packets is available in [`docs/EC_Protocol.md`](https://github.com/amule-project/amule/blob/master/docs/EC_Protocol.md) in the source repository.
