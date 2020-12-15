//
// Created by Mario on 21.11.2020.
//

#define pageSize 4096
#define fenceSize 16
#define fenceVal 17
#define utilitySize (sizeof(struct control)+(2*fenceSize))
#define PAGES_AVAILABLE 16384
//#define hehe 1

#include "heap.h"
#include "custom_unistd.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>

void fenceset(char* ptr)
{
    for(int i = 0 ; i < fenceSize ; i++)
    {
        ptr[i] = (char)fenceVal;
    }
}

uint64_t controlCalc(struct control *ptr)
{
    uint64_t sum = 0;
    for(int i = 0; i < 5; i++)
    {
        sum += *((uint64_t *)ptr+i);
    }
    return sum;
}

char fence[17] = {fenceVal, fenceVal, fenceVal, fenceVal, fenceVal, fenceVal, fenceVal, fenceVal, fenceVal, fenceVal, fenceVal, fenceVal, fenceVal, fenceVal, fenceVal, fenceVal};

struct mein_heap myHeap = {0,0,0};

struct control *getLost() {
    struct control *temp = myHeap.head;
    for (; temp->next != NULL; temp = temp->next)
    {
        ;
    }
    return temp;
}

void *findAlignment(struct control *node)
{
    void *temp = node->usersMem;
    for(unsigned int i = 0 ; i <= node->size ; i++)
    {
        if((((intptr_t)((char *)temp+i) & (intptr_t)(pageSize - 1)) == 0))
        {
            return (char *)temp+i;
        }
    }
    return NULL;
}

int heap_setup(void) //1
{
    errno = 0;
    myHeap.heapStart= custom_sbrk(2*pageSize);
    if(errno)
    {
        return -1;
    }
    myHeap.size = 2*pageSize; // calkowita dostepna pamiec
    myHeap.head = (struct control *)((char*)myHeap.heapStart+fenceSize); //ustawiam control block na poczatku sterty, za plotkami
    memset(myHeap.heapStart,fenceVal,fenceSize); // plotek na poczatku
    memset((char*)(myHeap.heapStart)-fenceSize+2*pageSize,fenceVal, fenceSize); // plotek na koncu
    (myHeap.head)->next = NULL; // pierwszy wezel
    (myHeap.head)->prev = NULL; //
    (myHeap.head)->size = myHeap.size - 3*fenceSize - sizeof(struct control); // rozmiar wolnego bloku
    (myHeap.head)->isFree = 1; // wolny,
    memset((char *)myHeap.head+sizeof(struct control), fenceVal, fenceSize); // plotek za control blockiem
    (myHeap.head)->usersMem = (char*)myHeap.head+fenceSize+sizeof(struct control); //wskaznik na pierwszy adres wolnej przestrzeni (za blokiem i plotkiem)
    myHeap.head->control = controlCalc(myHeap.head);

    return 0;
}
void heap_clean(void) //2
{
    myHeap.heapStart = custom_sbrk((int)-myHeap.size);
    myHeap.heapStart = NULL;
    myHeap.size = 0;
    myHeap.head = NULL;
}

