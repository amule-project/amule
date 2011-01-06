Make aMule use an Sqlite database to store data previously kept in memory

CURRENT STATE IS "UNFINISHED AND USELESS". Don't use !

If you want to play with it, better talk to me (Stu) first.

To compile you need to install libsqlite3-dev (Ubuntu).

State (6.1.2011):
Kad key index is stored in db.
Cleanup, create response are missing.
Kad tables are created from scratch from stored Kad data on every startup

Next steps:
Fully implement Kad key index and run current code and sqlite code in parallel, comparing sent search responses.


