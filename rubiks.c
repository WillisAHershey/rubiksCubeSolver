#include <stdio.h>
#include <stdlib.h>
#include <time.h>

//The following preprocessor directive allows older POSIX systems that don't have compiler support for the C11 threads.h library to compile using pthread.h
#ifdef __STDC_NO_THREADS__
#	define THREAD_RETURN void*
#	include <pthread.h>
#	define thrd_t pthread_t
#	define thrd_create(a,b,c) pthread_create(a,NULL,b,c)
#	define thrd_join(a,b) pthread_join(a,b)
#	define mtx_t pthread_mutex_t
#	define mtx_init(a,b) pthread_mutex_init(a,NULL)
#	define mtx_lock(a) pthread_mutex_lock(a)
#	define mtx_unlock(a) pthread_mutex_unlock(a)
#	define mtx_destroy(a) pthread_mutex_destroy(a)
#else
#	define THREAD_RETURN int
#	include <threads.h>
#endif

#include <unistd.h>
#include <string.h>

#define NUM_THREADS 8

//enum color represents a relationship between the colors of a rubiks cube and integers 0-5
enum color {white=0,blue=1,green=2,yellow=3,red=4,orange=5,w=0,b=1,g=2,y=3,r=4,o=5};

//A state is 48 colors in a very specific order, which defines the placement of all of the colors on the cube
typedef struct stateStruct{
  enum color c[48];
}state;

//Returns tier where difference occurs times negative one if a<b, 0 if a=b, and positive tier where difference occurs if a>b. This allows us to order states
static inline int compareStates(state *a,state *b,int tier){
  for(int i=tier;i<48;++i)
	if(a->c[i]!=b->c[i])
		return a->c[i]<b->c[i]?-1*(i+1):i+1;
  return 0;
}

//This allows us to print states for debugging purposes
void printState(state *in){
  printf("{");
  for(int c=0;c<48;++c){
	switch(in->c[c]){
	case white:
		printf("w");
		break;
	case blue:
		printf("b");
		break;
	case green:
		printf("g");
		break;
	case yellow:
		printf("y");
		break;
	case red:
		printf("r");
		break;
	case orange:
		printf("o");
		break;
	}
	if(c!=47)
		printf(",");
  }
  printf("}\n");
}

//This is the solved state, the state we attempt to achieve
const state solved=(state){.c={white,white,white,white,white,white,white,white,blue,blue,blue,blue,blue,blue,blue,blue,green,green,green,green,green,green,green,green,yellow,yellow,yellow,yellow,yellow,yellow,yellow,yellow,red,red,red,red,red,red,red,red,orange,orange,orange,orange,orange,orange,orange,orange}}; 

//Contains a state, pointers to all 18 possible children states (in the order of the functions below), tier of node in tree, and side it was computed on by parent
typedef struct stateTreeNodeStruct{
  state s;
  unsigned char tier;
  unsigned char side;
  struct stateTreeNodeStruct *children[18];
}stateTreeNode;

//Linked list node for BFS queue
typedef struct treeQueueNodeStruct{
  stateTreeNode *node;
  struct treeQueueNodeStruct *next;
}treeQueueNode;

//Thread-safe structure for BFS queue
typedef struct treeQueueStruct{
  treeQueueNode *head;
  treeQueueNode *tail;
  mtx_t mutex;
}treeQueue;

//Structure/node for ordered linked-list of visited states
typedef struct stateListNodeStruct{
  state *s[6];
  struct stateListNodeStruct *next[6];
}stateListNode;

typedef struct stateListStruct{
  stateListNode *head;
  mtx_t mutex;
}stateList;

