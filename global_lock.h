#ifndef __GLOBAL_LOCK_H
#define  __GLOBAL_LOCK_H

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
//    int lock;
	pthread_spinlock_t lock;
};

#define lock_init(H)  pthread_spin_init(&H->lock, PTHREAD_PROCESS_SHARED)
#define lock_set(H)   pthread_spin_lock(&H->lock)
#define unlock_set(H) pthread_spin_unlock(&H->lock)

//void lock_set (struct HashSet * H){
//
//
//    while (1){
//        if (!H->lock){
//            if(!__sync_lock_test_and_set(&(H->lock),1)) break;
//        }
//    }
//}

//void unlock_set(struct HashSet * H){
//
//    H->lock = 0;
//}




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

//search value in bucket;
int list_search(struct node_t * Head,int val){
    
    struct node_t * curr;
    
    curr=Head;
    while(curr){
        if(curr->value==val) return 1;
        curr=curr->next;
    }
    return 0;
}


//add value in bucket;
//NOTE: duplicate values are allowed...
void list_add(struct HashSet * H, int key,int val,int hash_code){
    
    struct node_t * curr;
    struct node_t * next;
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
    node->value=val;
    node->hash_code=hash_code;
    if(H->table[key]==NULL) node->next=NULL;
    else node->next=H->table[key];
    H->table[key]=node;
}


// delete from bucket. The fist value equal to val will be deleted
int list_delete(struct HashSet *H,int key,int val){
    
    struct node_t * curr;
    struct node_t * next;
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
        prev=curr;
        curr=curr->next;
    }
    return 0;
}





void initialize(struct HashSet * H, int capacity){
    
    int i;
//    H->lock=0;
	lock_init(H);
    H->setSize=0;
    H->capacity=capacity;
    H->table = (struct node_t **)malloc(sizeof(struct node_t *)*capacity);
    for(i=0;i<capacity;i++){
        H->table[i]=NULL;
    }
}


int policy(struct HashSet *H){
//    return ((H->setSize/H->capacity) >4);
	return 0;
}

void resize(struct HashSet *);

int contains(struct HashSet *H,int hash_code, int val, params_t *params){
    
	tsc_start(&params->insert_lock_set_tsc);
    lock_set(H);
	tsc_pause(&params->insert_lock_set_tsc);
    int bucket_index = hash_code % H->capacity;
    int res=list_search(H->table[bucket_index],val);
	tsc_start(&params->insert_lock_set_tsc);
    unlock_set(H);
	tsc_pause(&params->insert_lock_set_tsc);
    return res;
}

//reentrant ==1 means we must not lock( we are calling from resize so we have already locked the data structure)
void add(struct HashSet *H,int hash_code, int val, int reentrant, params_t *params){
    
    if(!reentrant) {
		tsc_start(&params->insert_lock_set_tsc);
		lock_set(H);
		tsc_pause(&params->insert_lock_set_tsc);
	}
    int bucket_index = hash_code % H->capacity;
    list_add(H,bucket_index,val,hash_code);
    H->setSize++;
    if(!reentrant) {
		tsc_start(&params->insert_lock_set_tsc);
		unlock_set(H);
		tsc_pause(&params->insert_lock_set_tsc);
	}
    if (policy(H)) resize(H);
}

int _delete(struct HashSet *H,int hash_code, int val, params_t *params){
    
	tsc_start(&params->insert_lock_set_tsc);
    lock_set(H);
	tsc_pause(&params->insert_lock_set_tsc);
    int bucket_index =  hash_code % H->capacity;
    int res=list_delete(H,bucket_index,val);
    H->setSize--;
	tsc_start(&params->insert_lock_set_tsc);
    unlock_set(H);
	tsc_pause(&params->insert_lock_set_tsc);
    return res;
}


void resize(struct HashSet *H){
    
    int i;
    struct node_t * curr;
    int old_capacity = H->capacity;
    lock_set(H);
    if(old_capacity!=H->capacity){
        unlock_set(H);
        return; //somebody beat us to it
    }
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
                add(H,hash_code,val,1, NULL);
                curr=curr->next;
        }
    }
    free(old_table);
    unlock_set(H);
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

int Insert(struct HashSet *H,int key, params_t *params){
    add(H,key,key,0, params);
    return 1;
}

int Lookup(struct HashSet *H,int key, params_t *params){
    return contains(H,key,key, params);
}

int Erase(struct HashSet *H, int key, params_t *params){
    return _delete(H,key,key, params);
}
#endif /* __GLOBAL_LOCK_H*/
