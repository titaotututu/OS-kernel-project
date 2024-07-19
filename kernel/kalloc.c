// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"
//buddy system parameter
#define MIN_BLOCK_SIZE 16  //16B
#define MAX_ORDER 20  //16*2^20=16MB=BUDDY_HEAP_SIZE
#define BUDDY_HEAP_SIZE (16*1024*1024)  //16MB

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.


struct run {   //free page 
  struct run *next;
};

//struct kmem
struct {
  struct spinlock lock;  //a lock
  struct run *freelist;  //free linklist_store all free page 
} kmem;

struct block{   //memory block 
  struct block *next;
  int order; //its order num
  int id;
};

struct buddy{   
  struct block *free_list[MAX_ORDER+1];  //store the linklist 
  char *base;   //the start area of heap
};

struct buddy buddy_system;  //create buddy system



//initial the buddy system 
void buddy_init(void *heap_start,void *heap_end){
  //CHECK 
  // if(heap_start==null|heap_end==0||heap_start>=heap_end){
  //   printf("Invalid heap range \n");
  //   return;
  // }
  buddy_system.base=(char *)heap_start;  //the start area of the heap 
  memset(buddy_system.free_list,0,sizeof(buddy_system.free_list));

  struct block *initial_block=(struct block *)buddy_system.base;  
  initial_block->next=0;
  buddy_system.free_list[MAX_ORDER]=initial_block;
  printf("Buddy System heap: from %p to %p\n",buddy_system.base,buddy_system.base+BUDDY_HEAP_SIZE);
  }

  //calculate the order of the block
int get_order(int size){
  int order =0;
  if(size<MIN_BLOCK_SIZE){
  size=MIN_BLOCK_SIZE;
  }
  while((1<<(order+4))<size){  //move left 4bit (not suit )
   order++; 
  }
  return order;
}


//allocate the memory ---------------------------------------------------------------------------wangyao 
void *
buddy_malloc(uint64 size){
printf("try to malloc the area\n");
int order=get_order(size+sizeof(struct block));  //the block is a space of pointer 
for (int current_order =order;current_order<=MAX_ORDER;current_order++){
  if(buddy_system.free_list[current_order]){
    //find the block and allocate it 
    struct block *block_to_alloc=buddy_system.free_list[current_order];
    buddy_system.free_list[current_order]=block_to_alloc->next;
    block_to_alloc->order=current_order;

    //if the current_order>order,we should cut the block to a smaller one
    while (current_order>order){
      current_order--;
      unsigned long block_address=(unsigned long) block_to_alloc;
      struct block *buddy_block =(struct block *)(block_address+(1<<(current_order+4)));
      //add the buddy_block to the free_list
      buddy_block->next =buddy_system.free_list[current_order];
      buddy_block->order=current_order;  //set the order
      block_to_alloc->order=current_order;
      buddy_system.free_list[current_order]=buddy_block;
    }
    void *my_ptr=(void *)((char*)block_to_alloc+sizeof(struct block ));
    return my_ptr;
  }

}
printf("fail to alloctae the area.\n");  //fail
return 0;
}

//--------------------------------------------------------------------merge wangyao 

void merge_block( struct block* free_block,int order){

//merge the block of the free_list
unsigned long free_addr=(unsigned long)free_block;
int merge=0; //flag 

//calculate the buddy_block of the free block 
//case `1:before
unsigned long buddy_addr=free_addr-(1<<(order+4));
if((char*)buddy_addr>=buddy_system.base){
  struct block *buddy_block =(struct block*)buddy_addr;

  //search for the buddy block in the free list 
  struct block **current=&buddy_system.free_list[order];  //current here means address
  struct block *prev=0;
  while (*current){
    if(*current==buddy_block&&buddy_block->order==order){
      //find the buddy_block and remove it
      if(prev){
        prev->next=buddy_block->next;
      }else {
        buddy_system.free_list[order]=buddy_block->next;
      }
      buddy_system.free_list[order]=free_block->next;

      //update 
      free_block =(struct block*)(free_addr<buddy_addr?free_addr:buddy_addr);
      order++;
      merge=1;
      break;
    }
    prev=*current;
    current=&(*current)->next;
  }

}
//case 2: buddy_block is behind the free_block 
if(!merge){
  buddy_addr=free_addr+(1<<(order+4));
if((char*)buddy_addr<=buddy_system.base+BUDDY_HEAP_SIZE){
  struct block *buddy_block =(struct block*)buddy_addr;

  //search for the buddy block in the free list 
  struct block **current=&buddy_system.free_list[order];
  struct block *prev=0;
  while (*current){
    if(*current==buddy_block&&buddy_block->order==order){
      //find the buddy_block and remove it
      if(prev){
        prev->next=buddy_block->next;
      }else {
        buddy_system.free_list[order]=buddy_block->next;
      }
      buddy_system.free_list[order]=free_block->next;

      //update 
      free_block =(struct block*)(free_addr<buddy_addr?free_addr:buddy_addr);
      order++;
      merge=1;
      break;
    }
    prev=*current;
    current=&(*current)->next;
  }
}

}

 if(merge){ //add the bigger free block to the free list 
    free_block->next=buddy_system.free_list[order];
    free_block->order=order;  //renew the order
    buddy_system.free_list[order]=free_block;
  }

  if(merge){
    merge_block(free_block,order);  //continue check_recursion
  }

}

//--------------------------------------------------------------------------------------wangyao
void 
buddy_free(void *ptr){
struct block *free_block =(struct block*)((char*)ptr-sizeof(struct block));
int order =free_block->order;
free_block->next=buddy_system.free_list[order];  //put into the free_list according to the order
buddy_system.free_list[order]=free_block;

merge_block(free_block,order);
}


//print the information of the free list-----------------wangyao
void show(){
  printf("the status of the heap:\n");
for (int i=0;i<=MAX_ORDER;i++){
struct block *current =buddy_system.free_list[i];
printf("Order %d :\n",i);
  while (current){
    printf("block address:%p\n",current);
    current=current->next;
  }
  
}}



void
kinit()    //init memory 
{
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)(PHYSTOP-BUDDY_HEAP_SIZE)); 
  buddy_init((void *)(PHYSTOP-BUDDY_HEAP_SIZE),(void *)PHYSTOP);   //16MB
  //test_malloc();
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);   //PGROUNDUP
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)  
    kfree(p);  //free page and add it to the free linklist 
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}