void* heap_malloc(size_t size) //4
{
    if(size<=0)
    {
        return NULL; // no troche maly size imo
    }

    struct control *temp = myHeap.head;

    for(;temp!=NULL; temp = temp->next)
    {
        if(temp->next == NULL)
        {
            break; // BO MI TEMP ZNIKNIE
        }
        if(temp->isFree == 1)
        {
            if(temp->size >= size || (char*)temp->usersMem+size == (char*)temp->next-fenceSize) // czy kiedys byl recyklingowy
            {

                temp->size = size; // nowy size
                memset((char*)temp->usersMem+size,fenceVal,fenceSize); // fill
                temp->isFree = 0;

                temp->control = controlCalc(temp);
                return temp->usersMem; // zwracam stary wskaznik
            }else
            {
                continue;
            }
        }
    }

    //  Nie ma free blockow w liscie -> jestem na koncu listy// jesli koniec listy, to raczej ten blok nie bedzie fillowany // ten if sie aktywuje przy 1. mallocu ZAWSZE

    if(temp->next == NULL)
    {
        //if(temp->trueSize == temp->size)
        {
            if(temp->size > utilitySize + size) // czy zmieszcze nowy malloc
            {


                memset((char *)temp->usersMem+size, fenceVal, fenceSize);

                struct control *node = (struct control *)((char *)temp->usersMem + size + fenceSize);

                memset(((char *)node + sizeof(struct control)), fenceVal, fenceSize);

                unsigned int nowySize = temp->size - size - utilitySize;

                temp->size = size;
                temp->next = node;
                temp->isFree = 0;
                temp->control = controlCalc(temp);
                node->next = NULL;
                node->prev = temp;
                node->usersMem = (char *)node + fenceSize+sizeof(struct control);


                if((int)nowySize - fenceSize < 0)
                {
                    errno = 0;
                    custom_sbrk(pageSize);
                    if(errno)
                    {
                        return NULL;
                    }

                    myHeap.size += pageSize;
                    memset((char*)myHeap.heapStart+myHeap.size-fenceSize, fenceVal, fenceSize);

                    nowySize += pageSize;
                    node->size = nowySize;
                    node->control = controlCalc(node);
                }else
                {
                    node->size = nowySize;
                    node->control = controlCalc(node);
                }

                node->isFree = 1;
                node->control = controlCalc(node);
                return temp->usersMem;

            }else // koncowka strony, nie ma miejsca na nowy malloc
            {
                //plswork:
                errno = 0;
                unsigned int ileStron = ((size/pageSize)+1); // ile potrzebuje stron ///
                if(ileStron > PAGES_AVAILABLE)
                {
                    return NULL;
                }
                unsigned int ileDoszlo = pageSize * ileStron;
                custom_sbrk(ileDoszlo);
                if(errno)
                {
                    return NULL;
                }
                //printf(" size before dociagnieciu %d\n", myHeap.size);
                myHeap.size += ileDoszlo;
                //printf(" size  after dociagnieciu %d\n\n", myHeap.size);

                memset(((char*)myHeap.heapStart+myHeap.size-fenceSize),fenceVal,fenceSize); // nowe plotki na koncu heapa


                temp->size += (ileDoszlo);
                // malloc + nowy control block //


                struct control *lastNode = (struct control *)((char* )temp->usersMem+size+fenceSize);

                temp->next = lastNode;
                temp->isFree = 0;

                memset((char *)lastNode+sizeof(struct control),fenceVal, fenceSize);
                memset((char *)lastNode - fenceSize, fenceVal, fenceSize);

                lastNode->next = NULL;
                lastNode->prev = temp;
                lastNode->isFree = 1;
                lastNode->size = temp->size-size-utilitySize;
                temp->size = size;
                lastNode->usersMem = (char*)lastNode + sizeof(struct control) + fenceSize;

                lastNode->control = controlCalc(lastNode);
                temp->control = controlCalc(temp);

                return temp->usersMem;
            }
        }

    }



    return NULL;
}
void* heap_calloc(size_t number, size_t size)
{
    if(number <= 0 || size <= 0)
    {
        return NULL;
    }
    void *temp = heap_malloc(number*size);
    if(temp != NULL)
        memset(temp,0,number*size);
    return temp;
}
void* heap_realloc(void* memblock, size_t count)
{
    if(heap_validate() != 0)
    {
        return NULL;
    }
    if(get_pointer_type(memblock) != pointer_valid && get_pointer_type(memblock) != pointer_null)
    {
        return NULL;
    }
    if(memblock == NULL)
    {
        return heap_malloc(count);
    }

    if(count == 0)
    {
        heap_free(memblock);
        return NULL;
    }
    struct control *last = getLost();
    struct control *temp = (struct control*)((char*)memblock-(fenceSize+sizeof(struct control)));
    if(count==temp->size)
    {
        return memblock;
    }
    if(temp->size>count){
        //przestaw plotek, rozmiar i zwroc block
        temp->size=count;
        memset((char *)memblock + temp->size, fenceVal, fenceSize);
        temp->control=controlCalc(temp);
        return memblock;
    }
    plswork:;
    //zwiekszamy size bloku
    if(count>temp->size){//to liczy ile wolnej pamieci pomiedzy poczatkiem usermema ktory powiekszasz a koncem nastepnego z ktorym merge
        if(temp->next && temp->next->isFree && (((uint64_t)((char*)temp->next+temp->next->size+sizeof(struct control)+fenceSize)-(uint64_t)temp->usersMem)>=count && temp->next->next != NULL)){
            //merge w prawo
            //dla potomnych wskazniki
            struct control placeholder = *temp->next;
            placeholder.next=temp->next;
            placeholder.prev=temp->prev;
            //setting new size
            temp->size = count;
            //relinking list
            temp->next=placeholder.next->next;

            if(placeholder.next->next){
                placeholder.next->next->prev=temp;
                placeholder.next->next->control=controlCalc(placeholder.next->next);
            }
            temp->control=controlCalc(temp);
            memset((char *)memblock + temp->size, fenceVal, fenceSize);
            return temp->usersMem;
        }else if(temp->next && temp->next->isFree && (((uint64_t)((char*)temp->next+temp->next->size+sizeof(struct control)+fenceSize)-(uint64_t)temp->usersMem) >= count + utilitySize && temp->next->next == NULL)){

            //merge w prawo
            //dla potomnych wkazniki
            struct control placeholder = *temp->next;
            placeholder.next=temp->next;
            placeholder.prev=temp->prev;
            //nowy size
            temp->size = count;
            //przepinka
            temp->next=placeholder.next->next;

            if(placeholder.next->next){
                placeholder.next->next->prev=temp;
                placeholder.next->next->control=controlCalc(placeholder.next->next);
            }else
            {
                struct control *newNode = (struct control *)((char *)temp->usersMem + temp->size + fenceSize);
                memset((char *)newNode+sizeof(struct control),fenceVal, fenceSize);
                newNode->usersMem = (char *)newNode + sizeof(struct control) + fenceSize;
                newNode->isFree = 1;
                newNode->next = NULL;
                newNode->prev = temp;
                temp->next = newNode;
                newNode->size = (uint64_t)((char *)myHeap.heapStart + myHeap.size - fenceSize) - (uint64_t)(newNode->usersMem);
                newNode->control = controlCalc(newNode);
            }
            temp->control=controlCalc(temp);
            memset((char *)memblock + temp->size, fenceVal, fenceSize);
            return temp->usersMem;
        }else if(count + utilitySize >= last->size)
        {

            errno = 0;
            custom_sbrk((((count + utilitySize)/pageSize)+1)*pageSize);
            if(errno)
            {
                return NULL;
            }

            myHeap.size += (((count + utilitySize)/pageSize)+1)*pageSize;
            memset((char*)myHeap.heapStart+myHeap.size-fenceSize, fenceVal, fenceSize);

            last->size += (((count + utilitySize)/pageSize)+1)*pageSize;
            last->control = controlCalc(last);
            //printf("1");
            goto plswork;
        }
            // nie mozna uzyc starego wiec malloc -> memcpy ->free ->nowy blok
        else{
            void* newBlock=heap_malloc(count);
            memcpy(newBlock,memblock,temp->size);
            heap_free(memblock);
            return newBlock;
        }
    }

    return NULL;
}
void heap_free(void* memblock)
{
    //printf("samo freee\n");
    if(custom_sbrk_check_fences_integrity())
    {
        printf(" free check \n");
    }
    if(memblock != NULL)
    {
        if(heap_validate() == 0 && get_pointer_type(memblock) == pointer_valid)
        {
            struct control *temp = (struct control *)((char *)memblock - sizeof(struct control) - fenceSize);
            temp->size = (char *)temp->next - (char *)temp->usersMem - fenceSize;
            memset((char *)temp->usersMem + temp->size, fenceVal, fenceSize);
            //temp->isFree = 1;
            if(temp->next->isFree == 1 && temp->next != NULL) // merge lewo
            {
                struct control *temp2 = temp->next;
                temp->next = temp2->next;
                temp->size += temp2->size+utilitySize;
                temp2->isFree = 1;
                temp->isFree = 1;
                temp->control = controlCalc(temp);
                if(temp->next){
                    temp->next->prev=temp;
                    temp->next->control=controlCalc(temp->next);
                }
                memset((char*)temp->usersMem+temp->size, fenceVal, fenceSize);
            }

            if(temp->prev != NULL && temp->prev->isFree == 1 && !(((intptr_t)memblock & (intptr_t)(pageSize - 1)) == 0))
            {
                struct control *temp2 = temp->prev;
                temp2->next = temp->next;
                temp2->size += temp->size + utilitySize;
                if(temp2->next){
                    temp2->next->prev=temp2;
                    temp2->next->control=controlCalc(temp2->next);
                }

                temp2->control = controlCalc(temp2);
                memset((char*)temp2->usersMem+temp2->size, fenceVal, fenceSize);
            }
            temp->isFree = 1;
            temp->control = controlCalc(temp);
        }
    }

}

