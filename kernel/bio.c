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

#define NBUCKET 13
#define HASH(x) ((x) % NBUCKET)

struct {
	struct spinlock lock[NBUCKET];
	struct buf buf[NBUF];
	struct buf head[NBUCKET];
} bcache;

void
binit(void) 
{
  struct buf *b;

  for (int i=0;i<NBUCKET;i++) {
    initlock(&bcache.lock[i], "bcache");
  }
  // Create linked list of buffers
  // bcache.head[0].prev = &bcache.head[0];
  bcache.head[0].next = &bcache.buf[0];
  for(b = bcache.buf; b < bcache.buf+NBUF-1; b++) {
    b->next = b+1;
    initsleeplock(&b->lock, "buffer");
  }
  initsleeplock(&b->lock, "buffer");
}

// 写入缓存块基本信息（用于重新分配空闲块）
void
write_cache(struct buf *take_buf, uint dev, uint blockno)
{
  take_buf->dev = dev;
  take_buf->blockno = blockno;
  take_buf->valid = 0;
  take_buf->refcnt = 1;
  take_buf->time = ticks;
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b, *last;
  struct buf *take_buf = 0;
  int id = HASH(blockno);
  acquire(&(bcache.lock[id]));

  // 在当前桶中查找缓存的块，同时顺便寻找空闲块，并记录链表尾部位置，用于后续插入新的buf
  b = bcache.head[id].next;
  last = &(bcache.head[id]);
  for(; b; b = b->next, last = last->next) {
    if(b->dev == dev && b->blockno == blockno) {
      // 命中缓存，更新访问时间与引用计数，返回该块
      b->time = ticks;
      b->refcnt++;
      release(&(bcache.lock[id]));
      acquiresleep(&b->lock);
      return b;
    }
    if(b->refcnt == 0) {
      // 记录当前桶中第一个可用空闲块
      take_buf = b;
    }
  }

  // 当前桶中没有命中缓存，但存在空闲块，直接使用
  if(take_buf) {
    write_cache(take_buf, dev, blockno);
    release(&(bcache.lock[id]));
    acquiresleep(&(take_buf->lock));
    return take_buf;
  }

  // 当前桶没有空闲块，从其他桶中查找最久未使用（LRU）的空闲块
  int lock_num = -1; // 记录已持有的桶锁编号
  uint64 time = __UINT64_MAX__;  // 记录最小时间（越小越旧）
  struct buf *tmp;
  struct buf *last_take = 0;
  for(int i = 0; i < NBUCKET; ++i) {
    if(i == id) continue;

    acquire(&(bcache.lock[i]));
    for(b = bcache.head[i].next, tmp = &(bcache.head[i]); b; b = b->next,tmp = tmp->next) {
      if(b->refcnt == 0) {
        if(b->time < time) {
          // 找到更“旧”的空闲块，更新候选块记录
          time = b->time;
          last_take = tmp;
          take_buf = b;
          // 若前一个桶锁仍持有且与当前桶不同，释放前一个桶的锁
          if(lock_num != -1 && lock_num != i && holding(&(bcache.lock[lock_num])))
            release(&(bcache.lock[lock_num]));
          lock_num = i;
        }
      }
    }
    // 当前桶未用作分配来源，立即释放其锁
    if(lock_num != i)
      release(&(bcache.lock[i]));
  }

  // 所有桶都没有空闲块，触发panic
  if (!take_buf)
    panic("bget: no buffers");

  // 将被选中的空闲块从其原桶链表中移除
  last_take->next = take_buf->next;
  take_buf->next = 0;
  release(&(bcache.lock[lock_num]));
  
  // 将该空闲块插入当前桶末尾，并更新其元信息
  b = last;
  b->next = take_buf;
  write_cache(take_buf, dev, blockno);


  release(&(bcache.lock[id]));
  acquiresleep(&(take_buf->lock));

  return take_buf;
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
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
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  int h = HASH(b->blockno);
  acquire(&bcache.lock[h]);
  b->refcnt--;
  release(&bcache.lock[h]);
}

void bpin(struct buf *b)
{
  int bucket_id = b->blockno % NBUCKET;
  acquire(&bcache.lock[bucket_id]);
  b->refcnt++;
  release(&bcache.lock[bucket_id]);
}

void bunpin(struct buf *b)
{
  int bucket_id = b->blockno % NBUCKET;
  acquire(&bcache.lock[bucket_id]);
  b->refcnt--;
  release(&bcache.lock[bucket_id]);
}