#ifndef __CUCKOO_H
#define __CUCKOO_H

#include <pthread.h> /* pthread_spinlock */
#include "tsc.h"
#include "parallel_benchmarks.h"

struct node{
    int value;
    struct node * next;
};

struct array_list{ // its really a simple linked list
    struct node * head;
    int size;
};

struct CuckooSet{
    
    int capacity;
    int locks_length;
    struct array_list ** table; // need double arrray 
    char pad_cuck_0[(64- sizeof(struct array_list **) -2*sizeof(int))/sizeof(char)];
    pthread_spinlock_t **locks;
    char pad_cuck_1[(64- sizeof(pthread_spinlock_t ** ))/sizeof(char)];
} __attribute__ ((packed));

int THRESHOLD=0;
int PROB_SIZE=4000;
int LIMIT=160;

int hash0( int x){
    return x;
}

 
int hash1( int x){
    return x;
}

void acquire(struct CuckooSet * C ,int x){
    
    int x0=64*(hash0(x)%C->locks_length);        
    int x1=64*(hash1(x)%C->locks_length);        

    pthread_spin_lock(&(C->locks[0][x0]));
    pthread_spin_lock(&(C->locks[1][x1]));
    /*
    while (1){
        if (!C->locks[0][x0]){
            if(!__sync_lock_test_and_set(&(C->locks[0][x0]),1)) break;
        }
    }
    while (1){
        if (!C->locks[1][x1]){
            if(!__sync_lock_test_and_set(&(C->locks[1][x1]),1)) break;
        }
    }

    */

}
// i only lock the locks on one of the two arrays. If i hold these then noone can procede + i avoid deadlocks
void take_all_locks(struct CuckooSet *C){
    
    int i;
    for(i=0;i<C->locks_length;i++){

        pthread_spin_lock(&(C->locks[0][i*64]));
        /*while(1){
            if( !C->locks[0][i]){
                if(!__sync_lock_test_and_set(&(C->locks[0][i]),1)) break;
            }
        }
        */
    }
}

void leave_all_locks(struct CuckooSet *C){
    int i;
    for(i=0;i<C->locks_length;i++){
        pthread_spin_unlock(&(C->locks[0][i*64]));
        //C->locks[0][i] = 0;
    }
}

void release(struct CuckooSet *C, int x){

    
    int x0=64*(hash0(x)%C->locks_length);        
    int x1=64*(hash1(x)%C->locks_length);      

    
    pthread_spin_unlock(&(C->locks[0][x0]));
    pthread_spin_unlock(&(C->locks[1][x1]));
    //C->locks[0][x0] = 0;
    //C->locks[1][x1] = 0;

}

void _initialize(struct CuckooSet *C, int capacity){

    int i,j;
    C->capacity = capacity;
    C->locks_length = capacity;
    
    struct array_list * inner_table = (struct array_list *) malloc(sizeof( struct array_list ) * capacity *2);
    C->table = (struct array_list **) malloc(sizeof(struct array_list *) *2); 
    C->table[0]=&inner_table[0];
    C->table[1]=&inner_table[capacity]; //TODO: check this
    for(i=0;i<2;i++)
        for(j=0;j<capacity;j++){
            C->table[i][j].size=0;
            C->table[i][j].head=NULL;
        }
    pthread_spinlock_t  * inner_locks = (pthread_spinlock_t *)malloc(sizeof(pthread_spinlock_t) *capacity *2*64);
    C->locks = (pthread_spinlock_t **)malloc(sizeof(pthread_spinlock_t *) *2);
    C->locks[0]=&inner_locks[0];
    C->locks[1]=&inner_locks[capacity*64];
    for(i=0;i<2;i++)
        for(j=0;j<capacity;j++) 
            pthread_spin_init(&(C->locks[i][j*64]),PTHREAD_PROCESS_SHARED);
            //C->locks[i][j]=0;
}

int list_search(struct array_list * A, int x){
    
    int i;
    struct node * curr=A->head;
    while(curr!=NULL){
        if(curr->value> x) return 0;
        if(curr->value ==  x) return 1;
        curr = curr->next;
    }
    return 0;
}