size_t heap_get_largest_used_block_size(void)
{
    struct control *temp = myHeap.head;
    if(temp == NULL)
    {
        return 0;
    }
    if(heap_validate())
    {
        return 0;
    }
    unsigned int max = 0;
    unsigned int cnt = 0;
    for(;temp!=NULL; temp = temp->next)
    {
        if(cnt != 0 && temp->next == NULL && temp->isFree == 0)
        {
            return 0;
        }
        cnt++;
        if(temp->size > max && temp->isFree == 0 && temp->next != NULL)
        {
            max = temp->size;
        }
    }
    return max;
}
enum pointer_type_t get_pointer_type(const void* const pointer)
{
    if(pointer == NULL) return pointer_null;
    if(heap_validate())
    {
        return pointer_heap_corrupted;
    }
    struct control *temp = myHeap.head;
    for(;temp!=NULL; temp = temp->next)
    {
        void *begin = temp;
        //AAAABBBBB
        //void *end = temp->next;
        void *end = (void *)((char *)temp->usersMem + temp->size + fenceSize);
        if(pointer >= begin && pointer < end)
        {
            if(temp->isFree == 1)
            {
                return pointer_unallocated;
            }else
            {
                //control block//
                if(pointer >= begin && (char *)pointer < (char *)begin+sizeof(struct control))
                {
                    return pointer_control_block;
                }
                //plotek za CB//
                if((char*)pointer >= (char *)begin+sizeof(struct control) && (char *)pointer < (char *)temp->usersMem)
                {
                    return pointer_inside_fences;
                }
                //poczatek user blocka//
                if(pointer == temp->usersMem)
                {
                    return pointer_valid;
                }
                if(pointer > temp->usersMem && (char *)pointer < (char *)temp->usersMem + temp->size)
                {
                    return pointer_inside_data_block;
                }
                //za user blockem (czyli plotek + retarded plotek)//
                if((char *)pointer >= (char *)temp->usersMem + temp->size &&(char *)pointer < (char *)temp->usersMem + temp->size +fenceSize)
                {
                    return pointer_inside_fences;
                }
            }
        }
    }
    return pointer_unallocated;
}

