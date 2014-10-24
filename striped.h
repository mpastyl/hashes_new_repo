#ifndef __STRIPED_H
#define __STRIPED_H

#include <pthread.h> /* pthread_spinlock */
#include "tsc.h"
#include "parallel_benchmarks.h"
// node of a list (bucket)
struct node_t{
    int value;
    int hash_code;
    struct node_t * next;
};


struct HashSet{
    //int length;
    struct node_t ** table;
    int capacity;
    int setSize;
    int locks_length;
    char pad_str_0[(64- sizeof(struct node_t **) -3*sizeof(int))/sizeof(char)];
    pthread_spinlock_t * locks;
    char pad_str_1[(64- sizeof(pthread_spinlock_t *))/sizeof(char)];
} __attribute__ ((packed));

void lock_set (struct HashSet * H, int hash_code){

    
    int indx=64*(hash_code % H->locks_length);
    pthread_spin_lock(&H->locks[indx]);
    /*
    while (1){
        if (!H->locks[indx]){
            if(!__sync_lock_test_and_set(&(H->locks[indx]),1)) break;
        }
    }
    */
}

void unlock_set(struct HashSet * H, int hash_code){

    int indx=64*(hash_code % H->locks_length);
    pthread_spin_unlock(&H->locks[indx]);
    //H->locks[indx] = 0;
}



//Returns the number of items in the list.
unsigned int list_length(struct node_t * Head){
    
    struct node_t * curr;
	unsigned int ret = 0;
    
    curr=Head;
    while(curr){
		ret++;
        curr=curr->next;
    }

    return ret;
}

long long  int list_sum(struct node_t * Head){
    
    struct node_t * curr;
	long long int ret = 0;
    
    curr=Head;
    while(curr){
		ret+=curr->value;
        curr=curr->next;
    }

    return ret;
}


//search value in bucket;
int list_search(struct node_t * Head,int val){
    
    struct node_t * curr;
    
    curr=Head;
    while(curr){
        if(curr->value>val) return 0;
        if(curr->value==val) return 1;
        curr=curr->next;
    }
    return 0;
}


//add value in bucket;
int list_add(struct HashSet * H, int key,int val,int hash_code){
    
    struct node_t * curr;
    struct node_t * prev;
    struct node_t * node=(struct node_t *)malloc(sizeof(struct node_t));
    /*node->value=val;
    node->next=NULL;
    curr=H->table[key];
    if(curr==NULL){
        H->table[key]=node;
        return ;
    }
    while(curr->next){
        curr=curr->next;
        next=curr->next;
    }
    curr->next=node;
    */
    //node->value=val;
    //node->hash_code=hash_code;
    //if(H->table[key]==NULL) node->next=NULL;
   //else node->next=H->table[key];
    //H->table[key]=node;
    node->value=val;
    node->hash_code=hash_code;

    curr=H->table[key];
    prev=NULL;
    while(curr){
        if (curr->value==val) return 0;
        if (curr->value>val) break;
        prev=curr;
        curr=curr->next;
    }
    node->next=curr;
    if (prev) prev->next=node;
    else H->table[key]=node;
    return 1;
}


// delete from bucket. The fist value equal to val will be deleted
int list_delete(struct HashSet *H,int key,int val){
    
    struct node_t * curr;
    //struct node_t * next;
    struct node_t * prev;

    curr=H->table[key];
    prev=curr;
    if((curr!=NULL)&&(curr->value==val)) {
        H->table[key]=curr->next;
        free(curr);
        return 1;
    }
    while(curr){
        if( curr->value==val){
            prev->next=curr->next;
            free(curr);
            return 1;
        }
        else if (curr->value >val) return 0;
        prev=curr;
        curr=curr->next;
    }
    return 0;
}





void _initialize(struct HashSet * H, int capacity,int lock_length){
    
    int i;
    H->setSize=0;
    H->capacity=capacity;
    H->table = (struct node_t **)malloc(sizeof(struct node_t *)*capacity);
    for(i=0;i<capacity;i++){
        H->table[i]=NULL;
    }
    H->locks_length=lock_length;
    //H->locks=(pthread_spinlock_t *)malloc(sizeof(pthread_spinlock_t)* lock_length  );
    H->locks=(pthread_spinlock_t *)malloc(sizeof(pthread_spinlock_t)* lock_length *64 );//padding for locks
    //H->locks=(pthread_spinlock_t *)malloc(sizeof(64* lock_length ); //padding for locks
    //for(i=0;i<lock_length;i++) H->locks[i*64]=0;
    for(i=0;i<(lock_length);i++) pthread_spin_init(&H->locks[64*i],PTHREAD_PROCESS_SHARED);

}


int policy(struct HashSet *H,params_t * params){
    if (params->enable_resize)
        return ((H->setSize/H->capacity) >10);
    else return 0;
}

void resize(struct HashSet *,params_t *params);

int contains(struct HashSet *H,int hash_code, int val,params_t *params){
    
	tsc_start(&params->insert_lock_set_tsc);
    lock_set(H,hash_code);
	tsc_pause(&params->insert_lock_set_tsc);
    int bucket_index = hash_code % H->capacity;
    int res=list_search(H->table[bucket_index],val);
	tsc_start(&params->insert_lock_set_tsc);
    unlock_set(H,hash_code);
	tsc_pause(&params->insert_lock_set_tsc);
    return res;
}

