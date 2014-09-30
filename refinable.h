#ifndef __REFINABLE_H
#define __REFINABLE_H

#include <pthread.h> /* pthread_spinlock */
#include "tsc.h"
#include "parallel_benchmarks.h"
// node of a list (bucket)
struct node_t{
    int value;
    int hash_code;
    struct node_t * next;
};

struct locks{
    pthread_spinlock_t * locks_array;
    int locks_length;
};

struct HashSet{
    //int length;
    struct node_t ** table;
    int capacity;
    int setSize;
    struct locks * locks_struct;
    int owner;
};

int NULL_VALUE = 5139239;


unsigned long long get_count(unsigned long long a){

    unsigned long long b = a >>63;
    return b;
}

unsigned long long get_pointer(unsigned long long a){
    unsigned long long b = a << 1;
    b= b >>1;
    return b;
}

unsigned long long set_count(unsigned long long  a, unsigned long long count){
    unsigned long long count_temp =  count << 63;
    unsigned long long b = get_pointer(a);
    b = b | count_temp;
    return b;
}

unsigned long long set_pointer(unsigned long long a, unsigned long long ptr){
    unsigned long long b = 0;
    unsigned long long c = get_count(a);
    b = set_count(b,c);
    ptr = get_pointer(ptr);
    b= b | ptr;
    return b;
}

unsigned long long set_both(unsigned long long a, unsigned long long ptr, unsigned long long count){
    a=set_pointer(a,ptr);
    a=set_count(a,count);
    return a;
}
void lock_set (pthread_spinlock_t * locks, int hash_code){

    int indx=hash_code;
    pthread_spin_lock(&locks[indx]);
    //int indx=hash_code % H->locks_length;
    /*while (1){
        if (!locks[indx]){
            if(!__sync_lock_test_and_set(&(locks[indx]),1)) break;
        }
    }
    */
 
}

void unlock_set(pthread_spinlock_t *,int);

// operations call acquire to lock
void acquire(struct HashSet *H,int hash_code,params_t *params){
    int me = params->tid;
    int who,cpy_owner,mark;
    while (1){
        cpy_owner=H->owner;
        who=get_pointer(cpy_owner);
        mark=get_count(cpy_owner);
        while((mark==1)&&(who!=me)){
            cpy_owner=H->owner;
            who=get_pointer(cpy_owner);
            mark=get_count(cpy_owner);
        }
        //int old_locks_length=H->locks_length;
        //int * old_locks=H->locks;
        struct locks * cpy_locks=H->locks_struct;
        pthread_spinlock_t * old_locks=cpy_locks->locks_array;
        int  old_locks_length =cpy_locks->locks_length;

	    tsc_start(&params->insert_lock_set_tsc);
        lock_set(old_locks,hash_code % old_locks_length);
	    tsc_pause(&params->insert_lock_set_tsc);

        cpy_owner=H->owner;
        who=get_pointer(cpy_owner);
        mark=get_count(cpy_owner);
        
        if(((!mark) || (who==me))&&(H->locks_struct==cpy_locks)){
            return;
        }
        else{
	        tsc_start(&params->insert_lock_set_tsc);
            unlock_set(old_locks,hash_code % old_locks_length);
	        tsc_pause(&params->insert_lock_set_tsc);
        }
    }

}
void unlock_set(pthread_spinlock_t * locks, int hash_code){

    int indx=hash_code;
    pthread_spin_unlock(&locks[indx]);
    //locks[indx] = 0;
}

void release(struct HashSet * H,int hash_code){ //:TODO IS IT OK?

    unlock_set(H->locks_struct->locks_array,hash_code % (H->locks_struct->locks_length));
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
    H->setSize=0;
    H->capacity=capacity;
    H->table = (struct node_t **)malloc(sizeof(struct node_t *)*capacity);
    for(i=0;i<capacity;i++){
        H->table[i]=NULL;
    }
    H->locks_struct = (struct locks *) malloc(sizeof(struct locks ));
    H->locks_struct->locks_length = capacity;
    H->locks_struct->locks_array = (pthread_spinlock_t *) malloc(sizeof(pthread_spinlock_t)* capacity);
    for(i=0;i<capacity;i++) pthread_spin_init(&H->locks_struct->locks_array[i],PTHREAD_PROCESS_SHARED);//H->locks_struct->locks_array[i*]=0;
    H->owner = set_both(H->owner,NULL_VALUE,0);

}


int policy(struct HashSet *H){
    //return ((H->setSize/H->capacity) >4000);
    return 0;
}

void resize(struct HashSet *,params_t *params);

int contains(struct HashSet *H,int hash_code, int val,params_t *params){
    
	tsc_start(&params->insert_lock_set_tsc);
    acquire(H,hash_code,params);
	tsc_pause(&params->insert_lock_set_tsc);
    int bucket_index = hash_code % H->capacity;
    int res=list_search(H->table[bucket_index],val);
	tsc_start(&params->insert_lock_set_tsc);
    release(H,hash_code);
	tsc_pause(&params->insert_lock_set_tsc);
    return res;
}

