// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run
{
  struct run *next;
};

#ifdef LAB_LOCK
struct
{
  struct spinlock lock;
  struct run *freelist;
} kmem[NCPU];
#else
struct
{
  struct spinlock lock;
  struct run *freelist;
} kmem;

#endif
void kinit()
{
  int cpu = cpuid();
#ifdef LAB_LOCK

  char lockname[16];
  snprintf(lockname, sizeof(lockname), "kmem%d", cpu);
  initlock(&kmem[cpu].lock, "kmem3");
#else
  initlock(&kmem.lock, "kmem");

#endif

#ifdef LAB_LOCK
  char *start, *stop;
  uint64 mem_per_cpu = ((uint64)PHYSTOP - (uint64)end) / 3;
  if (cpu == 0)
  {
    start = (char *)end;
    stop = start + mem_per_cpu;
  }
  else if (cpu == 1)
  {
    start = (char *)end + mem_per_cpu;
    stop = start + mem_per_cpu;
  }
  else
  { // id == 2
    start = (char *)end + 2 * mem_per_cpu;
    stop = (char *)PHYSTOP;
  }
  freerange((void *)start, (void *)stop);
#else
  freerange(end, (void *)PHYSTOP);
#endif
}

void freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char *)PGROUNDUP((uint64)pa_start);
  for (; p + PGSIZE <= (char *)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void kfree(void *pa)
{
  struct run *r;
  int cpu;
  if (((uint64)pa % PGSIZE) != 0 || (char *)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run *)pa;
#ifdef LAB_LOCK
  push_off();
  cpu = cpuid();
  pop_off();
  acquire(&kmem[cpu].lock);
  r->next = kmem[cpu].freelist;
  kmem[cpu].freelist = r;
  release(&kmem[cpu].lock);
#else
  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
#endif
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;
  int cpu;
#ifdef LAB_LOCK
  push_off();
  cpu = cpuid();
  pop_off();
  int flag = 0;
  for (int i = 0; i < 3; i++)
  {
    acquire(&kmem[(cpu+i)%3].lock);
    r = kmem[(cpu+i)%3].freelist;
    if (r){
      kmem[(cpu+i)%3].freelist = r->next;
      flag=1;
    }
    release(&kmem[(cpu+i)%3].lock);
    if(flag)break;
  }

#else
  acquire(&kmem.lock);
  r = kmem.freelist;
  if (r)
    kmem.freelist = r->next;
  release(&kmem.lock);
#endif

  if (r)
    memset((char *)r, 5, PGSIZE); // fill with junk
  return (void *)r;
}
