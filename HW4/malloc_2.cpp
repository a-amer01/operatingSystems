#include <unistd.h>
#include <iostream>
#include <cstring>

//struct fileds
typedef struct MallocMetadata_t {
    size_t size;
    bool is_free;
    MallocMetadata_t *next;
    MallocMetadata_t *prev;
}* MallocMetadata;

MallocMetadata allocListHead = nullptr;
MallocMetadata allocListTail = nullptr;
//end of struct fields

//meta data statistics
size_t _num_meta_data_bytes(){
    return _num_allocated_blocks() * _size_meta_data();
}

size_t _size_meta_data(){
    return (sizeof(struct MallocMetadata_t));
}
size_t _num_free_blocks();
size_t _num_free_bytes();
size_t _num_allocated_blocks();
size_t _num_allocated_bytes();

void *smalloc(size_t size)
{
    if (size == 0 || size > 100000000)
    {
        return nullptr;
    }

    void *allocated_ptr;

    if (!allocListHead){
        allocListHead = (MallocMetadata) sbrk(_size_meta_data());
            if (allocListHead == (void*) -1){
                return nullptr;
            }
        allocListHead->size = size;
        allocListHead->prev = nullptr;
        allocListHead->next = nullptr;
        allocListHead->is_free = false;
        allocListTail = allocListHead;

        allocated_ptr = sbrk(size);
            if (allocated_ptr == (void *) -1) {
                return nullptr;
            }
        return allocated_ptr;
    }

    MallocMetadata iter;
        for (iter = allocListHead; iter; iter = iter->next){
            if (iter->size >= size && iter->is_free){
                break;
            }
        }

        if (iter){
            iter->is_free = false;
            return (void *) ((size_t) iter + _size_meta_data());
        }

    MallocMetadata BlockMetaData = (MallocMetadata) sbrk(_size_meta_data());
        if (BlockMetaData == (void *) -1){
            return nullptr;
        }
    BlockMetaData->next = nullptr;
    BlockMetaData->is_free = false;
    BlockMetaData->prev = allocListTail;
    allocListTail->next = BlockMetaData;
    BlockMetaData->size = size;

    allocListTail = BlockMetaData;

    allocated_ptr = sbrk(size);
        if (allocated_ptr == (void *) -1){
            return nullptr;
        }
    return allocated_ptr;

}

void *scalloc(size_t num, size_t size)
{
        if (size == 0 || num * size > 100000000){
            return nullptr;
        }

    void *allocated_ptr;
    allocated_ptr = smalloc(num * size);
        if (!allocated_ptr){
            return nullptr;
        }
    std::memset(allocated_ptr, 0, num * size);
    return allocated_ptr;
}

void sfree(void *oldp)
{
        if (oldp == nullptr){
            return;
        }
    MallocMetadata metadata_ptr = (MallocMetadata) ((size_t) oldp - _size_meta_data());
        if (metadata_ptr->is_free){
            return;
        }
    metadata_ptr->is_free = true;
}

void *srealloc(void *oldp, size_t size)
{
        if (size == 0 || size > 100000000){
            return nullptr;
        }

    void *allocated_ptr;
        if (!oldp) {
            return smalloc(size);
        }

    MallocMetadata old_metadata = (MallocMetadata) ((size_t) oldp - _size_meta_data());

        if (size <= old_metadata->size){
            old_metadata->is_free = false;
            return oldp;
        }

    allocated_ptr = smalloc(size);
        if (!allocated_ptr){
            return nullptr;
        }

    std::memmove(allocated_ptr, oldp, old_metadata->size); 
    sfree(oldp);
    return allocated_ptr;
}

//free blocks statistics
size_t _num_free_blocks() {
    size_t freedBlocks_num = 0;
    for (MallocMetadata iter = allocListHead; iter; iter = iter->next) {
        if (iter->is_free) freedBlocks_num++;
    }
    return freedBlocks_num;
}

size_t _num_free_bytes() {
    size_t totalFree_bytes = 0;
        for (MallocMetadata iter = allocListHead; iter; iter = iter->next) {
            if (iter->is_free) totalFree_bytes += iter->size;
        }
    return totalFree_bytes;
}

//alloced blocks statistics
size_t _num_allocated_blocks() {
    size_t allocatedBlocks_num = 0;
        for (MallocMetadata iter = allocListHead; iter; iter = iter->next) {
            allocatedBlocks_num++;
        }
    return allocatedBlocks_num;
}

size_t _num_allocated_bytes()
{
    size_t totalAlloced_bytes = 0;
        for (MallocMetadata iter = allocListHead; iter; iter = iter->next){
            totalAlloced_bytes += iter->size;
        }
    return totalAlloced_bytes;
}





