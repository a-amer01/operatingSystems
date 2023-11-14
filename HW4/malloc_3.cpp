#include <unistd.h>
#include <iostream>
#include <cstring>
#include <cassert>
#include <sys/mman.h>
#include <cmath>

#define META_SIZE (sizeof(struct block_metadata_t))
int random_cookie = rand();

typedef struct block_metadata_t{
    int cookie = random_cookie;
    bool is_free = true;
    bool big_block = false;
    size_t used_size;
    int size_order;
    block_metadata_t* next_malloc = nullptr;
    block_metadata_t* prev_malloc = nullptr;
    block_metadata_t* next_free = nullptr;
    block_metadata_t* prev_free = nullptr;

}* BlockMetaData;

int big_block_counter = 0;
int big_block_size = 0;
bool initialized = false;



BlockMetaData alloc_list_head = nullptr;
BlockMetaData alloc_list_tail = nullptr;
BlockMetaData free_hist[11] = {nullptr};

void _check_for_overflow();
void _remove_from_malloc_list(BlockMetaData to_remove);
BlockMetaData _remove_from_free_list(int min_wanted_order);
void initializeHeap();
int findMinPowOfTwo(int size);
int findMinAvailableOrder(int min_block_size);
void separate_block(int min_block_size, int size);
void _move_To_New_List(BlockMetaData block);
BlockMetaData _remove_from_free_list2(int min_wanted_order);
bool emptyFreeHist(int size);