int heap_validate(void) //3 validate as true
{
   // printf("validate called\n");
    if(!myHeap.heapStart)
    {
        return 2;
    }
    //fenceset(fence);
    //printf("%d\n", __LINE__);
    struct control *temp = myHeap.head;

    //plotki heapa
   // printf("%d\n", __LINE__);
    if(memcmp(myHeap.heapStart, fence, fenceSize)==0)
    {
        //printf("%d\n", __LINE__);
        if(memcmp((char*)myHeap.heapStart+myHeap.size-fenceSize,fence,fenceSize) != 0)
        {
            //printf("%d\n", __LINE__);
            //printf("umar plotek heapa :( \n");
            return 1;
        }

    }
    //plotki na stercie//
   // printf("%d\n", __LINE__);
    uint64_t chck = 0;
    for(;temp!=NULL; temp = temp->next)
    {
        // suma kontrolna
        //printf("%d\n", __LINE__);
        chck = controlCalc(temp);
        if(temp->control != chck)
        {
           // printf("%d\n", __LINE__);
            return 3;
        }

        //plotki dookola user blocka//
        //printf("%d\n", __LINE__);
        if(memcmp((char*)temp->usersMem-fenceSize,(char*)fence,fenceSize) != 0)
        {
            //printf("%d\n", __LINE__);
            //printf("umar plotek user blocku :( no.1 wywolanie %d\n", i);
            return 1;
        }else
        {
            //printf("%d\n", __LINE__);
            if(memcmp((char*)temp->usersMem+temp->size,(char*)fence,fenceSize) != 0)
            {
               // printf("%d\n", __LINE__);
                //printf("umar plotek user blocku :( no.2 wywolanie %d\n", i);
                return 1;
            }
        }
    }
    //printf("%d\n", __LINE__);
   // printf("validate done\n");
    return 0;
}