//reentrant ==1 means we must not lock( we are calling from resize so we have already locked the data structure)
void add(struct HashSet *H,int hash_code, int val, int reentrant,params_t *params){
    
    if(!reentrant) {
	    tsc_start(&params->insert_lock_set_tsc);
        acquire(H,hash_code,params);
	    tsc_pause(&params->insert_lock_set_tsc);
    }
    int bucket_index = hash_code % H->capacity;
    list_add(H,bucket_index,val,hash_code);
    //H->setSize++;
    __sync_fetch_and_add(&(H->setSize),1);
    if(!reentrant) {
	    tsc_start(&params->insert_lock_set_tsc);
        release(H,hash_code);
	    tsc_pause(&params->insert_lock_set_tsc);
    }
    if(!reentrant) {if (policy(H)) resize(H,params);}
}

int _delete(struct HashSet *H,int hash_code, int val,params_t *params){
    
	tsc_start(&params->insert_lock_set_tsc);
    acquire(H,hash_code,params);
	tsc_pause(&params->insert_lock_set_tsc);
    int bucket_index =  hash_code % H->capacity;
    int res=list_delete(H,bucket_index,val);
    //H->setSize--;
    __sync_fetch_and_sub(&(H->setSize),1);
	tsc_start(&params->insert_lock_set_tsc);
    release(H,hash_code);
	tsc_pause(&params->insert_lock_set_tsc);
    return res;
}

void quiesce(struct HashSet *H,params_t *params){
    int i;
    pthread_spinlock_t *locks= H->locks_struct->locks_array;
    for(i=0;i<H->locks_struct->locks_length;i++){
        //while(locks[i]==1); //TODO: is it a race?
	    tsc_start(&params->insert_lock_set_tsc);
        pthread_spin_lock(&locks[i]);
	    tsc_pause(&params->insert_lock_set_tsc);
    }
}

void resize(struct HashSet *H,params_t *params){
    printf("@resize!!\n");
    int i;
    int mark,me;
    struct node_t * curr;
    int old_capacity = H->capacity;
    int new_capacity =  old_capacity * 2;

    me = params->tid;
    int expected_value = set_both(expected_value,NULL_VALUE,0);
    int new_owner=set_both(new_owner,me,1);
    if(__sync_bool_compare_and_swap(&(H->owner),expected_value,new_owner)){
        
    //for(i=0;i<H->locks_length;i++) lock_set(H,i);
        if(old_capacity!=H->capacity) {
            //for(i=0;i<H->locks_length;i++) //unlock_set(H,i);
                H->owner=set_both(H->owner,NULL_VALUE,0);
                return; //somebody beat us to it
        }
        quiesce(H,params);  
        //H->locks_length = new_capacity; //in this implementetion 
                                        //locks_length == capacity
                                        //edit!!
        int new_locks_length=new_capacity;
        struct node_t ** old_table = H->table;
        H->setSize=0;
        H->table = (struct node_t **)malloc(sizeof(struct node_t *)*new_capacity);
        for(i=0;i<new_capacity;i++){
            H->table[i]=NULL;
        }
        //re hash everything from the old table to the new one
        for(i=0;i<old_capacity;i++){
        
            curr=old_table[i];
            while(curr){
                int val = curr->value;
                int hash_code = curr->hash_code;
                //int bucket_index= hash_code % new_capacity;
                add(H,hash_code,val,1,params);
                curr=curr->next;
            }
        }
        //free(old_table);
        //all locks should be free now (quiesce ensures that)
        //so we might as well delete the old ones and make new ones
        pthread_spinlock_t * old_locks = H->locks_struct->locks_array;
        //for(i=0;i<old_capacity;i++) if( H->locks_struct->locks_array[i]!=0) printf("thread %d capacity %d HEY!\n",omp_get_thread_num(),H->capacity);
        pthread_spinlock_t * new_locks = (pthread_spinlock_t *)malloc(sizeof(pthread_spinlock_t) * new_capacity);//edit!
        for(i=0;i<new_locks_length;i++) pthread_spin_init(&new_locks[i],PTHREAD_PROCESS_SHARED);
        struct locks * new_locks_struct = (struct locks *) malloc(sizeof(struct locks));
        new_locks_struct->locks_array=new_locks;
        new_locks_struct->locks_length = new_locks_length;
        H->locks_struct=new_locks_struct; //TODO: free old struct
        expected_value = new_owner;
        H->capacity =  new_capacity;
        new_owner = set_both(new_owner,NULL_VALUE,0);
        if(!__sync_bool_compare_and_swap(&(H->owner),expected_value,new_owner))
            printf("This should not have happened\n");

        //free(old_locks);
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
////////
int Insert(struct HashSet *H,int key,params_t *params){
        add(H,key,key,0,params);
        return 1;
}

int Lookup(struct HashSet *H,int key,params_t *params){
        return contains(H,key,key,params);
}

int Erase(struct HashSet *H,int key,params_t *params){
        return _delete(H,key,key,params);
}

#endif /*__REFINABLE_H*/