//reentrant ==1 means we must not lock( we are calling from resize so we have already locked the data structure)
int add(struct HashSet *H,int hash_code, int val, int reentrant,params_t *params){
    
    if(!reentrant){
		tsc_start(&params->insert_lock_set_tsc);
        lock_set(H,hash_code);
		tsc_pause(&params->insert_lock_set_tsc);
    }
    int bucket_index = hash_code % H->capacity;
    int res =list_add(H,bucket_index,val,hash_code);
    //H->setSize++;
    if(!reentrant) {
		tsc_start(&params->insert_lock_set_tsc);
        unlock_set(H,hash_code);
		tsc_pause(&params->insert_lock_set_tsc);
    }
    if (!res) return 0;
    __sync_fetch_and_add(&(H->setSize),1);
    
    if (policy(H,params)) if (!reentrant) resize(H,params);
    return 1;
}

int _delete(struct HashSet *H,int hash_code, int val,params_t *params){
    
	tsc_start(&params->insert_lock_set_tsc);
    lock_set(H,hash_code);
	tsc_pause(&params->insert_lock_set_tsc);
    int bucket_index =  hash_code % H->capacity;
    int res=list_delete(H,bucket_index,val);
    //H->setSize--;
    __sync_fetch_and_sub(&(H->setSize),1);
	tsc_start(&params->insert_lock_set_tsc);
    unlock_set(H,hash_code);
	tsc_pause(&params->insert_lock_set_tsc);
    return res;
}


int times_resized=0;
void resize(struct HashSet *H,params_t *params){
    
    int i,res;
    struct node_t * curr;
    int old_capacity = H->capacity;
    double time= tsc_getsecs(&params[i].resize_timer);
	tsc_start(&params->resize_timer);
    
    lock_set(H,i);
    if (H->capacity != old_capacity){
	    tsc_pause(&params->resize_timer);
        unlock_set(H,i);
        return ;//somebody beat us to it
    }
            
    
    for(i=1;i<H->locks_length;i++) {
	    //tsc_start(&params->insert_lock_set_tsc);
        lock_set(H,i);
	    //tsc_pause(&params->insert_lock_set_tsc);
    }
	/*tsc_pause(&params->resize_timer);
    if(old_capacity!=H->capacity) {
        for(i=0;i<H->locks_length;i++) {
	        //tsc_start(&params->insert_lock_set_tsc);
            unlock_set(H,i);
	        //tsc_pause(&params->insert_lock_set_tsc);
        }
        return; //somebody beat us to it
    }
    */
	//tsc_start(&params->resize_timer);
	//tsc_start(&params->resize_timer);
    printf("@resize!\n");
    
    times_resized++;
    int new_capacity =  old_capacity * 2;
    H->capacity =  new_capacity;

    struct node_t ** old_table = H->table;
    H->setSize=0;
    H->table = (struct node_t **)malloc(sizeof(struct node_t *)*new_capacity);
    for(i=0;i<new_capacity;i++){
        H->table[i]=NULL;
    }
    for(i=0;i<old_capacity;i++){
        
        curr=old_table[i];
        while(curr){
                int val = curr->value;
                int hash_code = curr->hash_code;
                //int bucket_index= hash_code % new_capacity;
                res=add(H,hash_code,val,1,params);
                curr=curr->next;
        }
    }
    free(old_table);
    for(i=0;i<H->locks_length;i++) unlock_set(H,i);
    printf("capacity after resize %d\n",H->capacity);
	tsc_pause(&params->resize_timer);
    params->resize_time += tsc_getsecs(&params->resize_timer) - time;
    printf("Resize No %d  time taken %4.8lf\n",times_resized,tsc_getsecs(&params->resize_timer) - time);
}

void print_set(struct HashSet * H){
    
    int i;
    for(i=0;i<H->capacity;i++){
        
        struct node_t * curr=H->table[i];
        while(curr){
            printf("(%d) ",curr->value);
            curr=curr->next;
        }
        printf("--\n");
    }
}

void print_set_length(struct HashSet *H)
{
    int i;
	unsigned int total_elements = 0;

    for(i=0;i<H->capacity;i++) {
		unsigned int ll = list_length(H->table[i]);
		printf("Bucket %6d: %8u elements\n", i, ll);
		total_elements += ll;
	}
	printf("Total: %8u\n", total_elements);
}

int find_elements_count(struct HashSet *H){
    int i;
	unsigned int total_elements = 0;

    for(i=0;i<H->capacity;i++) {
		unsigned int ll = list_length(H->table[i]);
		//printf("Bucket %6d: %8u elements\n", i, ll);
		total_elements += ll;
	}
	//printf("Total: %8u\n", total_elements);
    return total_elements;
}

long long int find_elements_sum(struct HashSet *H){
    int i;
	long long int total_sum = 0;

    for(i=0;i<H->capacity;i++) {
		long long int ll = list_sum(H->table[i]);
		//printf("Bucket %6d: %8u elements\n", i, ll);
		total_sum += ll;
	}
	//printf("Total: %8u\n", total_elements);
    return total_sum;
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

void initialize(struct HashSet *H,int size,int starting_locks){
    //NOTE: change locks granuallity here    
    //if (size>10000000) size = 10000000;
    _initialize(H,size,starting_locks);
}


int Insert(struct HashSet *H,int key,params_t *params){
    return add(H,key,key,0,params);
}

int Lookup(struct HashSet *H,int key,params_t *params){
    return contains(H,key,key,params);
}

int Erase(struct HashSet *H,int key,params_t *params){
    return _delete(H,key,key,params);
}
#endif /*__STRIPED_H*/