int list_remove(struct array_list * A, int x){
   
    struct node * curr=A->head;
    struct node * prev=NULL;
    if (!curr) return 0;
    if(curr->value == x){
       A->head=curr->next;
       //free(curr);
       A->size--;
       return 1;
    }
    while(curr!=NULL){
        if (curr->value == x){
            prev->next=curr->next;
            //free(curr);
            A->size--;
            return 1;
        }
        if (curr->value >x) {
            //A->size++;
            return 0;
        }
        prev=curr;
        curr=curr->next;
   }
   return 0;
}

int  list_add(struct array_list *A, int x){
    
    struct node * new = (struct node * )malloc(sizeof(struct node));
    new->value = x;
    new->next = NULL;
    A->size++;
    /*struct node * curr = A->head;
    if (curr==NULL) {
        A->head = new;
        return;
    }
    while(curr->next!=NULL)
        curr = curr->next;

    curr->next = new;
    */
    struct node * curr=A->head;
    struct node * prev=NULL; 

    while(curr){
        if (curr->value== x) {
            A->size--;
            return 0;
        }
        if (curr->value>x) break;
        prev=curr;
        curr=curr->next;
    }
    new->next=curr;
    if (prev) prev->next=new;
    else A->head=new;
    return 1;
}   


int remove_set(struct CuckooSet *C, int x,params_t * params){

    tsc_start(&params->insert_lock_set_tsc);
    acquire(C,x);
    tsc_pause(&params->insert_lock_set_tsc);
    struct array_list * set0 = &C->table[0][hash0(x)%C->capacity];
    if(list_search(set0,x)){ //if it is on set 1 we delete it
        list_remove(set0,x);
        tsc_start(&params->insert_lock_set_tsc);
        release(C,x);
        tsc_pause(&params->insert_lock_set_tsc);
        return 1;
    }
    else{
        struct array_list *set1 = &C->table[1][hash1(x)%C->capacity];
        if(list_search(set1,x)){ //if it is on set 2 we delete it 
            list_remove(set1,x);
            tsc_start(&params->insert_lock_set_tsc);
            release(C,x);
            tsc_pause(&params->insert_lock_set_tsc);
            return 1;
        }
    }
    tsc_start(&params->insert_lock_set_tsc);
    release(C,x);
    tsc_pause(&params->insert_lock_set_tsc);
    return 0;
}


int contains(struct CuckooSet  *C, int x,params_t *params){
    
    tsc_start(&params->insert_lock_set_tsc);
    acquire(C,x);
    tsc_pause(&params->insert_lock_set_tsc);
    int h0=hash0(x)%C->capacity;
    struct array_list set0 = C->table[0][h0];
    if (list_search(&set0,x)){
        tsc_start(&params->insert_lock_set_tsc);
        release(C,x);
        tsc_pause(&params->insert_lock_set_tsc);
        return 1;
    }

    int h1=hash1(x)%C->capacity;
    struct array_list set1 = C->table[1][h1];
    if (list_search(&set1,x)){
        tsc_start(&params->insert_lock_set_tsc);
        release(C,x);
        tsc_pause(&params->insert_lock_set_tsc);
        return 1;
    }
    tsc_start(&params->insert_lock_set_tsc);
    release(C,x);
    tsc_pause(&params->insert_lock_set_tsc);
    return 0;
}

int times_resized=0;
void resize(struct CuckooSet *C,params_t *params){
    int i,j,res;
    struct node * curr;
    int old_capacity = C->capacity;
    take_all_locks(C);
    if (C->capacity!=old_capacity){
        leave_all_locks(C);
        return;
    }
    tsc_start(&params->resize_timer);
    printf("@resize\n");
    times_resized++;
    struct array_list ** old_table = C->table;
    C->capacity = 2*old_capacity;
    
    struct array_list * inner_table = (struct array_list *) malloc(sizeof( struct array_list ) * C->capacity *2);
    C->table = (struct array_list **) malloc(sizeof(struct array_list *) *2); 
    C->table[0]=&inner_table[0];
    C->table[1]=&inner_table[C->capacity]; //TODO: check this
    for(i=0;i<2;i++)
        for(j=0;j<C->capacity;j++){
            C->table[i][j].size=0;
            C->table[i][j].head=NULL;
        }

    for(i=0;i<2;i++){
        for(j=0;j<old_capacity;j++){
            curr = old_table[i][j].head;
            while (curr){
                int val = curr->value;
                res=add(C,val,1,params);
                curr = curr->next;
            }
         }
    }
    //TODO: free old table
    
    leave_all_locks(C);
    tsc_pause(&params->resize_timer);

}