///////////////////////////////////////////////// malloc implementation ////////////////////////////////////////////////
void *smalloc(size_t size){
    if(!initialized) initializeHeap();
    if(size < 1 || size > 100000000) return nullptr;
    _check_for_overflow();

    int min_block_size = findMinPowOfTwo(size + META_SIZE) - 7;

    if(min_block_size > 10){ // blocks bigger than
        BlockMetaData new_block = (BlockMetaData)mmap(nullptr, size + META_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        new_block->big_block = true;
        new_block->used_size = size;
        big_block_counter++;
        big_block_size += size;
        return (void*)(new_block+1);
    }

    if(emptyFreeHist(min_block_size)){
        return nullptr;
    }

    separate_block(min_block_size,size);

    BlockMetaData new_block = _remove_from_free_list(min_block_size);
    if (!new_block) return nullptr;
    new_block->is_free = false;
    return((void*)(new_block+1));//should point to right after the metadata

}

void initializeHeap(){
    initialized = true;
    auto curr_sbrk = (unsigned long)sbrk(0);
    unsigned long miss_allign = curr_sbrk & (4194303);
    auto adjest = 4194304 - miss_allign;

    sbrk(adjest);
    auto heap_base = (unsigned long)sbrk(4194304);
    auto first_metadata = (BlockMetaData)heap_base;
    first_metadata->size_order = 10;
    first_metadata->cookie = random_cookie;
    first_metadata->is_free = true;
    first_metadata->used_size = 0;
    free_hist[10] = first_metadata;

    char* z = (char*)first_metadata;

    for (int i = 1; i < 32; i++) {
        auto w = (BlockMetaData)z;
        z+=(128*1024);

        w->next_free = (BlockMetaData) z;
        w->next_malloc = nullptr;
        w->prev_malloc = nullptr;

        ((BlockMetaData)z)->prev_free = (BlockMetaData) w;
        ((BlockMetaData)z)->next_free = nullptr;
        ((BlockMetaData)z)->next_malloc = nullptr;
        ((BlockMetaData)z)->prev_malloc = nullptr;
        ((BlockMetaData)z)->big_block = false;
        ((BlockMetaData)z)->is_free = true;
        ((BlockMetaData)z)->cookie = random_cookie;
        ((BlockMetaData)z)->size_order = 10;
        ((BlockMetaData)z)->used_size = 0;
    }
}


void separate_block(int min_block_size, int size){
    int min_available_order = min_block_size;
    if(free_hist[min_available_order]){
        return;
    }

    for (; min_available_order < 11; min_available_order++) {
        if (free_hist[min_available_order]) break;
    }

    if (min_available_order == 11) return;

    BlockMetaData block = _remove_from_free_list2(min_available_order);

    while(block->size_order > min_block_size){
        unsigned long new_block_addr;
        if(block->size_order > 1){
            new_block_addr = ((unsigned long)block)+pow(2,block->size_order-1)*128;
        }else{
            new_block_addr = ((unsigned long)block)+128;
        }
        BlockMetaData new_block = (BlockMetaData)new_block_addr;
        new_block->is_free =true;
        new_block->size_order = block->size_order-1;
        new_block->cookie = random_cookie;
        new_block->used_size = 0;
        if(new_block->size_order > 10){
            new_block->big_block = true;
        }
        _move_To_New_List(new_block);
        block->size_order--;
    }
    block->used_size = size;
    _move_To_New_List(block);
}

////////////////////////////////////////////////// free implementation /////////////////////////////////////////////////
bool isBuddy(unsigned long addr , int size_order);
BlockMetaData _unite_Buddies(BlockMetaData block);

void sfree(void* ptr) {
    _check_for_overflow();
    if(!ptr) return;
    auto to_free = (BlockMetaData) ((size_t)ptr - META_SIZE);
    if (!to_free || to_free->is_free) return;

    if (to_free->big_block) {
        big_block_size -= to_free->used_size;
        munmap(ptr, to_free->used_size + META_SIZE);
        big_block_counter--;
        return;
    }

    _remove_from_malloc_list(to_free);
    _move_To_New_List(_unite_Buddies(to_free));
}

void _move_To_New_List(BlockMetaData block) {
    if (block->prev_free) block->prev_free->next_free = block->next_free;
    if (block->next_free) block->next_free->prev_free = block->prev_free;
    block->next_free = nullptr;
    block->prev_free = nullptr;

    if (!free_hist[block->size_order]) {
        free_hist[block->size_order] = block;
        return;
    }

    BlockMetaData ptr = nullptr, prev = free_hist[block->size_order];
    if (prev) ptr = prev->next_free;

    while (ptr){
        if((unsigned long)ptr > (unsigned long)block){
            block->next_free = ptr;
            block->prev_free = prev;
            prev->next_free = block;
            ptr->prev_free = block;
            return;
        }
        ptr = ptr->next_free;
        prev = prev->next_free;
    }

    if ((unsigned long)prev > (unsigned long)block) {
        free_hist[block->size_order] = block;
        block->next_free = prev;
        prev->prev_free = block;
    }
    else {
        prev->next_free = block;
        block->prev_free = prev;
    }
}

bool isBuddy(unsigned long addr , int size_order) {
    BlockMetaData list = free_hist[size_order];
    BlockMetaData iter = list;

    while (iter){
        if((unsigned long)iter == addr){
            return true;
        }
        iter = iter->next_free;
    }
    return false;
}


void _remove_buddy_from_free_list(unsigned long buddy_addr , int size_order)
{
    BlockMetaData iter = free_hist[size_order];
    BlockMetaData prev = iter;
    BlockMetaData block = (BlockMetaData)buddy_addr;
        if(prev == block){
            _remove_from_free_list2(size_order);
            return;
        }

    iter = iter->next_free;
        while (iter){
            if((unsigned long)iter == buddy_addr){
                prev->next_free = block->next_free;
                block->next_free->prev_free = iter;
                block->next_free = nullptr;
                block->prev_free = nullptr;
                return;
            }
            prev = iter;
            iter = iter->next_free;
        }
    return;//should not get to this line

}

BlockMetaData _unite_Buddies(BlockMetaData block){
    while(block->size_order<10){
        auto addr =(unsigned long) block;
        unsigned long buddy_addr = addr ^((unsigned long)(pow(2,block->size_order)*128));
        if(isBuddy(buddy_addr,block->size_order)){
            _remove_buddy_from_free_list(buddy_addr,block->size_order);
            if(buddy_addr <(unsigned long)block){
                block =(BlockMetaData)buddy_addr;
            }
            block->size_order++;
        }else{
            break;
        }
    }

    return block;
}

///////////////////////////////////////////// realloc implementation ////////////////////////////////////////////////////
int _check_Buddies(BlockMetaData block ,int size);
BlockMetaData srealloc_unite_buddies(BlockMetaData block,int s_order);
void  srealloc_remove_from_free_list(BlockMetaData to_remove);
bool buddyFitExists(BlockMetaData block, size_t size);

void* srealloc(void *oldp, size_t size){
    _check_for_overflow();
    if(size <= ((BlockMetaData)((size_t)oldp - META_SIZE))->used_size) return oldp;


    if (!oldp) return smalloc(size);
    BlockMetaData block = (BlockMetaData)oldp;
    block--;

    if(size + META_SIZE > 100000000) return nullptr;


    if(META_SIZE + size > 128*pow(2,10)) {
        void* new_block = smalloc(size);
        std::memmove(new_block,block,block->used_size);
        sfree(oldp);
        return new_block;
    }

    if(META_SIZE + size > pow(2,block->size_order)*128){
        if(buddyFitExists(block,size)){
            _remove_from_malloc_list(block);
            BlockMetaData to_return = (srealloc_unite_buddies(block,_check_Buddies(block,size)));
            srealloc_remove_from_free_list(to_return);
            to_return->used_size = pow(2, to_return->size_order) * 128;
//            if (!alloc_list_head){
//                alloc_list_head = to_return;
//                alloc_list_tail = to_return;
//            }
//            else {
//                alloc_list_tail->next_malloc = to_return;
//                to_return->prev_malloc = alloc_list_tail;
//                alloc_list_tail = to_return;
//            }
            return (void*)(to_return+1);
        }
        void* new_block = smalloc(block->used_size  + size);
        std::memmove(new_block,block,block->used_size);
        sfree(oldp);
        return new_block;
    }else{
        return oldp;
    }

    return nullptr;
}

int _check_Buddies(BlockMetaData block ,int size){
    int achieved_order = block->size_order;
    while(block->size_order < 10  && pow(2,achieved_order)*128 < META_SIZE + size) {
        auto addr =(unsigned long) block;
        unsigned long buddy_addr = addr ^((unsigned long)(pow(2,block->size_order)*128));
        if(isBuddy(buddy_addr,block->size_order)){
            if(buddy_addr < (unsigned long)block){
                block =(BlockMetaData)buddy_addr;
            }
            achieved_order++;
        }else{
            break;
        }
    }

    if(pow(2,achieved_order)*128 >= META_SIZE + size){
        return achieved_order;
    }else{
        return 0;
    }
}


bool buddyFitExists(BlockMetaData block, size_t size) {
    size_t achieved_size = pow(2, block->size_order)*128;
    while(block->size_order < 10  && achieved_size < META_SIZE + size) {
        auto addr = (unsigned long) block;
        unsigned long buddy_addr = addr ^((unsigned long)(pow(2,block->size_order)*128));
        if(isBuddy(buddy_addr,block->size_order)){
            if(buddy_addr < (unsigned long)block){
                block =(BlockMetaData)buddy_addr;
            }
            achieved_size *= 2;
        }else{
            break;
        }
    }

    if(achieved_size >= META_SIZE + size){
        return true;
    }else{
        return false;
    }
}


BlockMetaData srealloc_unite_buddies(BlockMetaData block, int s_order){
    while(block->size_order <= s_order){
        auto addr =(unsigned long) block;
        unsigned long buddy_addr = addr ^((unsigned long)(pow(2,block->size_order)*128));
        if(isBuddy(buddy_addr,block->size_order)){
            _remove_buddy_from_free_list(buddy_addr,block->size_order);
            if(buddy_addr <(unsigned long)block){
                block =(BlockMetaData)buddy_addr;
            }
            block->size_order++;
        }
    }
    return block;
}

void srealloc_remove_from_free_list(BlockMetaData to_remove)
{
    if (!free_hist[to_remove->size_order]) return;
    BlockMetaData iter = free_hist[to_remove->size_order];
    BlockMetaData prev = iter;

    if(prev == to_remove){
        _remove_from_free_list2(to_remove->size_order);
        return;
    }

    iter = iter->next_free;

    while (iter){
        if(iter == to_remove){
              prev->next_free = to_remove->next_free;
            if(iter->next_free != nullptr){
            to_remove->next_free->prev_free = iter;
            }

            to_remove->next_free = nullptr;
            to_remove->prev_free = nullptr;
            return; /*block;*/
        }
        prev = iter;
        iter = iter->next_free;
    }

    if(alloc_list_head == nullptr){
        alloc_list_head = to_remove;
        alloc_list_tail = to_remove;
    }else{
        alloc_list_tail->next_malloc = to_remove;
        to_remove->prev_malloc = alloc_list_tail;
        alloc_list_tail = to_remove;
    }

    return;// block;//should not get to this line

}
/////////////////////////////////////////////scalloc implementation/////////////////////////////////////////////////////
void* scalloc(size_t num, size_t size) {
    _check_for_overflow();
    void* allocated_block = smalloc(num * size);
    if (!allocated_block) return nullptr;
    memset(allocated_block, 0, num * size);
    return allocated_block;
}

////////////////////////////////////////////// heap status function ////////////////////////////////////////////////////
size_t _num_free_blocks() {
    size_t free_blocks_num = 0;
    for (int i = 0; i < 11; i++) {
        BlockMetaData free_list_ptr = free_hist[i];
        while (free_list_ptr) {
            free_blocks_num++;
            free_list_ptr = free_list_ptr->next_free;
        }
    }
    return free_blocks_num;
}

size_t _num_free_bytes() {
    size_t free_bytes_num = 0;
    for (int i = 0; i < 11; i++) {
        BlockMetaData free_list_ptr = free_hist[i];
        while (free_list_ptr) {
            free_bytes_num += ((pow(2,free_list_ptr->size_order)*128)- META_SIZE);
            free_list_ptr = free_list_ptr->next_free;
        }
    }
    return free_bytes_num;
}

size_t _num_allocated_blocks() {
    size_t num_allocated_blocks = big_block_counter + _num_free_blocks();
    for (BlockMetaData it = alloc_list_head; it; it = it->next_malloc) {
        num_allocated_blocks++;
    }
    return num_allocated_blocks;
}

size_t _num_allocated_bytes() {
    size_t allocated_bytes_num = big_block_size + _num_free_bytes();
    for (BlockMetaData it = alloc_list_head; it; it = it->next_malloc) {
        allocated_bytes_num += ((pow(2,it->size_order)*128)- META_SIZE);
    }
    return allocated_bytes_num;
}


size_t _num_meta_data_bytes() {
    return _num_allocated_blocks() * META_SIZE;
}

size_t _size_meta_data() {
    return (sizeof(struct block_metadata_t));
}

////////////////////////////////////////////////////helper functions////////////////////////////////////////////////////

void _check_for_overflow() {
    for (BlockMetaData iter = alloc_list_head; iter; iter = iter->next_malloc) {
        if (iter->cookie != random_cookie){
            exit(0xdeadbeef);
        }
    }

    for (int index = 0; index<10;index++){
        for (BlockMetaData iter = free_hist[index]; iter; iter = iter->next_free){
            if (iter->cookie != random_cookie){
                exit(0xdeadbeef);
            }
        }
    }
}


BlockMetaData _remove_from_free_list(int min_wanted_order) {
    int min_available = findMinAvailableOrder(min_wanted_order);
    if(min_available == -1) return nullptr;
    BlockMetaData to_allocate = free_hist[min_available];

    if (!to_allocate) return nullptr;

    free_hist[min_available] = to_allocate->next_free;
    if(free_hist[min_available])
        free_hist[min_available]->prev_free = nullptr;
    to_allocate->prev_free = nullptr;
    to_allocate->next_free = nullptr;
    to_allocate->is_free =false;


    if(alloc_list_head == nullptr){
        alloc_list_head = to_allocate;
        alloc_list_tail = to_allocate;
    }else{
        alloc_list_tail->next_malloc = to_allocate;
        to_allocate->prev_malloc = alloc_list_tail;
        alloc_list_tail = to_allocate;
    }
    return to_allocate;
}

BlockMetaData _remove_from_free_list2(int min_wanted_order) {
    int min_available = findMinAvailableOrder(min_wanted_order);
    if(min_available == -1) return nullptr;
    BlockMetaData to_allocate = free_hist[min_available];

    if (!to_allocate) return nullptr;

    free_hist[min_available] = to_allocate->next_free;
    if(free_hist[min_available])
        free_hist[min_available]->prev_free = nullptr;
    to_allocate->prev_free = nullptr;
    to_allocate->next_free = nullptr;
    to_allocate->is_free =false;
    return to_allocate;
}

void _remove_from_malloc_list(BlockMetaData to_remove){
    to_remove->is_free = true;
    to_remove->used_size =0;

    if(to_remove->prev_malloc == nullptr){
        alloc_list_head = to_remove->next_malloc;
        if(alloc_list_head == nullptr){//only one node
            alloc_list_tail = nullptr;
            return;
        }
        to_remove->next_malloc->prev_malloc =nullptr;
        to_remove->next_malloc = nullptr;
        to_remove->prev_malloc = nullptr;
        return;
    }

    if(to_remove->next_malloc == nullptr){
        alloc_list_tail = to_remove->prev_malloc;
        if(alloc_list_tail == nullptr){//only one node
            alloc_list_head = nullptr;
            return;
        }
        to_remove->prev_malloc->next_malloc = nullptr;
        to_remove->next_malloc = nullptr;
        to_remove->prev_malloc = nullptr;
        return;
    }

    to_remove->prev_malloc->next_malloc = to_remove->next_malloc;
    to_remove->next_malloc->prev_malloc = to_remove->prev_malloc;
    to_remove->next_malloc = nullptr;
    to_remove->prev_malloc = nullptr;
}

int findMinPowOfTwo(int size) {
    if (size == 0) return 0;
    int min_power_of_two = 0, temp_size = size;

    while (temp_size != 0) {
        min_power_of_two++;
        temp_size = temp_size>>1;
    }

    if (pow(2,min_power_of_two) < size) min_power_of_two++;

    //added the case if the block request lower than 128bytes
    if(min_power_of_two < 7){
        min_power_of_two = 7;
    }
    return min_power_of_two;
}

int findMinAvailableOrder(int min_block_size) {
    int order = min_block_size ;
    for (; order < 11; order++) {
        if (free_hist[order]) return order;
    }
    return -1;
}

bool emptyFreeHist(int size)
{
    for (int i = size; i < 11; i++)
    {
        if(free_hist[i]){
            return false;
        }
    }

    return true;
}