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
#define NBUCKETS 13

struct {
  struct spinlock lock[NBUCKETS];
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // head.next is most recently used.
  //struct buf head;
  struct buf hashbucket[NBUCKETS]; //每个哈希队列一个linked list及一个lock
} bcache;

void
binit(void)
{
  struct buf *b;
  for(int i=0;i<NBUCKETS;i++){
    initlock(&bcache.lock[i], "bcache");
    bcache.hashbucket[i].prev =&bcache.hashbucket[i];
    bcache.hashbucket[i].next =&bcache.hashbucket[i];
  }
  
  
  // Create linked list of buffers
  int j=0;
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = bcache.hashbucket[j%NBUCKETS].next;
    b->prev = &bcache.hashbucket[j%NBUCKETS];
    initsleeplock(&b->lock, "buffer");
    bcache.hashbucket[j%NBUCKETS].next->prev = b;
    bcache.hashbucket[j%NBUCKETS].next = b;
    j++;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  uint key = blockno % NBUCKETS;

  acquire(&bcache.lock[key]);

  // Is the block already cached?
  for(b = bcache.hashbucket[key].next; b != &bcache.hashbucket[key]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock[key]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached; recycle an unused buffer.
  for(b = bcache.hashbucket[key].prev; b != &bcache.hashbucket[key]; b = b->prev){
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.lock[key]);
      acquiresleep(&b->lock);
      return b;
    }
  }
  release(&bcache.lock[key]);
  //寻找未使用的block
  for(int i= 0;i<NBUCKETS;i++){
    acquire(&bcache.lock[i]);
    for(b = bcache.hashbucket[i].prev; b != &bcache.hashbucket[i]; b = b->prev){
      if(b->refcnt == 0) {
        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;
        //移除链表 
        b->next->prev=b->prev;
        b->prev->next=b->next;
        release(&bcache.lock[i]);
        //插入哈希值对应链表
        acquire(&bcache.lock[key]);
        b->next = bcache.hashbucket[key].next;
        b->prev = &bcache.hashbucket[key];
        bcache.hashbucket[key].next->prev = b;
        bcache.hashbucket[key].next = b;
        release(&bcache.lock[key]);

        acquiresleep(&b->lock);
        return b;
      }
    }
    release(&bcache.lock[i]);
  }
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b->dev, b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b->dev, b, 1);
}

// Release a locked buffer.
// Move to the head of the MRU list.
void
brelse(struct buf *b)
{
  uint key =b->blockno %NBUCKETS;
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  acquire(&bcache.lock[key]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.hashbucket[key].next;
    b->prev = &bcache.hashbucket[key];
    bcache.hashbucket[key].next->prev = b;
    bcache.hashbucket[key].next = b;
  }
  
  release(&bcache.lock[key]);
}

void
bpin(struct buf *b) {
  uint key =b->blockno %NBUCKETS;
  acquire(&bcache.lock[key]);
  b->refcnt++;
  release(&bcache.lock[key]);
}

void
bunpin(struct buf *b) {
  uint key =b->blockno %NBUCKETS;
  acquire(&bcache.lock[key]);
  b->refcnt--;
  release(&bcache.lock[key]);
}
