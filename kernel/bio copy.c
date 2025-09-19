// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.

#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"
#define LOCK_NUM 101
#define LOCK_NAME_LEN 11
static char bcache_locknames[LOCK_NUM][LOCK_NAME_LEN];
struct
{
  struct spinlock lock;
  struct buf buf[NBUF];
  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head;
} bcache;

static struct bucket buckets[LOCK_NUM]; // 现在用这个了
void binit(void)
{
  struct buf *b;
  for (int i = 0; i < LOCK_NUM; i++)
  {
    snprintf(bcache_locknames[i], LOCK_NAME_LEN, "bcache%d", i);
    initlock(&buckets[i].lock, bcache_locknames[i]);
  }
  initlock(&bcache.lock, "global_bcache");
  for (int i = 0; i < LOCK_NUM; i++)
  {
    buckets[i].head.prev = &buckets[i].head;
    buckets[i].head.next = &buckets[i].head;
  }
  bcache.head.prev = &bcache.head;
  bcache.head.next = &bcache.head;
  for (b = bcache.buf; b < bcache.buf + NBUF; b++)
  {
    b->bucket=0;
    b->hnext=0;
    b->hprev=0;
    b->next = bcache.head.next;
    b->prev = &bcache.head;
    initsleeplock(&b->lock, "buffer");
    bcache.head.next->prev = b;
    bcache.head.next = b;
  }
  // Create linked list of buffers
  // for(b = bcache.buf; b < bcache.buf+NBUF; b++){
  //   b->next = bcache.head.next;
  //   b->prev = &bcache.head;
  //   initsleeplock(&b->lock, "buffer");
  //   bcache.head.next->prev = b;
  //   bcache.head.next = b;
  // }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf *
bget(uint dev, uint blockno)
{
  struct buf *b;
  int i = blockno % LOCK_NUM;
  acquire(&buckets[i].lock);
  for (b = buckets[i].head.next; b != &buckets[i].head; b = b->next)
  {
    if (b->dev == dev && b->blockno == blockno)
    {
      b->refcnt++;
      release(&buckets[i].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  //release(&buckets[i].lock);

  // 获取全局的
  acquire(&bcache.lock);

  for (b = bcache.head.prev; b != &bcache.head; b = b->prev)
  {
    if (b->refcnt == 0)
    {
      //re
      if(b->bucket==0){
        b->bucket=&buckets[i];
        b->hnext=buckets[i].head.next;
        b->hprev=buckets[i].head.prev;
      }
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.lock);
      release(&buckets[i].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf *
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if (!b->valid)
  {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void bwrite(struct buf *b)
{
  if (!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void brelse(struct buf *b)
{
  if (!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);
  if(b->bucket==0){
    panic("bucket");
  }
  int idx=b->blockno%LOCK_NUM;
  acquire(&buckets[idx].lock);

  acquire(&bcache.lock);
  b->refcnt--;
  if (b->refcnt == 0)
  { 
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    if (b->bucket != 0) {
      b->hnext->prev = b->hprev;
      b->hprev->next = b->hnext;
    }
    
    b->next = bcache.head.next;
    b->prev = &bcache.head;
    //清空
    b->hnext = 0;
    b->hprev = 0;
    b->bucket=0;
    bcache.head.next->prev = b;
    bcache.head.next = b;
  } 
  release(&bcache.lock);
  release(&buckets[idx].lock);
}

void bpin(struct buf *b)
{
  acquire(&bcache.lock);
  b->refcnt++;
  release(&bcache.lock);
}

void bunpin(struct buf *b)
{
  acquire(&bcache.lock);
  b->refcnt--;
  release(&bcache.lock);
}
