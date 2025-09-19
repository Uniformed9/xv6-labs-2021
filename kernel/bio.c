

#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"
#define NBUCKET 13

struct bcache_bucket
{
  struct spinlock lock;
  struct buf *head; // 桶链表头
};
struct
{
  struct bcache_bucket bucket[NBUCKET];
  struct buf buf[NBUF];
} bcache;
static inline int
hash(uint dev, uint blockno)
{
  return (dev + blockno) % NBUCKET;
}

void binit(void)
{
  struct buf *b;
  int i;
  // Create linked list of buffers
  for (i = 0; i < NBUCKET; i++)
  {
    initlock(&bcache.bucket[i].lock, "bcache.bucket");
    bcache.bucket[i].head = 0;
  }
  for (i = 0, b = bcache.buf; i < NBUF; i++, b++)
  {
    initsleeplock(&b->lock, "buffer");
    b->next = bcache.bucket[0].head;
    b->timestamp = 0;
    bcache.bucket[0].head = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf *
bget(uint dev, uint blockno)
{
  struct buf *b;
  int bi = hash(dev, blockno);

  // 1. 查找目标桶
  acquire(&bcache.bucket[bi].lock);
  for (b = bcache.bucket[bi].head; b; b = b->next)
  {
    if (b->dev == dev && b->blockno == blockno)
    {
      b->refcnt++;
      release(&bcache.bucket[bi].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  release(&bcache.bucket[bi].lock);

  // 2. 没命中，选择一个空闲 buf（全局扫描，找 timestamp 最小）
  struct buf *victim = 0;
  uint oldest = 0xffffffff;
  int victim_bucket = -1;
  int victim_last = -1;

  //printf("---------------------------------\n");
  for (int i = 0; i < NBUCKET; i++)
  {
    //printf("acquire%d\n", i);
    acquire(&bcache.bucket[i].lock);
    int flag = 0;
    for (b = bcache.bucket[i].head; b; b = b->next)
    {
      if (b->refcnt == 0 && b->timestamp < oldest)
      {
       
        //printf("here%d\n",i);
        //并不是每次都更新啊只要更新一次就好了
        if(!flag){
          victim_last = victim_bucket;
        }
        oldest = b->timestamp;
        victim = b;
        victim_bucket = i;
        flag = 1;
      }
    }
    //printf("victim_last%d\n", victim_last);
    if (flag)
    {
      if (victim_last != -1)
      {
       
        //printf("release%d\n", victim_last);
        release(&bcache.bucket[victim_last].lock);
         victim_last = -1;
      }
    }
    else
    {
      release(&bcache.bucket[i].lock);
    }
  }
  //printf("-------\n");
  if (victim == 0)
    panic("bget: no buffers");

  // 3. 把 victim 从原桶移到目标桶

  // 从原桶删除 victim
  struct buf **pp = &bcache.bucket[victim_bucket].head;
  while (*pp && *pp != victim)
    pp = &(*pp)->next;
  if (*pp == 0)
    panic("bget: victim not found");
  *pp = victim->next;
  //printf("bio.c:lock_victim_bucket:%d\n", victim_bucket);
  release(&bcache.bucket[victim_bucket].lock);

  // 加入新桶
  //printf("bio.c:lock:%d\n", bi);
  acquire(&bcache.bucket[bi].lock);
  victim->next = bcache.bucket[bi].head;
  bcache.bucket[bi].head = victim;
  victim->dev = dev;
  victim->blockno = blockno;
  victim->valid = 0;
  victim->refcnt = 1;
  victim->timestamp = ticks; // 初始化时间戳
  release(&bcache.bucket[bi].lock);

  acquiresleep(&victim->lock);
  return victim;
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

  int bi = hash(b->dev, b->blockno);
  acquire(&bcache.bucket[bi].lock);
  b->refcnt--;
  if (b->refcnt == 0)
    b->timestamp = ticks; // 记录最后释放时间
  release(&bcache.bucket[bi].lock);
}

void bpin(struct buf *b)
{
  int bi = hash(b->dev, b->blockno);
  acquire(&bcache.bucket[bi].lock);
  b->refcnt++;
  release(&bcache.bucket[bi].lock);
}

void bunpin(struct buf *b)
{
  int bi = hash(b->dev, b->blockno);
  acquire(&bcache.bucket[bi].lock);
  b->refcnt--;
  release(&bcache.bucket[bi].lock);
}