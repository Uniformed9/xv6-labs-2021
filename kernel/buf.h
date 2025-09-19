
// struct buf {
//   int valid;   // has data been read from disk?
//   int disk;    // does disk "own" buf?
//   uint dev;
//   uint blockno;
//   struct sleeplock lock;
//   uint refcnt;
//   struct buf *prev; // LRU cache list
//   struct buf *next;
//   struct buf *hprev; // LRU cache list
//   struct buf *hnext;
//   struct bucket *bucket;
//   uchar data[BSIZE];
// };
// struct bucket
// {
//   struct spinlock lock;
//   struct buf head; // 该桶的双向链表头
// };

struct buf {
  int valid;   // has data been read from disk?
  int disk;    // does disk "own" buf?
  uint dev;
  uint blockno;
  struct sleeplock lock;
  uint refcnt;
  struct buf *prev; // LRU cache list
  struct buf *next;
  uint timestamp;
  uchar data[BSIZE];
};