//This is meant to be a literal stored in the data section of the executable, so we can memcpy() NULLs into an array, instead of using a loop which probs saves time
const stateTreeNode *eightteenNulls[]={NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
//Same story here. This is to use memcpy() on new stateListNodes in the m-ary tree stateList
const stateListNode emptyStateListNode=(stateListNode){.s={NULL,NULL,NULL,NULL,NULL,NULL},.next={NULL,NULL,NULL,NULL,NULL,NULL}};

//The following eightteen functions mutate the input state to simulate some specific turn of the cube and save the resulting state at the given address
//These functions receive a state as an input, and write that state mutated in their own way to some given memory location
//Despite their one-lined-ness, they are each rather expensive functions

void faceClock(state in,state *out){
  *out=(state){.c={in.c[5],in.c[3],in.c[0],in.c[6],in.c[1],in.c[7],in.c[4],in.c[2],in.c[40],in.c[41],in.c[42],in.c[11],in.c[12],in.c[13],in.c[14],in.c[15],in.c[32],in.c[33],in.c[34],in.c[19],in.c[20],in.c[21],in.c[22],in.c[23],in.c[24],in.c[25],in.c[26],in.c[27],in.c[28],in.c[29],in.c[30],in.c[31],in.c[8],in.c[9],in.c[10],in.c[35],in.c[36],in.c[37],in.c[38],in.c[39],in.c[16],in.c[17],in.c[18],in.c[43],in.c[44],in.c[45],in.c[46],in.c[47]}};
}

void faceCounter(state in,state *out){
  *out=(state){.c={in.c[2],in.c[4],in.c[7],in.c[1],in.c[6],in.c[0],in.c[3],in.c[5],in.c[32],in.c[33],in.c[34],in.c[11],in.c[12],in.c[13],in.c[14],in.c[15],in.c[40],in.c[41],in.c[42],in.c[19],in.c[20],in.c[21],in.c[22],in.c[23],in.c[24],in.c[25],in.c[26],in.c[27],in.c[28],in.c[29],in.c[30],in.c[31],in.c[16],in.c[17],in.c[18],in.c[35],in.c[36],in.c[37],in.c[38],in.c[39],in.c[8],in.c[9],in.c[10],in.c[43],in.c[44],in.c[45],in.c[46],in.c[47]}};
}

void faceTwice(state in,state *out){
  *out=(state){.c={in.c[7],in.c[6],in.c[5],in.c[4],in.c[3],in.c[2],in.c[1],in.c[0],in.c[16],in.c[17],in.c[18],in.c[11],in.c[12],in.c[13],in.c[14],in.c[15],in.c[8],in.c[9],in.c[10],in.c[19],in.c[20],in.c[21],in.c[22],in.c[23],in.c[24],in.c[25],in.c[26],in.c[27],in.c[28],in.c[29],in.c[30],in.c[31],in.c[40],in.c[41],in.c[42],in.c[35],in.c[36],in.c[37],in.c[38],in.c[39],in.c[32],in.c[33],in.c[34],in.c[43],in.c[44],in.c[45],in.c[46],in.c[47]}};
}

void leftClock(state in,state *out){
  *out=(state){.c={in.c[39],in.c[1],in.c[2],in.c[36],in.c[4],in.c[34],in.c[6],in.c[7],in.c[13],in.c[11],in.c[8],in.c[14],in.c[9],in.c[15],in.c[12],in.c[10],in.c[16],in.c[17],in.c[18],in.c[19],in.c[20],in.c[21],in.c[22],in.c[23],in.c[24],in.c[25],in.c[45],in.c[27],in.c[43],in.c[29],in.c[30],in.c[40],in.c[32],in.c[33],in.c[26],in.c[35],in.c[28],in.c[37],in.c[38],in.c[31],in.c[0],in.c[41],in.c[42],in.c[3],in.c[44],in.c[5],in.c[46],in.c[47]}};
}

void leftCounter(state in,state *out){
  *out=(state){.c={in.c[40],in.c[1],in.c[2],in.c[43],in.c[4],in.c[45],in.c[6],in.c[7],in.c[10],in.c[12],in.c[15],in.c[9],in.c[14],in.c[8],in.c[11],in.c[13],in.c[16],in.c[17],in.c[18],in.c[19],in.c[20],in.c[21],in.c[22],in.c[23],in.c[24],in.c[25],in.c[34],in.c[27],in.c[36],in.c[29],in.c[30],in.c[39],in.c[32],in.c[33],in.c[5],in.c[35],in.c[3],in.c[37],in.c[38],in.c[0],in.c[31],in.c[41],in.c[42],in.c[28],in.c[44],in.c[26],in.c[46],in.c[47]}};
}

void leftTwice(state in,state *out){
  *out=(state){.c={in.c[31],in.c[1],in.c[2],in.c[28],in.c[4],in.c[26],in.c[6],in.c[7],in.c[15],in.c[14],in.c[13],in.c[12],in.c[11],in.c[10],in.c[9],in.c[8],in.c[16],in.c[17],in.c[18],in.c[19],in.c[20],in.c[21],in.c[22],in.c[23],in.c[24],in.c[25],in.c[5],in.c[27],in.c[3],in.c[29],in.c[30],in.c[0],in.c[32],in.c[33],in.c[45],in.c[35],in.c[43],in.c[37],in.c[38],in.c[40],in.c[39],in.c[41],in.c[42],in.c[36],in.c[44],in.c[34],in.c[46],in.c[47]}};
}

void rightClock(state in,state *out){
  *out=(state){.c={in.c[0],in.c[1],in.c[42],in.c[3],in.c[44],in.c[5],in.c[6],in.c[47],in.c[8],in.c[9],in.c[10],in.c[11],in.c[12],in.c[13],in.c[14],in.c[15],in.c[21],in.c[19],in.c[16],in.c[22],in.c[17],in.c[23],in.c[20],in.c[18],in.c[32],in.c[25],in.c[26],in.c[35],in.c[28],in.c[37],in.c[30],in.c[31],in.c[7],in.c[33],in.c[34],in.c[4],in.c[36],in.c[2],in.c[38],in.c[39],in.c[40],in.c[41],in.c[29],in.c[43],in.c[27],in.c[45],in.c[46],in.c[24]}};
}

void rightCounter(state in,state *out){
  *out=(state){.c={in.c[0],in.c[1],in.c[37],in.c[3],in.c[35],in.c[5],in.c[6],in.c[32],in.c[8],in.c[9],in.c[10],in.c[11],in.c[12],in.c[13],in.c[14],in.c[15],in.c[18],in.c[20],in.c[23],in.c[17],in.c[22],in.c[16],in.c[19],in.c[21],in.c[47],in.c[25],in.c[26],in.c[44],in.c[28],in.c[42],in.c[30],in.c[31],in.c[24],in.c[33],in.c[34],in.c[27],in.c[36],in.c[29],in.c[38],in.c[39],in.c[40],in.c[41],in.c[2],in.c[43],in.c[4],in.c[45],in.c[46],in.c[7]}};
}

void rightTwice(state in,state *out){
  *out=(state){.c={in.c[0],in.c[1],in.c[29],in.c[3],in.c[27],in.c[5],in.c[6],in.c[24],in.c[8],in.c[9],in.c[10],in.c[11],in.c[12],in.c[13],in.c[14],in.c[15],in.c[23],in.c[22],in.c[21],in.c[20],in.c[19],in.c[18],in.c[17],in.c[16],in.c[7],in.c[25],in.c[26],in.c[4],in.c[28],in.c[2],in.c[30],in.c[31],in.c[47],in.c[33],in.c[34],in.c[44],in.c[36],in.c[42],in.c[38],in.c[39],in.c[40],in.c[41],in.c[37],in.c[43],in.c[35],in.c[45],in.c[46],in.c[32]}};
}

void rearClock(state in,state *out){
  *out=(state){.c={in.c[0],in.c[1],in.c[2],in.c[3],in.c[4],in.c[5],in.c[6],in.c[7],in.c[8],in.c[9],in.c[10],in.c[11],in.c[12],in.c[37],in.c[38],in.c[39],in.c[16],in.c[17],in.c[18],in.c[19],in.c[20],in.c[45],in.c[46],in.c[47],in.c[29],in.c[27],in.c[24],in.c[30],in.c[25],in.c[31],in.c[28],in.c[26],in.c[32],in.c[33],in.c[34],in.c[35],in.c[36],in.c[21],in.c[22],in.c[23],in.c[40],in.c[41],in.c[42],in.c[43],in.c[44],in.c[13],in.c[14],in.c[15]}};
}

void rearCounter(state in,state *out){
  *out=(state){.c={in.c[0],in.c[1],in.c[2],in.c[3],in.c[4],in.c[5],in.c[6],in.c[7],in.c[8],in.c[9],in.c[10],in.c[11],in.c[12],in.c[45],in.c[46],in.c[47],in.c[16],in.c[17],in.c[18],in.c[19],in.c[20],in.c[37],in.c[38],in.c[39],in.c[26],in.c[28],in.c[31],in.c[25],in.c[30],in.c[24],in.c[27],in.c[29],in.c[32],in.c[33],in.c[34],in.c[35],in.c[36],in.c[13],in.c[14],in.c[15],in.c[40],in.c[41],in.c[42],in.c[43],in.c[44],in.c[21],in.c[22],in.c[23]}};
}

void rearTwice(state in,state *out){
  *out=(state){.c={in.c[0],in.c[1],in.c[2],in.c[3],in.c[4],in.c[5],in.c[6],in.c[7],in.c[8],in.c[9],in.c[10],in.c[11],in.c[12],in.c[21],in.c[22],in.c[23],in.c[16],in.c[17],in.c[18],in.c[19],in.c[20],in.c[13],in.c[14],in.c[15],in.c[31],in.c[30],in.c[29],in.c[28],in.c[27],in.c[26],in.c[25],in.c[24],in.c[32],in.c[33],in.c[34],in.c[35],in.c[36],in.c[45],in.c[46],in.c[47],in.c[40],in.c[41],in.c[42],in.c[43],in.c[44],in.c[37],in.c[38],in.c[39]}};
}

void topClock(state in,state *out){
  *out=(state){.c={in.c[18],in.c[20],in.c[23],in.c[3],in.c[4],in.c[5],in.c[6],in.c[7],in.c[2],in.c[9],in.c[10],in.c[1],in.c[12],in.c[0],in.c[14],in.c[15],in.c[16],in.c[17],in.c[24],in.c[19],in.c[25],in.c[21],in.c[22],in.c[26],in.c[13],in.c[11],in.c[8],in.c[27],in.c[28],in.c[29],in.c[30],in.c[31],in.c[37],in.c[35],in.c[32],in.c[38],in.c[33],in.c[39],in.c[36],in.c[34],in.c[40],in.c[41],in.c[42],in.c[43],in.c[44],in.c[45],in.c[46],in.c[47]}};
}

void topCounter(state in,state *out){
  *out=(state){.c={in.c[13],in.c[11],in.c[8],in.c[3],in.c[4],in.c[5],in.c[6],in.c[7],in.c[26],in.c[9],in.c[10],in.c[25],in.c[12],in.c[24],in.c[14],in.c[15],in.c[16],in.c[17],in.c[0],in.c[19],in.c[1],in.c[21],in.c[22],in.c[2],in.c[18],in.c[20],in.c[23],in.c[27],in.c[28],in.c[29],in.c[30],in.c[31],in.c[34],in.c[36],in.c[39],in.c[33],in.c[38],in.c[32],in.c[35],in.c[37],in.c[40],in.c[41],in.c[42],in.c[43],in.c[44],in.c[45],in.c[46],in.c[47]}};
}

void topTwice(state in,state *out){
  *out=(state){.c={in.c[24],in.c[25],in.c[26],in.c[3],in.c[4],in.c[5],in.c[6],in.c[7],in.c[23],in.c[9],in.c[10],in.c[20],in.c[12],in.c[18],in.c[14],in.c[15],in.c[16],in.c[17],in.c[13],in.c[19],in.c[11],in.c[21],in.c[22],in.c[8],in.c[0],in.c[1],in.c[2],in.c[27],in.c[28],in.c[29],in.c[30],in.c[31],in.c[39],in.c[38],in.c[37],in.c[36],in.c[35],in.c[34],in.c[33],in.c[32],in.c[40],in.c[41],in.c[42],in.c[43],in.c[44],in.c[45],in.c[46],in.c[47]}};
}

void bottomClock(state in,state *out){
  *out=(state){.c={in.c[0],in.c[1],in.c[2],in.c[3],in.c[4],in.c[15],in.c[12],in.c[10],in.c[8],in.c[9],in.c[31],in.c[11],in.c[30],in.c[13],in.c[14],in.c[29],in.c[5],in.c[17],in.c[18],in.c[6],in.c[20],in.c[7],in.c[22],in.c[23],in.c[24],in.c[25],in.c[26],in.c[27],in.c[28],in.c[16],in.c[19],in.c[21],in.c[32],in.c[33],in.c[34],in.c[35],in.c[36],in.c[37],in.c[38],in.c[39],in.c[45],in.c[43],in.c[40],in.c[46],in.c[41],in.c[47],in.c[44],in.c[42]}};
}

void bottomCounter(state in,state *out){
  *out=(state){.c={in.c[0],in.c[1],in.c[2],in.c[3],in.c[4],in.c[16],in.c[19],in.c[21],in.c[8],in.c[9],in.c[7],in.c[11],in.c[6],in.c[13],in.c[14],in.c[5],in.c[29],in.c[17],in.c[18],in.c[30],in.c[20],in.c[31],in.c[22],in.c[23],in.c[24],in.c[25],in.c[26],in.c[27],in.c[28],in.c[15],in.c[12],in.c[10],in.c[32],in.c[33],in.c[34],in.c[35],in.c[36],in.c[37],in.c[38],in.c[39],in.c[42],in.c[44],in.c[47],in.c[41],in.c[46],in.c[40],in.c[43],in.c[45]}};
}

void bottomTwice(state in,state *out){
  *out=(state){.c={in.c[0],in.c[1],in.c[2],in.c[3],in.c[4],in.c[29],in.c[30],in.c[31],in.c[8],in.c[9],in.c[21],in.c[11],in.c[19],in.c[13],in.c[14],in.c[16],in.c[15],in.c[17],in.c[18],in.c[12],in.c[20],in.c[10],in.c[22],in.c[23],in.c[24],in.c[25],in.c[26],in.c[27],in.c[28],in.c[5],in.c[6],in.c[7],in.c[32],in.c[33],in.c[34],in.c[35],in.c[36],in.c[37],in.c[38],in.c[39],in.c[47],in.c[46],in.c[45],in.c[44],in.c[43],in.c[42],in.c[41],in.c[40]}};
}

//This array of function pointers allows us to invoke the above 18 functions in loops
void (*transformations[])(state,state*)={faceClock,faceCounter,faceTwice,leftClock,leftCounter,leftTwice,rightClock,rightCounter,rightTwice,rearClock,rearCounter,rearTwice,topClock,topCounter,topTwice,bottomClock,bottomCounter,bottomTwice};

//String descriptions of the transformations in the array of function pointers are stored here in the same order for simplicity and rhyme/reason's sake
char *descriptions[]={"Face clockwise","Face counter-clockwise","Face twice","Left clockwise","Left counter-clockwise","Left twice","Right clockwise","Right counter-clockwise","Right Twice","Rear clockwise","Rear counter-clockwise","Rear twice","Top clockwise","Top counter-clockwise","Top twice","Bottom clockwise","Bottom counter-clockwise","Bottom twice"};

state shuffle(int in,int verbose){ //Performs a random virtual shuffle of some input number of moves upon the solved state and returns the shuffled state
  static int seed=0;
  if(!seed)
	srand((seed=time(NULL)));
  state out;
  int r;
  (transformations[(r=rand()%18)])(solved,&out);
  if(verbose)
	printf("%s\n",descriptions[r]);
  for(int c=0;c<in-1;++c){
	int last=r;
	r=rand()%15;
	if(last/3<=r/3)
		r+=3;
	(transformations[r])(out,&out);
	if(verbose)
		printf("%s\n",descriptions[r]);
  }
  return out;
}

//Returns a pointer to the place in the list where a pointer to the state should be stored, returns NULL if state is a duplicate.
state** addList(stateList *list,state *s){
/*The state_t* that is passed to this function is a pointer to a temporary and contantly-changing stack variable in the calling function,
  so saving that pointer in the list would be #verybad. A permament home for the state_t is also not made until this function confirms it is not a duplicate.
  To remedy this problem this function behaves ina rather unorthidox way. If the state is found to be a duplicate, the mutex is unlocked, and NULL is returned.
  If it is found to be a new unique state, memory is allocated, and placed into the list at the correct place, a pointer to that new uninitialized memory is returned,
  and it is up to the calling function to save a pointer to the permament home of the new state_t at the address returned, and also unlock the mutex after the fact.
  Is it messy? Yes. Does it work? Without problem.
  */
  //int comp, tier;
  mtx_lock(&list->mutex);
  stateListNode *pt=list->head;
  for(int tier=0;tier<48;++tier){
	if(!pt->s[s->c[tier]])
		return &pt->s[s->c[tier]];
	int comp=compareStates(s,pt->s[s->c[tier]],tier);
	if(!comp){ //If state is equal to current one in list
		mtx_unlock(&list->mutex);
		return NULL;
	}
	if(comp>0){ //if state is greater than current one in the list
		if(pt->next[s->c[tier]])
			pt=pt->next[s->c[tier]];
		else{
			stateListNode *newNode=malloc(sizeof(stateListNode));
			memcpy(newNode,&emptyStateListNode,sizeof(stateListNode));
			pt->next[s->c[tier]]=newNode;
			return &pt->next[s->c[tier]]->s[s->c[tier+1]];
		}
	}
	else{ //If state is less than current one in the list
		state **out=&pt->s[s->c[tier]];
		state *push=pt->s[s->c[tier]];
		while(pt->next[push->c[tier]]){
			pt=pt->next[push->c[tier]];
			++tier;
			if(!pt->s[push->c[tier]]){
				pt->s[push->c[tier]]=push;
				return out;
			}
			else{
				state *hold=pt->s[push->c[tier]];
				pt->s[push->c[tier]]=push;
				push=hold;
			}
		}
		stateListNode *newNode=malloc(sizeof(stateListNode));
		memcpy(newNode,&emptyStateListNode,sizeof(stateListNode));
		pt->next[push->c[tier]]=newNode;
		pt->next[push->c[tier]]->s[push->c[tier+1]]=push;
		return out;
	}
  }
  //If this is reached, it means that the list has no duplicate of this state, and there is also no place in the list to put it, which should be impossible
  fprintf(stderr,"Something went wrong. addList() reached impossible state\n");
  return NULL;
}

//The following two functions work together to recursively print all the states in the list
void printStateListNode(stateListNode *node){
  for(int c=0;c<6;++c)
  	if(node->s[c]){
		printState(node->s[c]);
		if(node->next[c])
			printStateListNode(node->next[c]);	
	}
}

void printStateList(stateList *list){
  mtx_lock(&list->mutex);
  printf("Beginning statelist:\n");
  printStateListNode(list->head);
  printf("End statelist\n");
  mtx_unlock(&list->mutex);
}

//Frees all memory associated with the stateList
void freeStateListNode(stateListNode *node){
  for(int c=0;c<6;++c)
	if(node->next[c])
  		freeStateListNode(node->next[c]);
  free(node);
}

void freeStateList(stateList *list){
  mtx_destroy(&list->mutex);
  freeStateListNode(list->head);
}

//Thread-safe function to add a pointer to a new stateTreeNode_t to the BFS queue
void treeQueueAdd(treeQueue *queue,stateTreeNode *n){
  treeQueueNode *node=malloc(sizeof(treeQueueNode));
  *node=(treeQueueNode){.node=n,.next=NULL};
  mtx_lock(&queue->mutex);
  if(queue->tail){
	queue->tail->next=node;
	queue->tail=node;
  }
  else
	queue->head=queue->tail=node;
  mtx_unlock(&queue->mutex);
}

//Thread-safe function to remove a pointer to the oldest stateTreeNode_t from the BFS queue. Blocks if the queue is empty
stateTreeNode* treeQueueRemove(treeQueue *queue){
  mtx_lock(&queue->mutex);
  if(!queue->head){
	mtx_unlock(&queue->mutex);
	return NULL;
  }
  stateTreeNode *out=queue->head->node;
  treeQueueNode *hold=queue->head;
  queue->head=hold->next;
  free(hold);
  if(!queue->head)
	queue->tail=NULL;
  mtx_unlock(&queue->mutex);
  return out;
}

//This global variable functions as a signal to all threads to stop making new nodes
//I kinda hate global varibales, tho, so there's a good chance I'll replace this mechanism with signals someday
volatile static int solutionFound=0;

state listMatch(stateList *a,stateList *b){
  //This function originally used a malloced stack for astack and bstack, but since the tree is limited to 48, I think this is less messy
  //than calling malloc() and free() constantly on a 20-byte memspace while the other threads are busy building the trees and stateLists
  struct listStack{int index;stateListNode *node;} astack[48],bstack[48];
  while(!solutionFound){
	int aindex=0,bindex=0,asindex=0,bsindex=0;
	stateListNode *anode=a->head,*bnode=b->head;
	for(;aindex<6;++aindex)
		if(anode->s[aindex])
			break;
	for(;bindex<6;++bindex)
		if(bnode->s[bindex])
			break;
	while(aindex<6&&bindex<6){
		int comp=compareStates(anode->s[aindex],bnode->s[bindex],0);
		if(!comp){
			solutionFound=1;
			return *anode->s[aindex];
		}
		else if(comp<0){
			if(anode->next[aindex]){
				astack[asindex]=(struct listStack){.index=aindex,.node=anode};
				++asindex;
				anode=anode->next[aindex];
				aindex=-1;
			}
			for(++aindex;aindex<6;++aindex)
				if(anode->s[aindex])
					break;
			while(aindex==6&&asindex>0){
				--asindex;
				aindex=astack[asindex].index;
				anode=astack[asindex].node;
				for(++aindex;aindex<6;++aindex)
					if(anode->s[aindex])
						break;
			}
		}
		else{
			if(bnode->next[bindex]){
				bstack[bsindex]=(struct listStack){.index=bindex,.node=bnode};
				++bsindex;
				bnode=bnode->next[bindex];
				bindex=-1;
			}
			for(++bindex;bindex<6;++bindex)
				if(bnode->s[bindex])
					break;
			while(bindex==6&&bsindex>0){
				--bsindex;
				bindex=bstack[bsindex].index;
				bnode=bstack[bsindex].node;
				for(++bindex;bindex<6;++bindex)
					if(bnode->s[bindex])
						break;
			}
		}
	}
  }
  //This can't happen, but the compiler requires a valid retval here because it doesn't know that
  return solved;
}

void buildOne(stateList *list,treeQueue *queue){
  stateTreeNode *node=treeQueueRemove(queue);
  while(!node)
	node=treeQueueRemove(queue);
  for(int c=0;c<18;++c){
	state hold;
	(transformations[c])(node->s,&hold);
	state **listSlip;
	if((listSlip=addList(list,&hold))){
		node->children[c]=malloc(sizeof(stateTreeNode));
		node->children[c]->s=hold;
		*listSlip=&node->children[c]->s;
		mtx_unlock(&list->mutex);
		node->children[c]->tier=node->tier+1;
		node->children[c]->side=c/3;
		treeQueueAdd(queue,node->children[c]);
	}
	else
		node->children[c]=NULL;
  }
}

//This is the struct to hold the data a thread needs to perform the buildTree function properly.
typedef struct buildTreeDataStruct{
  stateList *list;
  treeQueue *queue;
}buildTreeData;

//This allows us to keep track of which tier of the tree we're currently processing
volatile unsigned char currentTier=0;

//This is a function meant to be executed by a thread. It processes nodes from the BFS queue, and uses them to produce up to 15 more nodes to add to the queue
THREAD_RETURN buildTree(void *data){
  stateList *list=((buildTreeData*)data)->list;
  treeQueue *queue=((buildTreeData*)data)->queue;
  stateTreeNode *node;
  while(!solutionFound){
	node=treeQueueRemove(queue);
	if(!node)
		continue;
	unsigned char currentSide=node->side*3;
 	if(node->tier>currentTier){
		currentTier=node->tier;
		printf("Beginning tier %d\n",(int)currentTier);
	}
 	for(int c=0;c<18;++c){
		if(c==currentSide){
			node->children[c]=node->children[c+1]=node->children[c+2]=NULL;
			c+=2;
			continue;
		}
		state hold;
		(transformations[c])(node->s,&hold);
		state **listSlip;
		if((listSlip=addList(list,&hold))){
			node->children[c]=malloc(sizeof(stateTreeNode));
			node->children[c]->s=hold;
			*listSlip=&node->children[c]->s;
			mtx_unlock(&list->mutex);
			node->children[c]->tier=node->tier+1;
			node->children[c]->side=c/3;
			treeQueueAdd(queue,node->children[c]);
		}
		else
			node->children[c]=NULL;
	}
  }
//The pointers in all the nodes in the queue were left uninitialized, so they must be made NULL before search is done on the tree.
  while((node=treeQueueRemove(queue)))
	memcpy(node->children,eightteenNulls,sizeof eightteenNulls);
  return (THREAD_RETURN)0;
}

//This searches the mixed tree and returns a string of instructions to get to the target state of the tree. Return string must be freed
char* searchTree(stateTreeNode *tree,state *target){
  char *out=NULL;
  if(!compareStates(&tree->s,target,0)){
	out=malloc(1);
	*out='\0';
	return out;
  }
  int c;
  char *hold;
  for(c=0;c<18;++c)
	if(tree->children[c])
		if((hold=searchTree(tree->children[c],target))){
			out=malloc(strlen(hold)+strlen(descriptions[c])+2);
			sprintf(out,"%s\n%s",descriptions[c],hold);
			free(hold);
			return out;
		}
  return NULL;
}

//This searches the solved tree and prints instructions on how to get from the target state to solved
int backwardsSearchTree(stateTreeNode *tree,state *target){
  if(!compareStates(&tree->s,target,0))
	return 1;
  for(int c=0;c<18;++c)
	if(tree->children[c])
		if(backwardsSearchTree(tree->children[c],target)){
			int hold=c%3;
			if(hold==0)
				printf("%s\n",descriptions[c+1]);
			else if(hold==1)
				printf("%s\n",descriptions[c-1]);
			else
				printf("%s\n",descriptions[c]);
			return 1;
		}
  return 0;
}

//Frees all malloced nodes of the tree
void recursiveFreeTree(stateTreeNode *tree){
  if(!tree)
	return;
  int c;
  for(c=0;c<18;++c)
  	recursiveFreeTree(tree->children[c]);
  free(tree);
}

void freeTree(stateTreeNode *tree){
  int c;
  for(c=0;c<18;++c)
  	recursiveFreeTree(tree->children[c]);
}


/*Two stateTrees, two stateLists, and two treeQueues are made. One for the mixed cube's state, and one for the solution state.
 *Half of the threads work on solving the mixed cube, and the other work on shuffling the solved cube, until they encounter a common state
 *When that common state is found, all threads terminate and the trees are compared to create a list of results.
 */
void solve(state in){
  printf("Solving ");
  printState(&in);
  //state shuffled=(state_t){.c={b,o,b,y,y,y,b,g,w,g,g,r,o,w,b,r,r,r,y,o,w,o,w,b,r,g,b,g,y,w,o,w,o,w,o,b,w,y,r,r,o,r,y,b,g,g,y,g}};
  stateTreeNode fromMixed=(stateTreeNode){.s=in,.tier=0,.side=7};
  stateTreeNode fromSolved=(stateTreeNode){.s=solved,.tier=0,.side=7};
  stateList mixedList=(stateList){.head=malloc(sizeof(stateListNode))};
  memcpy(mixedList.head,&emptyStateListNode,sizeof(stateListNode));
  mtx_init(&mixedList.mutex,mtx_plain);
  *addList(&mixedList,&fromMixed.s)=&fromMixed.s;
  mtx_unlock(&mixedList.mutex);
  stateList solvedList=(stateList){.head=malloc(sizeof(stateListNode))};
  memcpy(solvedList.head,&emptyStateListNode,sizeof(stateListNode));
  mtx_init(&solvedList.mutex,mtx_plain);
  *addList(&solvedList,&fromSolved.s)=&fromSolved.s;
  mtx_unlock(&solvedList.mutex);
  treeQueue mixedQueue;
  mixedQueue.head=mixedQueue.tail=malloc(sizeof(treeQueueNode));
  *mixedQueue.head=(treeQueueNode){.node=&fromMixed,.next=NULL};
  mtx_init(&mixedQueue.mutex,mtx_plain);
  treeQueue solvedQueue;
  solvedQueue.head=solvedQueue.tail=malloc(sizeof(treeQueueNode));
  *solvedQueue.head=(treeQueueNode){.node=&fromSolved,.next=NULL};
  mtx_init(&solvedQueue.mutex,mtx_plain);
  buildOne(&mixedList,&mixedQueue);
  buildOne(&solvedList,&solvedQueue);
  buildTreeData mixedTreeData=(buildTreeData){.list=&mixedList,.queue=&mixedQueue};
  buildTreeData solvedTreeData=(buildTreeData){.list=&solvedList,.queue=&solvedQueue};
  thrd_t tids[NUM_THREADS];
  int c;
  for(c=0;c<NUM_THREADS;c+=2){
    thrd_create(&tids[c],buildTree,(void*)&mixedTreeData);
    thrd_create(&tids[c+1],buildTree,(void*)&solvedTreeData);
  }
  state link=listMatch(&mixedList,&solvedList);
  for(--c;c>-1;--c)
  	thrd_join(tids[c],NULL);
  char *string;
  if(!(string=searchTree(&fromMixed,&link)))
	printf("No solution found\n");
  else{
	string[strlen(string)-1]='\0';
	printf("%s\n",string);
	free(string);
	backwardsSearchTree(&fromSolved,&link);
  }
  freeStateList(&solvedList);
  freeStateList(&mixedList);
  freeTree(&fromSolved);
  freeTree(&fromMixed);
  mtx_destroy(&mixedQueue.mutex);
  mtx_destroy(&solvedQueue.mutex);
}

int main(int args,char *argv[]){
  if(args!=2&&args!=49){
	printf("USAGE: %s ([integer]|{w,b,g,r,o,y,...})\n",argv[0]);
  }
  else{
	int num=atoi(argv[1]);
	if(!num){
		state s;
		for(int c=1;c<49;++c){
			if(argv[c][0]=='w')
				s.c[c-1]=white;
			else if(argv[c][0]=='b')
				s.c[c-1]=blue;
			else if(argv[c][0]=='r')
				s.c[c-1]=red;
			else if(argv[c][0]=='g')
				s.c[c-1]=green;
			else if(argv[c][0]=='y')
				s.c[c-1]=yellow;
			else if(argv[c][0]=='o')
				s.c[c-1]=orange;
			else{
				printf("%c\n",argv[c][0]);
				printf("USAGE: %s ([integer]|{w,b,g,r,o,y,...})\n",argv[0]);
				return -1;
			}
		}
		solve(s);
	}
	else
		solve(shuffle(num,1));
  }
}
