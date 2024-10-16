//
// Created by matthieu on 10/10/24.
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MAGIC_NUMBER 0x0123456789ABCDEFL

typedef struct HEADER_TAG {
    struct HEADER_TAG * ptr_next;
    size_t bloc_size;
    long magic_number;
} HEADER;

HEADER * header_free_list = NULL;

void split_block(HEADER* block, ssize_t wanted_size) {
    HEADER * first_block = block;
    HEADER* second_block= (void*)(block + 1) + wanted_size + sizeof(HEADER);
    second_block->ptr_next = first_block->ptr_next;
    second_block->bloc_size = first_block->bloc_size - wanted_size - sizeof(HEADER)-sizeof(long);
    second_block->magic_number = MAGIC_NUMBER;
    long* second_end_magic_number = (long*)(second_block+1)+second_block->bloc_size;
    *second_end_magic_number = MAGIC_NUMBER;

    first_block->bloc_size = wanted_size;
    first_block->ptr_next = second_block;
    long* first_end_magic_number = (long*)(first_block+1)+first_block->bloc_size;
    *first_end_magic_number = MAGIC_NUMBER;
}

HEADER* find_block_of_size(ssize_t size)
{
    HEADER* current = header_free_list;
    HEADER* found = NULL;

    if (current != NULL && current->bloc_size >= size){
        if (current->bloc_size > size){
            split_block(current, size);
        }
        if (current->bloc_size == size){
            found = current;
            header_free_list = current->ptr_next;
            return found;
        }
    }
    while (current != NULL){
        if (current->ptr_next->bloc_size >= size){
            //printf("existant block found\n");
            if (current->ptr_next->bloc_size > size){
                split_block(current->ptr_next, size);
            }
            if (current->ptr_next->bloc_size == size){
                found = current->ptr_next;
                current->ptr_next = current->ptr_next->ptr_next;
                return found;
            }
        }
        current = current->ptr_next;
    }
    return NULL;
}
void * malloc_3is(ssize_t size) {
    HEADER * new_block = find_block_of_size(size);
    if (new_block == NULL) {
        new_block = sbrk(size + sizeof(HEADER) + sizeof(long));
        if (new_block == (void*)-1) {
            return NULL;
        }
        new_block->ptr_next = NULL;
        new_block->bloc_size = size;
        new_block->magic_number = MAGIC_NUMBER;
        long * end_magic_number = (long *)(new_block+1)+size;
        *end_magic_number = MAGIC_NUMBER;
    }

    return new_block+1;
}

void displayError() {
    printf("Memory overflow detected\n");
}

void free_3is(void * block) {
    if (block == NULL) {
        return;
    }
    HEADER * block_to_free = block - sizeof(HEADER);
    ssize_t block_size = block_to_free->bloc_size;
    long * end_magic_number = (long *)block+block_size;

    if ((block_to_free->magic_number != MAGIC_NUMBER) || (*end_magic_number != MAGIC_NUMBER)) {
        displayError();
        return;
    }
    block_to_free->ptr_next = header_free_list;
    header_free_list = block_to_free;
}

void print_linked_list() {
    if (header_free_list == NULL) {
        printf("NULL\n");
        return;
    }
    HEADER* current = header_free_list;
    printf("<%p>, %lu -> ", current, current->bloc_size+sizeof(HEADER)+sizeof(long));
    current = current->ptr_next;
    while (current != NULL) {
        printf("<%p>, %lu -> ", current, current->bloc_size+sizeof(HEADER)+sizeof(long));
        current = current->ptr_next;
    };
    printf("NULL\n");
}



int main(void) {
    printf("\n--------------- Spliting blocks ---------------\n");
    void * block4 = malloc_3is(200);
    printf("\nblock4: %p, size = %lu \n", block4, ((HEADER*)block4-1)->bloc_size);
    HEADER * full_block4 = block4 - sizeof(HEADER);
    free_3is(block4);
    print_linked_list();
    split_block(full_block4, 100);
    print_linked_list();

    printf("\n--------------- Use freed blocks ---------------\n");
    void* block5 = malloc_3is(60);
    printf("block5: %p, size = %lu \n", block5, ((HEADER*)block5-1)->bloc_size);
    print_linked_list();

    void* block6 = malloc_3is(50);
    printf("block6: %p, size = %lu \n", block6, ((HEADER*)block6-1)->bloc_size);
    print_linked_list();

    return 0;
}