void* heap_malloc_aligned(size_t size)
{
   // printf("malloc called \n");
    if(size<=0)
    {
        return NULL; // no troche maly size imo
    }

    struct control *temp = myHeap.head;

    for(;temp!=NULL; temp = temp->next)
    {
        if(temp->next == NULL)
        {
            if((((intptr_t)temp->usersMem & (intptr_t)(pageSize - 1)) == 0)) // TEGO NIE PRZEWIDZIELISMY
            {
                if(temp->size > size + utilitySize)
                {
                    struct control *newLastNode = (struct control* )((char *)temp->usersMem + size + fenceSize);
                    newLastNode->usersMem = (char *)newLastNode + sizeof(struct control) + fenceSize;
                    newLastNode->size = temp->size - size - utilitySize;
                    memset((char *)newLastNode->usersMem - fenceSize, fenceVal, fenceSize);
                    memset((char *)newLastNode->usersMem + newLastNode->size, fenceVal, fenceSize);
                    memset((char *)temp->usersMem + size, fenceVal, fenceSize);
                    temp->size = size;
                    temp->isFree = 0;
                    newLastNode->isFree = 1;
                    newLastNode->next = NULL;
                    newLastNode->prev = temp;
                    temp->next = newLastNode;
                    temp->control = controlCalc(temp);
                    newLastNode->control = controlCalc(newLastNode);

                    return temp->usersMem;
                }
            }else
                break; // BO MI TEMP ZNIKNIE
        }
        if(temp->isFree == 1 && (((intptr_t)temp->usersMem & (intptr_t)(pageSize - 1)) == 0)) //recykling alignowanego bloku
        {
            if(temp->size >= size || (char*)temp->usersMem+size == (char*)temp->next-fenceSize) // czy kiedys byl recyklingowy
            {

                temp->size = size; // nowy size
                memset((char*)temp->usersMem+size,fenceVal,fenceSize); // fill
                temp->isFree = 0;

                temp->control = controlCalc(temp);
                return temp->usersMem; // zwracam stary wskaznik
            }else
            {
                continue;
            }
        }
    }

    //  Nie ma free blockow w liscie -> jestem na koncu listy// jesli koniec listy, to raczej ten blok nie bedzie fillowany // ten if sie aktywuje przy 1. mallocu ZAWSZE
    void *alignedPtr = NULL;
    if(temp->next == NULL)
    {
        //if(temp->trueSize == temp->size)
        {
            plswork2:;
            if((alignedPtr = findAlignment(temp)) != NULL)
            {
                if((size_t)(((char *)myHeap.heapStart + myHeap.size - fenceSize) -((char *)alignedPtr)) > size + utilitySize) // koniec heapa - poczatek userblocka >= malloc + utility
                {
                    //printf("%d\n", __LINE__);
                    struct control backup; // bo sie zgubia linki oO
                    backup.next = temp->next;
                    backup.prev = temp->prev;
                    backup.usersMem = temp->usersMem;
                    backup.size = temp->size;
                    backup.isFree = temp->isFree;

                    struct control *nodeAlign = (struct control *)((char *)alignedPtr - fenceSize - sizeof(struct control));
                    memset((char *)alignedPtr - fenceSize, fenceVal, fenceSize);

                    memset((char *)alignedPtr + size, fenceVal, fenceSize);

                    struct control *newLast = (struct control *)((char *)alignedPtr + size + fenceSize);


                    newLast->size = ((char *)myHeap.heapStart + myHeap.size - fenceSize) - ((char *)newLast + sizeof(struct control) + fenceSize);
                    newLast->usersMem = (char *)newLast + sizeof(struct control) + fenceSize;
                    memset((char *)newLast->usersMem - fenceSize, fenceVal, fenceSize);

                    memset((char *)newLast->usersMem + newLast->size, fenceVal, fenceSize);

                    newLast->next = backup.next;
                    newLast->prev = nodeAlign;
                    newLast->isFree = 1;
                    newLast->control = controlCalc(newLast);

                    nodeAlign->next = newLast;
                    nodeAlign->prev = backup.prev;
                    if(nodeAlign->prev != NULL)
                    {
                        nodeAlign->prev->next = nodeAlign;
                        nodeAlign->prev->control = controlCalc(nodeAlign->prev);
                    }else
                    {
                        temp->next = nodeAlign;
                        temp->size = (char *)nodeAlign - (char *)temp->usersMem - fenceSize;
                        memset((char *)temp->usersMem+temp->size,fenceVal,fenceSize);
                        temp->control = controlCalc(temp);

                    }
                    nodeAlign->size = size;
                    nodeAlign->isFree = 0;
                    nodeAlign->usersMem = alignedPtr;
                    nodeAlign->control = controlCalc(nodeAlign);
                    //printf("returned \n");
                    return nodeAlign->usersMem;
                }else alignedPtr = NULL;
            }
            if(alignedPtr == NULL)
            {
                errno = 0;
                unsigned int ileStron = (((size+utilitySize + fenceSize)/pageSize)+1); // ile potrzebuje stron /// tu sie cos sypie
                if(ileStron > PAGES_AVAILABLE)
                {
                    return NULL;
                }
                unsigned int ileDoszlo = pageSize * ileStron;
                custom_sbrk(ileDoszlo);
                if(errno)
                {
                    return NULL;
                }
                myHeap.size += ileDoszlo;

                memset(((char*)myHeap.heapStart+myHeap.size-fenceSize),fenceVal,fenceSize); // nowe plotki na koncu heapa

                temp->size += (ileDoszlo);
                temp->control = controlCalc(temp);
                //printf("2");
                goto plswork2;
            }
        }

    }

    printf("Cos poszlo bardzo nie tak z kodem");
    return NULL;
}
void* heap_calloc_aligned(size_t number, size_t size)
{
    if(number <= 0 || size <= 0)
    {
        return NULL;
    }
    void *temp = heap_malloc_aligned(number*size);
    if(temp != NULL)
        memset(temp,0,number*size);
    return temp;
}
void* heap_realloc_aligned(void* memblock, size_t count)
{
    //printf("realloc\n");
    if(heap_validate() != 0)
    {
        return NULL;
    }
    if(get_pointer_type(memblock) != pointer_valid && get_pointer_type(memblock) != pointer_null)
    {
        return NULL;
    }
    if(memblock == NULL)
    {
        //printf("realloc to malloc memblock null\n");
        return heap_malloc_aligned(count);
    }

    if(count == 0)
    {
        //printf("realloc to free\n");
        heap_free(memblock);
        return NULL;
    }
    struct control *last = getLost();
    struct control *temp = (struct control*)((char*)memblock-(fenceSize+sizeof(struct control)));
    if(count==temp->size)
    {
        return memblock;
    }
    if(temp->size>count){
        //przestaw plotek, rozmiar i zwroc block
        temp->size=count;
        memset((char *)memblock + temp->size, fenceVal, fenceSize);
        temp->control=controlCalc(temp);
        return memblock;
    }
    plswork:;
    //zwiekszamy blok
    if(count>temp->size){//to liczy ile masz wolnej pamieci pomiedzy poczatkiem usermema ktory powiekszasz a koncem nastepnego z ktorym mergujesz
        if(temp->next && temp->next->isFree && (((uint64_t)((char*)temp->next+temp->next->size+sizeof(struct control)+fenceSize)-(uint64_t)temp->usersMem)>=count && temp->next->next != NULL)){
            //printf("%d\n", __LINE__);
            //merge w prawo
            //dla potomnych wskazniki
            struct control placeholder = *temp->next;
            placeholder.next=temp->next;
            placeholder.prev=temp->prev;
            //nowy size
            temp->size = count;
            //przepinka
            temp->next=placeholder.next->next;


            if(placeholder.next->next){
                placeholder.next->next->prev=temp;
                placeholder.next->next->control=controlCalc(placeholder.next->next);
            }
            temp->control=controlCalc(temp);
            memset((char *)memblock + temp->size, fenceVal, fenceSize);
            return temp->usersMem;
        }else if(temp->next && temp->next->isFree && (((uint64_t)((char*)temp->next+temp->next->size+sizeof(struct control)+fenceSize)-(uint64_t)temp->usersMem) >= count + utilitySize && temp->next->next == NULL)){
            //printf("%d\n", __LINE__);
            //merge w prawo
            //dla potomnych wskazniki
            struct control placeholder = *temp->next;
            placeholder.next=temp->next;
            placeholder.prev=temp->prev;
            //nowy size
            temp->size = count;
            //przepinka
            temp->next=placeholder.next->next;

            if(placeholder.next->next){
                placeholder.next->next->prev=temp;
                placeholder.next->next->control=controlCalc(placeholder.next->next);
            }else
            {
                //printf("%d\n", __LINE__);
                struct control *newNode = (struct control *)((char *)temp->usersMem + temp->size + fenceSize);
                memset((char *)newNode+sizeof(struct control),fenceVal, fenceSize);
                newNode->usersMem = (char *)newNode + sizeof(struct control) + fenceSize;
                newNode->isFree = 1;
                newNode->next = NULL;
                newNode->prev = temp;
                temp->next = newNode;
                newNode->size = (uint64_t)((char *)myHeap.heapStart + myHeap.size - fenceSize) - (uint64_t)(newNode->usersMem);
                newNode->control = controlCalc(newNode);
            }
            temp->control=controlCalc(temp);
            memset((char *)memblock + temp->size, fenceVal, fenceSize);
            return temp->usersMem;
        }else if((size_t)((char *)temp->next - (char *)temp->usersMem - fenceSize) > count)
        {
            //printf("%d\n", __LINE__);
            temp->size = count;
            memset((char *)temp->usersMem + temp->size,fenceVal, fenceSize);
            temp->control = controlCalc(temp);

            return temp->usersMem;
        }
        else if(count + utilitySize >= last->size)
        {
            //printf("%d\n", __LINE__);
            errno = 0;
            custom_sbrk((((count + utilitySize)/pageSize)+1)*pageSize);
            if(errno)
            {
                return NULL;
            }

            myHeap.size += (((count + utilitySize)/pageSize)+1)*pageSize;
            memset((char*)myHeap.heapStart+myHeap.size-fenceSize, fenceVal, fenceSize);

            last->size += (((count + utilitySize)/pageSize)+1)*pageSize;
            last->control = controlCalc(last);

           // printf("3"); // return 3 pls
            goto plswork;
        }
            // nie mozna uzyc starego wiec malloc -> memcpy ->free ->nowy blok
        else{
           // printf("realloc to malloc\n");
            //printf("%d\n", __LINE__);
            void* newBlock=heap_malloc_aligned(count);
            memcpy(newBlock,memblock,temp->size);
            heap_free(memblock);
            return newBlock;
        }
    }

    return NULL;
}