int realocate(struct CuckooSet *C,int i, int hi,int reentrant,params_t *params){
 
    //printf("@realocate\n");
    int hj = 0;
    int j=1-i;
    int round;
    int res;
    for (round=0; round<LIMIT;round++){
        if (!reentrant) {
            tsc_start(&params->insert_lock_set_tsc);
            acquire(C,hi);//edit
            tsc_pause(&params->insert_lock_set_tsc);
        }
        struct array_list iset = C->table[i][hi];
        if (iset.size==0){
            if (!reentrant) {
                tsc_start(&params->insert_lock_set_tsc);
                release(C,hi);
                tsc_pause(&params->insert_lock_set_tsc);
            }
            return 1;
        }
        int y = iset.head->value; //danger!!
        if (!reentrant) {
            tsc_start(&params->insert_lock_set_tsc);
            release(C,hi);
            tsc_pause(&params->insert_lock_set_tsc);
        }
        if (!reentrant) {
            tsc_start(&params->insert_lock_set_tsc);
            acquire(C,y);
            tsc_pause(&params->insert_lock_set_tsc);
        }
        if (i==0) hj = hash1(y) % C->capacity;
        else hj = hash0(y) % C->capacity;
        struct array_list jset = C->table[j][hj];
        if(list_remove(&iset,y)){
            if(jset.size<THRESHOLD){
                res=list_add(&jset,y);
                if (!reentrant) release(C,y);
                return 1;
            }
            else if(jset.size<PROB_SIZE){
                res=list_add(&jset,y);
                i=1-i;
                hi=hj;
                j=1-j;
                if (!reentrant) release(C,y);
            }
            else{
                res=list_add(&iset,y);
                if (!reentrant) release(C,y);
                return 0;
            }
        }
        else if(iset.size>=THRESHOLD){
            if (!reentrant) release(C,y);
            continue;
        }
        else{
            if (!reentrant) release(C,y);
            return 1;
        }
    }
    return 0;

}



// if reentrant == 1 we must not lock( we are beeing called from resize so we have already locked)
int add(struct CuckooSet *C,int x,int reentrant,params_t *params){

    int res;
    if (!reentrant) {
        tsc_start(&params->insert_lock_set_tsc);
        acquire(C,x);
        tsc_pause(&params->insert_lock_set_tsc);
    }
    int h0 = hash0(x) % C->capacity;
    int h1 = hash1(x) % C->capacity;
    int i=-1;
    int h=-1;
    int must_resize=0;
    
    struct array_list *set0 = &C->table[0][h0];
    struct array_list *set1 = &C->table[1][h1];

    // if it already exists we dont need to add
    if(list_search(set0,x) || list_search(set1,x)){
        if (!reentrant) {
            tsc_start(&params->insert_lock_set_tsc);
            release(C,x);
            tsc_pause(&params->insert_lock_set_tsc);
        }
        return 0;
    }
    
    if(set0->size<THRESHOLD){
        res=list_add(set0,x);
        if (!reentrant) {
            tsc_start(&params->insert_lock_set_tsc);
            release(C,x);
            tsc_pause(&params->insert_lock_set_tsc);
        }
        return 1;
    }
    else if(set1->size<THRESHOLD){
        res=list_add(set1,x);
        if (!reentrant) {
            tsc_start(&params->insert_lock_set_tsc);
            release(C,x);
            tsc_pause(&params->insert_lock_set_tsc);
        }
        return 1;
    }
    else if(set0->size<PROB_SIZE){
        res=list_add(set0,x);
        i=0;
        h=h0;
    }
    else if(set1->size<PROB_SIZE){
        res=list_add(set1,x);
        i=1;
        h=h1;
    }
    else{
        must_resize =1;
    }
    if (!reentrant) {
        tsc_start(&params->insert_lock_set_tsc);
        release(C,x);
        tsc_pause(&params->insert_lock_set_tsc);
    }
    if (must_resize){
        if (reentrant) {
            printf("HEY!!~\n");
        }
        if (!reentrant) resize(C,params); //:TODO check this
        add(C,x,0,params);
    }
    else if (!realocate(C,i,h,reentrant,params)){
        if (reentrant) {
            printf("HEYYYY!!!~~\n");
        }
        if (!reentrant) resize(C,params);
    }
    return 1;
}

