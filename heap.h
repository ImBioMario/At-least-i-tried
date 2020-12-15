//
// Created by PC on 21.11.2020.
//

#ifndef PROJECT1_HEAP_H
#define PROJECT1_HEAP_H
#include <stdio.h>
#include <stdint.h>

struct control
{
    struct control* next;
    struct control* prev;
    uint64_t size; // ile pamieci ma user
    uint64_t isFree; // 1-free, 0-occupied;
    void* usersMem; // pointer na poczatek obszaru uzytkownika
    uint64_t control;
};

struct mein_heap
{
    struct control* head;
    void* heapStart; // heapEnd to bedzie heapStart + size;
    unsigned int size;
};


enum pointer_type_t
{
    pointer_null, // 0 // null, duh
    pointer_heap_corrupted, // 1 // cos sie, cos sie zepsulo
    pointer_control_block, // 2 // w srodku control blocku
    pointer_inside_fences, // 3 // w srodku plotka
    pointer_inside_data_block, // 4 // wewnatrz nie-Free bloku uzytkownika
    pointer_unallocated, // 5 // wewnatrz free bloku uzytkownika
    pointer_valid // 6 // 1 bajt przestrzeni uzytkownika (nie-Free -> bo ma ten wskaznik zwalniac heap_free()
};
uint64_t controlCalc(struct control *ptr);
struct control *getLastSize();
void fenceset(char* ptr);
struct control *getLost();
void *findAlignment(struct control *node);

int heap_setup(void);
void heap_clean(void);

void* heap_malloc(size_t size);
void* heap_calloc(size_t number, size_t size);
void* heap_realloc(void* memblock, size_t count);
void heap_free(void* memblock);

size_t heap_get_largest_used_block_size(void);
enum pointer_type_t get_pointer_type(const void* const pointer);

int heap_validate(void);

void* heap_malloc_aligned(size_t size);
void* heap_calloc_aligned(size_t number, size_t size);
void* heap_realloc_aligned(void* memblock, size_t size);


#endif //PROJECT1_HEAP_H




/*
 * if(temp->next == NULL)
    {
        //if(temp->trueSize == temp->size)
        {
            if(temp->size > utilitySize + size) // czy zmieszcze nowy malloc
            {
                if(custom_sbrk_check_fences_integrity())
                {
                    printf(" line %d \n", __LINE__);
                }

                struct control *node = (struct control *)((char *)temp->usersMem + size + fenceSize);
                if(custom_sbrk_check_fences_integrity())
                {
                    printf(" line %d \n", __LINE__);
                }

                memset(((char *)node + sizeof(struct control)), fenceVal, fenceSize); // <---------- COS SIE SYPIE
                if(custom_sbrk_check_fences_integrity())
                {
                    printf(" line %d \n", __LINE__);
                }
                memset(((char *)node-fenceSize),fenceVal,fenceSize);
                if(custom_sbrk_check_fences_integrity())
                {
                    printf(" line %d \n", __LINE__);
                }
                unsigned int nowySize = temp->size - size -2*fenceSize-sizeof(struct control);
                if(custom_sbrk_check_fences_integrity())
                {
                    printf(" line %d \n", __LINE__);
                }
                temp->size = size;
                temp->next = node;
                temp->isFree = 0;
                node->next = NULL;
                node->prev = temp;
                node->usersMem = (char *)node + fenceSize+sizeof(struct control);
                node->size = nowySize;
                //memset(((char*)node->usersMem+node->size), fenceVal, fenceSize);
                node->isFree = 1;
                //node->trueSize =    // whattado ???
                return temp->usersMem;

            }else // koncowka strony, nie ma miejsca na nowy malloc
            {
                //plswork:
                errno = 0;
                unsigned int ileStron = ((size/pageSize)+250); // ile potrzebuje stron /// tu sie cos sypie
                if(ileStron > PAGES_AVAILABLE)
                {
                    return NULL;
                }
                custom_sbrk(pageSize * ileStron);
                if(errno)
                {
                    return NULL;
                }
                unsigned int ileDoszlo = pageSize * ileStron;
                myHeap.size += ileDoszlo;
                if(custom_sbrk_check_fences_integrity())
                {
                    printf(" line %d \n", __LINE__);
                }
                memset(((char*)myHeap.heapStart+myHeap.size-fenceSize),fenceVal,fenceSize); // nowe plotki na koncu heapa
                if(custom_sbrk_check_fences_integrity())
                {
                    printf(" line %d \n", __LINE__);
                }
                temp->size += ileDoszlo; // nowy size ostatniego bloku

                // malloc + nowy control block //

                if(custom_sbrk_check_fences_integrity())
                {
                    printf(" line %d \n", __LINE__);
                }
                struct control *lastNode = (struct control *)((char* )temp->usersMem+size+fenceSize);
                if(custom_sbrk_check_fences_integrity())
                {
                    printf(" line %d \n", __LINE__);
                }
                temp->next = lastNode;
                temp->isFree = 0;
                if(custom_sbrk_check_fences_integrity())
                {
                    printf(" line %d \n", __LINE__);
                }
                memset((char *)lastNode+sizeof(struct control),fenceVal, fenceSize);
                memset((char *)lastNode - fenceSize, fenceVal, fenceSize);
                if(custom_sbrk_check_fences_integrity())
                {
                    printf(" line %d \n", __LINE__);
                }
                lastNode->next = NULL;
                lastNode->prev = temp;
                lastNode->isFree = 1;
                lastNode->size = temp->size - size - fenceSize;
                temp->size = size;
                lastNode->usersMem = (char*)lastNode + sizeof(struct control) + fenceSize;

                return temp->usersMem;
                //TO DO : pomyslec nad: trueValue? zrobic doplotkowanie ? Jak ustawiam trueValue nowego wezla ?
            }
        }

    }
 *
 */
