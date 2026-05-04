# aMule Online Signature specification

## Introduction

The online signature exports aMule's statistics in a simple text-oriented
format.

Enable it under *Preferences → Online Signature*.

Two formats are supported:

* the original eMule online signature, written to `onlinesig.dat`
* the aMule online signature, written to `amulesig.dat`

This document specifies the latter.


## Format

`amulesig.dat` is a UTF-8 text file with one field per line. Fields appear
in the order below.

| Ordinal | Not running | Offline   | Online                                  | Connecting |
| ------- | ----------- | --------- | --------------------------------------- | ---------- |
| 1       | 0           | 0         | 1                                       | 2          |
| 2       | 0           | 0         | Server name                             | 0          |
| 3       | 0           | 0         | Server IP (dot-quad)                    | 0          |
| 4       | 0           | 0         | Server port                             | 0          |
| 5       | 0           | 0         | `H` or `L` (High-/Low-ID)               | 0          |
| 6       | 0.0         | As online | Download speed in kB/s                  | As online  |
| 7       | 0.0         | As online | Upload speed in kB/s                    | As online  |
| 8       | 0           | As online | Number of clients waiting for upload    | As online  |
| 9       | 0           | As online | Number of shared files                  | As online  |
| 10      | As online   | As online | Nick used on the eD2k network           | As online  |
| 11      | As online   | As online | Total download in bytes                 | As online  |
| 12      | As online   | As online | Total upload in bytes                   | As online  |
| 13      | As online   | As online | aMule version                           | As online  |
| 14      | 0           | As online | Total downloaded in session, in bytes   | As online  |
| 15      | 0           | As online | Total uploaded in session, in bytes     | As online  |
| 16      | 0           | As online | aMule uptime                            | As online  |

For comments and additions on this format please contact
<admin@amule.org>.


## Notes for implementors of tools accessing the online signature

* Be prepared to see linefeeds in either Unix or DOS format.
* Be prepared for new fields / lines being added.


## Links

* [`amulesig.dat`](https://github.com/amule-org/amule/wiki/Amulesig.dat_file) on
  the wiki.
* [`onlinesig.dat`](https://github.com/amule-org/amule/wiki/Onlinesig.dat_file)
  on the wiki.