void print_hash_table(struct CuckooSet *C){
    
    int i;
    for(i=0;i<C->capacity;i++){
        struct node * curr=C->table[0][i].head;
        while(curr!=NULL){
            printf("%d ",curr->value);
            curr=curr->next;
        }
        printf(" -\n");
    }
    
    printf("---------------------------------\n");
    for(i=0;i<C->capacity;i++){
        struct node * curr=C->table[1][i].head;
        while(curr!=NULL){
            printf("%d ",curr->value);
            curr=curr->next;
        }
        printf(" -\n");
    }
}

/* Arrange the N elements of ARRAY in random order.
   Only effective if N is much smaller than RAND_MAX;
   if this may not be the case, use a better random
   number generator. */
void shuffle(int *array, size_t n)
{
    if (n > 1) 
    {
        size_t i;
        for (i = 0; i < n - 1; i++) 
        {
          size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
          int t = array[j];
          array[j] = array[i];
          array[i] = t;
        }
    }
}

struct HashSet {
    int dummy;
};//dummy struct

struct CuckooSet C;

void initialize(struct HashSet *H, int size,int threshold){
    
    THRESHOLD=threshold;
    _initialize(&C,size);
}

//Returns the number of items in the list.
unsigned int list_length(struct HashSet * H){
    
	unsigned int ret = 0;
    struct node *curr;
    
    int i;
    for (i=0;i<C.capacity;i++){
        ret+=C.table[0][i].size;
        ret+=C.table[1][i].size;
    }

    return ret;
}

long long  int list_sum(struct HashSet *H){
    
    struct node * curr;
	long long int ret = 0;
    int i;

    for (i=0;i<C.capacity;i++){
        curr=C.table[0][i].head;
        while(curr){
            ret+=curr->value;
            curr=curr->next;
        }
        curr=C.table[1][i].head;
        while(curr){
            ret+=curr->value;
            curr=curr->next;
        }
    }
    return ret;
}
void print_set_length(struct HashSet *H)
{
    int i;
	unsigned int total_elements = 0;
    unsigned int ll;
    
    printf("Table 1\n");
    for(i=0;i<C.capacity;i++) {
		ll = C.table[0][i].size;
		printf("Bucket %6d: %8u elements\n", i, ll);
		total_elements += ll;
	}
    printf("\nTable 2\n");
    for(i=0;i<C.capacity;i++) {
		ll = C.table[1][i].size;
		printf("Bucket %6d: %8u elements\n", i, ll);
		total_elements += ll;
	}
	printf("Total: %8u\n", total_elements);
}

int find_elements_count(struct HashSet *H){
	unsigned int total_elements = 0;
        
    total_elements=list_length(H); 

    return total_elements;
}

long long int find_elements_sum(struct HashSet *H){
	long long int total_sum = 0;

    total_sum=list_sum(H);
    
    return total_sum;
}
int Insert(struct HashSet *H, int key,params_t *params){
    return add(&C,key,0,params);
}

int Lookup(struct HashSet *H,int key,params_t *params){
    return contains(&C,key,params);
}

int Erase(struct HashSet *H, int key,params_t *params){
    return remove_set(&C,key,params);
}
#endif /*__CUCKOO_H*/
