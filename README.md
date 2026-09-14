B+Tree Key-Value Storage Engine

An in-memory key-value databse built on B+tree index (order M), using Write-Ahead Log (WAL) to ensure data durability across restarts and WAL compaction to ensure WAL stays in limit.

Allowed operations:

PUT - add key , value to DB
GET - get value of given key
DEL - Delete key-value pair, given key 
SCAN - equivalent of select *

COMPACT - is to compress WAL to something, we can work with. 
i.e. In short, ensures that WAL had only content/inserts of key values actually in index.


