#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

#define NUM_THREADS 30

//enum color represents a relationship between the colors of a rubiks cube and integers 0-5
enum color {white=0,blue=1,green=2,yellow=3,red=4,orange=5,w=0,b=1,g=2,y=3,r=4,o=5};

//A state is 48 colors in a very specific order, which defines the placement of all of the colors on the cube
typedef struct state{  
  enum color c[48];
}state_t;

//Returns -1 if a<b, 0 if a=b, and 1 if a>b. This allows us to keep our stateList in order
int compareStates(state_t *a,state_t *b){
  int i;
  for(i=0;i<48;++i)
	if(a->c[i]!=b->c[i])
		return a->c[i]<b->c[i]?-1:1;
  return 0;
}

//This allows us to print states for debugging purposes
void printState(state_t *in){
  int c;
  printf("{");
  for(c=0;c<48;++c){
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
state_t solved=(state_t){{white,white,white,white,white,white,white,white,blue,blue,blue,blue,blue,blue,blue,blue,green,green,green,green,green,green,green,green,yellow,yellow,yellow,yellow,yellow,yellow,yellow,yellow,red,red,red,red,red,red,red,red,orange,orange,orange,orange,orange,orange,orange,orange}}; 

//Contains a state, pointers to all 18 possible children states (in the order of the functions below), tier of node in tree, and side it was computed on by parent
typedef struct stateTreeNode{
  state_t state;
  struct stateTreeNode *children[18];
  unsigned char tier;
  char side;
}stateTreeNode_t;

//Linked list node for BFS queue
typedef struct treeQueueNode{
  stateTreeNode_t *node;
  struct treeQueueNode *next;
}treeQueueNode_t;

//Thread-safe structure for BFS queue
typedef struct treeQueue{
  treeQueueNode_t *head;
  treeQueueNode_t *tail;
  sem_t turn;
}treeQueue_t;

//Structure/node for ordered linked-list of visited states
typedef struct stateList{
  state_t *state;
  struct stateList *next;
//Only the head of the list will have a sem_t
  sem_t turn[0];
}stateList_t;

//This is meant to be a literall stored in the data section of the executable, so we can memcpy NULLs into an array, instead of using a loop
void *eightteenNulls[]={NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};

//The following functions mutate the input state to simulate some specific turn of the cube and save the resulting state at the given address
//Pointers are used to prevent the machine from having to copy so much, but these are still rather heavy functions.

void faceClock(state_t *in,state_t *out){
  *out=(state_t){{in->c[5],in->c[3],in->c[0],in->c[6],in->c[1],in->c[7],in->c[4],in->c[2],in->c[40],in->c[41],in->c[42],in->c[11],in->c[12],in->c[13],in->c[14],in->c[15],in->c[32],in->c[33],in->c[34],in->c[19],in->c[20],in->c[21],in->c[22],in->c[23],in->c[24],in->c[25],in->c[26],in->c[27],in->c[28],in->c[29],in->c[30],in->c[31],in->c[8],in->c[9],in->c[10],in->c[35],in->c[36],in->c[37],in->c[38],in->c[39],in->c[16],in->c[17],in->c[18],in->c[43],in->c[44],in->c[45],in->c[46],in->c[47]}};
}

void faceCounter(state_t *in,state_t *out){
  *out=(state_t){{in->c[2],in->c[4],in->c[7],in->c[1],in->c[6],in->c[0],in->c[3],in->c[5],in->c[32],in->c[33],in->c[34],in->c[11],in->c[12],in->c[13],in->c[14],in->c[15],in->c[40],in->c[41],in->c[42],in->c[19],in->c[20],in->c[21],in->c[22],in->c[23],in->c[24],in->c[25],in->c[26],in->c[27],in->c[28],in->c[29],in->c[30],in->c[31],in->c[16],in->c[17],in->c[18],in->c[35],in->c[36],in->c[37],in->c[38],in->c[39],in->c[8],in->c[9],in->c[10],in->c[43],in->c[44],in->c[45],in->c[46],in->c[47]}};
}

void faceTwice(state_t *in,state_t *out){
  *out=(state_t){{in->c[7],in->c[6],in->c[5],in->c[4],in->c[3],in->c[2],in->c[1],in->c[0],in->c[16],in->c[17],in->c[18],in->c[11],in->c[12],in->c[13],in->c[14],in->c[15],in->c[8],in->c[9],in->c[10],in->c[19],in->c[20],in->c[21],in->c[22],in->c[23],in->c[24],in->c[25],in->c[26],in->c[27],in->c[28],in->c[29],in->c[30],in->c[31],in->c[40],in->c[41],in->c[42],in->c[35],in->c[36],in->c[37],in->c[38],in->c[39],in->c[32],in->c[33],in->c[34],in->c[43],in->c[44],in->c[45],in->c[46],in->c[47]}};
}

void leftClock(state_t *in,state_t *out){
  *out=(state_t){{in->c[39],in->c[1],in->c[2],in->c[36],in->c[4],in->c[34],in->c[6],in->c[7],in->c[13],in->c[11],in->c[8],in->c[14],in->c[9],in->c[15],in->c[12],in->c[10],in->c[16],in->c[17],in->c[18],in->c[19],in->c[20],in->c[21],in->c[22],in->c[23],in->c[24],in->c[25],in->c[45],in->c[27],in->c[43],in->c[29],in->c[30],in->c[40],in->c[32],in->c[33],in->c[26],in->c[35],in->c[28],in->c[37],in->c[38],in->c[31],in->c[0],in->c[41],in->c[42],in->c[3],in->c[44],in->c[5],in->c[46],in->c[47]}};
}

void leftCounter(state_t *in,state_t *out){
  *out=(state_t){{in->c[40],in->c[1],in->c[2],in->c[43],in->c[4],in->c[45],in->c[6],in->c[7],in->c[10],in->c[12],in->c[15],in->c[9],in->c[14],in->c[8],in->c[11],in->c[13],in->c[16],in->c[17],in->c[18],in->c[19],in->c[20],in->c[21],in->c[22],in->c[23],in->c[24],in->c[25],in->c[34],in->c[27],in->c[36],in->c[29],in->c[30],in->c[39],in->c[32],in->c[33],in->c[5],in->c[35],in->c[3],in->c[37],in->c[38],in->c[0],in->c[31],in->c[41],in->c[42],in->c[28],in->c[44],in->c[26],in->c[46],in->c[47]}};
}

void leftTwice(state_t *in,state_t *out){
  *out=(state_t){{in->c[31],in->c[1],in->c[2],in->c[28],in->c[4],in->c[26],in->c[6],in->c[7],in->c[15],in->c[14],in->c[13],in->c[12],in->c[11],in->c[10],in->c[9],in->c[8],in->c[16],in->c[17],in->c[18],in->c[19],in->c[20],in->c[21],in->c[22],in->c[23],in->c[24],in->c[25],in->c[5],in->c[27],in->c[3],in->c[29],in->c[30],in->c[0],in->c[32],in->c[33],in->c[45],in->c[35],in->c[43],in->c[37],in->c[38],in->c[40],in->c[39],in->c[41],in->c[42],in->c[36],in->c[44],in->c[34],in->c[46],in->c[47]}};
}

void rightClock(state_t *in,state_t *out){
  *out=(state_t){{in->c[0],in->c[1],in->c[42],in->c[3],in->c[44],in->c[5],in->c[6],in->c[47],in->c[8],in->c[9],in->c[10],in->c[11],in->c[12],in->c[13],in->c[14],in->c[15],in->c[21],in->c[19],in->c[16],in->c[22],in->c[17],in->c[23],in->c[20],in->c[18],in->c[32],in->c[25],in->c[26],in->c[35],in->c[28],in->c[37],in->c[30],in->c[31],in->c[7],in->c[33],in->c[34],in->c[4],in->c[36],in->c[2],in->c[38],in->c[39],in->c[40],in->c[41],in->c[29],in->c[43],in->c[27],in->c[45],in->c[46],in->c[24]}};
}

void rightCounter(state_t *in,state_t *out){
  *out=(state_t){{in->c[0],in->c[1],in->c[37],in->c[3],in->c[35],in->c[5],in->c[6],in->c[32],in->c[8],in->c[9],in->c[10],in->c[11],in->c[12],in->c[13],in->c[14],in->c[15],in->c[18],in->c[20],in->c[23],in->c[17],in->c[22],in->c[16],in->c[19],in->c[21],in->c[47],in->c[25],in->c[26],in->c[44],in->c[28],in->c[42],in->c[30],in->c[31],in->c[24],in->c[33],in->c[34],in->c[27],in->c[36],in->c[29],in->c[38],in->c[39],in->c[40],in->c[41],in->c[2],in->c[43],in->c[4],in->c[45],in->c[46],in->c[7]}};
}

void rightTwice(state_t *in,state_t *out){
  *out=(state_t){{in->c[0],in->c[1],in->c[29],in->c[3],in->c[27],in->c[5],in->c[6],in->c[24],in->c[8],in->c[9],in->c[10],in->c[11],in->c[12],in->c[13],in->c[14],in->c[15],in->c[23],in->c[22],in->c[21],in->c[20],in->c[19],in->c[18],in->c[17],in->c[16],in->c[7],in->c[25],in->c[26],in->c[4],in->c[28],in->c[2],in->c[30],in->c[31],in->c[47],in->c[33],in->c[34],in->c[44],in->c[36],in->c[42],in->c[38],in->c[39],in->c[40],in->c[41],in->c[37],in->c[43],in->c[35],in->c[45],in->c[46],in->c[32]}};
}

void rearClock(state_t *in,state_t *out){
  *out=(state_t){{in->c[0],in->c[1],in->c[2],in->c[3],in->c[4],in->c[5],in->c[6],in->c[7],in->c[8],in->c[9],in->c[10],in->c[11],in->c[12],in->c[37],in->c[38],in->c[39],in->c[16],in->c[17],in->c[18],in->c[19],in->c[20],in->c[45],in->c[46],in->c[47],in->c[29],in->c[27],in->c[24],in->c[30],in->c[25],in->c[31],in->c[28],in->c[26],in->c[32],in->c[33],in->c[34],in->c[35],in->c[36],in->c[21],in->c[22],in->c[23],in->c[40],in->c[41],in->c[42],in->c[43],in->c[44],in->c[13],in->c[14],in->c[15]}};
}

void rearCounter(state_t *in,state_t *out){
  *out=(state_t){{in->c[0],in->c[1],in->c[2],in->c[3],in->c[4],in->c[5],in->c[6],in->c[7],in->c[8],in->c[9],in->c[10],in->c[11],in->c[12],in->c[45],in->c[46],in->c[47],in->c[16],in->c[17],in->c[18],in->c[19],in->c[20],in->c[37],in->c[38],in->c[39],in->c[26],in->c[28],in->c[31],in->c[25],in->c[30],in->c[24],in->c[27],in->c[29],in->c[32],in->c[33],in->c[34],in->c[35],in->c[36],in->c[13],in->c[14],in->c[15],in->c[40],in->c[41],in->c[42],in->c[43],in->c[44],in->c[21],in->c[22],in->c[23]}};
}

void rearTwice(state_t *in,state_t *out){
  *out=(state_t){{in->c[0],in->c[1],in->c[2],in->c[3],in->c[4],in->c[5],in->c[6],in->c[7],in->c[8],in->c[9],in->c[10],in->c[11],in->c[12],in->c[21],in->c[22],in->c[23],in->c[16],in->c[17],in->c[18],in->c[19],in->c[20],in->c[13],in->c[14],in->c[15],in->c[31],in->c[30],in->c[29],in->c[28],in->c[27],in->c[26],in->c[25],in->c[24],in->c[32],in->c[33],in->c[34],in->c[35],in->c[36],in->c[45],in->c[46],in->c[47],in->c[40],in->c[41],in->c[42],in->c[43],in->c[44],in->c[37],in->c[38],in->c[39]}};
}

void topClock(state_t *in,state_t *out){
  *out=(state_t){{in->c[18],in->c[20],in->c[23],in->c[3],in->c[4],in->c[5],in->c[6],in->c[7],in->c[2],in->c[9],in->c[10],in->c[1],in->c[12],in->c[0],in->c[14],in->c[15],in->c[16],in->c[17],in->c[24],in->c[19],in->c[25],in->c[21],in->c[22],in->c[26],in->c[13],in->c[11],in->c[8],in->c[27],in->c[28],in->c[29],in->c[30],in->c[31],in->c[37],in->c[35],in->c[32],in->c[38],in->c[33],in->c[39],in->c[36],in->c[34],in->c[40],in->c[41],in->c[42],in->c[43],in->c[44],in->c[45],in->c[46],in->c[47]}};
}

void topCounter(state_t *in,state_t *out){
  *out=(state_t){{in->c[13],in->c[11],in->c[8],in->c[3],in->c[4],in->c[5],in->c[6],in->c[7],in->c[26],in->c[9],in->c[10],in->c[25],in->c[12],in->c[24],in->c[14],in->c[15],in->c[16],in->c[17],in->c[0],in->c[19],in->c[1],in->c[21],in->c[22],in->c[2],in->c[18],in->c[20],in->c[23],in->c[27],in->c[28],in->c[29],in->c[30],in->c[31],in->c[34],in->c[36],in->c[39],in->c[33],in->c[38],in->c[32],in->c[35],in->c[37],in->c[40],in->c[41],in->c[42],in->c[43],in->c[44],in->c[45],in->c[46],in->c[47]}};
}

void topTwice(state_t *in,state_t *out){
  *out=(state_t){{in->c[24],in->c[25],in->c[26],in->c[3],in->c[4],in->c[5],in->c[6],in->c[7],in->c[23],in->c[9],in->c[10],in->c[20],in->c[12],in->c[18],in->c[14],in->c[15],in->c[16],in->c[17],in->c[13],in->c[19],in->c[11],in->c[21],in->c[22],in->c[8],in->c[0],in->c[1],in->c[2],in->c[27],in->c[28],in->c[29],in->c[30],in->c[31],in->c[39],in->c[38],in->c[37],in->c[36],in->c[35],in->c[34],in->c[33],in->c[32],in->c[40],in->c[41],in->c[42],in->c[43],in->c[44],in->c[45],in->c[46],in->c[47]}};
}

void bottomClock(state_t *in,state_t *out){
  *out=(state_t){{in->c[0],in->c[1],in->c[2],in->c[3],in->c[4],in->c[15],in->c[12],in->c[10],in->c[8],in->c[9],in->c[31],in->c[11],in->c[30],in->c[13],in->c[14],in->c[29],in->c[5],in->c[17],in->c[18],in->c[6],in->c[20],in->c[7],in->c[22],in->c[23],in->c[24],in->c[25],in->c[26],in->c[27],in->c[28],in->c[16],in->c[19],in->c[21],in->c[32],in->c[33],in->c[34],in->c[35],in->c[36],in->c[37],in->c[38],in->c[39],in->c[45],in->c[43],in->c[40],in->c[46],in->c[41],in->c[47],in->c[44],in->c[42]}};
}

void bottomCounter(state_t *in,state_t *out){
  *out=(state_t){{in->c[0],in->c[1],in->c[2],in->c[3],in->c[4],in->c[16],in->c[19],in->c[21],in->c[8],in->c[9],in->c[7],in->c[11],in->c[6],in->c[13],in->c[14],in->c[5],in->c[29],in->c[17],in->c[18],in->c[30],in->c[20],in->c[31],in->c[22],in->c[23],in->c[24],in->c[25],in->c[26],in->c[27],in->c[28],in->c[15],in->c[12],in->c[10],in->c[32],in->c[33],in->c[34],in->c[35],in->c[36],in->c[37],in->c[38],in->c[39],in->c[42],in->c[44],in->c[47],in->c[41],in->c[46],in->c[40],in->c[43],in->c[45]}};
}

void bottomTwice(state_t *in,state_t *out){
  *out=(state_t){{in->c[0],in->c[1],in->c[2],in->c[3],in->c[4],in->c[29],in->c[30],in->c[31],in->c[8],in->c[9],in->c[21],in->c[11],in->c[19],in->c[13],in->c[14],in->c[16],in->c[15],in->c[17],in->c[18],in->c[12],in->c[20],in->c[10],in->c[22],in->c[23],in->c[24],in->c[25],in->c[26],in->c[27],in->c[28],in->c[5],in->c[6],in->c[7],in->c[32],in->c[33],in->c[34],in->c[35],in->c[36],in->c[37],in->c[38],in->c[39],in->c[47],in->c[46],in->c[45],in->c[44],in->c[43],in->c[42],in->c[41],in->c[40]}};
}

//Pointers to the above transformations are in this array, so they can be invoked in loops
void (*transformations[])(state_t*,state_t*)={faceClock,faceCounter,faceTwice,leftClock,leftCounter,leftTwice,rightClock,rightCounter,rightTwice,rearClock,rearCounter,rearTwice,topClock,topCounter,topTwice,bottomClock,bottomCounter,bottomTwice};

//String descriptions of the transformations are stored in the same order for simplicity and rhyme/reason's sake
char *descriptions[]={"Face clockwise","Face counter-clockwise","Face twice","Left clockwise","Left counter-clockwise","Left twice","Right clockwise","Right counter-clockwise","Right Twice","Rear clockwise","Rear counter-clockwise","Rear twice","Top clockwise","Top counter-clockwise","Top twice","Bottom clockwise","Bottom counter-clockwise","Bottom twice"};

int seed=0;

state_t shuffle(int in,int verbose){ //Performs a random virtual shuffle of some input number of moves upon the solved state and returns the shuffled state
  if(!seed)
	srand((seed=time(NULL)));
  int c;
  state_t out;
  state_t hold;
  int r;
  int last;
  (transformations[(r=rand()%18)])(&solved,&out);
  if(verbose)
	printf("%s\n",descriptions[r]);
  for(c=0;c<in-1;++c){
	last=r;
	r=rand()%15;
	if(last/3<=r/3)
		r+=3;
	(transformations[r])(&out,&hold);
	if(verbose)
		printf("%s\n",descriptions[r]);
	out=hold;
  }
  return out;
}

//Prints all values of the stateList from smallest to largest
void printStateList(stateList_t *list){
  stateList_t *pt;
  for(pt=list;pt;pt=pt->next)
	printState(pt->state);
}

//Retursn a pointer to the place in the list where a pointer to the state should be stored, returns NULL if state is a duplicate.
state_t** addList(stateList_t *stateList,state_t *state){
/*The state_t* that is passed to this function is a pointer to a temporary and contantly-changing stack variable in the calling function,
  so saving that pointer in the list would be #verybad. A permament home for the state_t is also not made until this function confirms it is not a duplicate.
  To remedy this problem this function behaves very oddly, in my opinion. If the state is found to be a duplicate, the semaphore is released, and NULL is returned.
  If it is found to be a new state, memory is malloced, and placed into the list at the correct place, a pointer to that new uninitialized memory is returned, and it
  is up to the calling function to save a pointer to the permament home of the new state_t at the address returned, and also release the mutex semaphore after the fact.
  Is it messy? Yes. Does it work? Without problem.
  */
  stateList_t *pt;
  int comp;
  state_t **out=NULL;
  sem_wait(stateList->turn);
  for(pt=stateList;(comp=compareStates(pt->state,state))==-1&&pt->next;pt=pt->next)
	;
  if(comp==0){
	sem_post(stateList->turn);
	return out;
  }
  else if(comp==-1){
	pt->next=malloc(sizeof(stateList_t));
	pt=pt->next;
	out=&pt->state;
	pt->next=NULL;
  }
  else{
	out=&pt->state;
	stateList_t *n=malloc(sizeof(stateList_t));
	n->state=pt->state;
	n->next=pt->next;
	pt->next=n;
  }
  return out;
}

//Frees all memory associated with the stateList
void freeStateList(stateList_t *list){
  stateList_t *pt,*lead;
  for(pt=list;pt;pt=lead){
	lead=pt->next;
	free(pt);
  }
}

//Thread-safe function to add a pointer to a new stateTreeNode_t to the BFS queue
void treeQueueAdd(treeQueue_t *treeQueue,stateTreeNode_t *n){
  treeQueueNode_t *node=malloc(sizeof(treeQueueNode_t));
  node->node=n;
  node->next=NULL;
  sem_wait(&treeQueue->turn);
  if(treeQueue->tail){
	treeQueue->tail->next=node;
	treeQueue->tail=node;
  }
  else
	treeQueue->head=treeQueue->tail=node;
  sem_post(&treeQueue->turn);
}

//Thread-safe function to remove a pointer to the oldest stateTreeNode_t from the BFS queue. Returns NULL if the queue is empty
stateTreeNode_t* treeQueueRemove(treeQueue_t *treeQueue){
  sem_wait(&treeQueue->turn);
  if(!treeQueue->head){
	sem_post(&treeQueue->turn);
	return NULL;
  }
  stateTreeNode_t *out=treeQueue->head->node;
  treeQueueNode_t *hold=treeQueue->head;
  treeQueue->head=hold->next;
  free(hold);
  if(!treeQueue->head)
	treeQueue->tail=NULL;
  sem_post(&treeQueue->turn);
  return out;
}

//This global variable is a signal to all threads to stop making new nodes.
volatile int solutionFound=0;

//This is the struct to hold the data a thread needs to perform the buildTree function properly.
typedef struct buildTreeData{
  stateList_t *stateList;
  treeQueue_t *treeQueue;
  sem_t *sem;
}buildTreeData_t;

//This allows us to keep track of which tier of the tree we're currently processing
volatile unsigned char currentTier=0;

//This function compares two stateLists for common elements every time it is awoken by some sem_t. It blocks on some thread until solutionFound becomes 1
state_t listMatch(stateList_t *a,stateList_t *b,sem_t *sem){
  stateList_t *apt,*bpt;
  int comp;
  while(!solutionFound){
	sem_wait(sem);
	sem_wait(a->turn);
	sem_wait(b->turn);
	apt=a;
	bpt=b;
	while(apt&&bpt){
		comp=compareStates(apt->state,bpt->state);
		if(!comp){
			solutionFound=1;
			printf("Solution found. Please wait while instructions are generated\n");
			sem_post(a->turn);
			sem_post(b->turn);
			return *(apt->state);
		}
		else if(comp==-1)
			apt=apt->next;
		else
			bpt=bpt->next;
	}
	sem_post(a->turn);
	sem_post(b->turn);
  }
  return solved;
}

//This is a function meant to be executed by a thread. It processes nodes from the BFS queue, and uses them to produce up to 15 more nodes to add to the queue
void* buildTree(void *data){
  stateList_t *stateList=((buildTreeData_t*)data)->stateList;
  treeQueue_t *treeQueue=((buildTreeData_t*)data)->treeQueue;
  sem_t *sem=((buildTreeData_t*)data)->sem;
  stateTreeNode_t *node=treeQueueRemove(treeQueue);
  state_t hold;
  state_t **listSlip;
  int c;
  char currentSide;
  while(!solutionFound){
	currentSide=node->side*3;
 	if(node->tier>currentTier){
		currentTier=node->tier;
		sem_post(sem);
		printf("Beginning tier %d\n",(int)currentTier);
	}
 	for(c=0;c<18;++c){
		if(c==currentSide){
			memcpy(&node->children[c],eightteenNulls,3*sizeof(void*));
			c+=2;
			continue;
		}
		(transformations[c])(&node->state,&hold);
		if((listSlip=addList(stateList,&hold))){
			node->children[c]=malloc(sizeof(stateTreeNode_t));
			node->children[c]->state=hold;
			*listSlip=&node->children[c]->state;
			sem_post(stateList->turn);
			node->children[c]->tier=node->tier+1;
			node->children[c]->side=c/3;
			treeQueueAdd(treeQueue,node->children[c]);
		}
		else
			node->children[c]=NULL;
	}
	if(!(node=treeQueueRemove(treeQueue)))
		solutionFound=1;
  }
//The pointers in all the nodes in the queue were left uninitialized, so they must be made NULL before search is done on the tree.
  while((node=treeQueueRemove(treeQueue)))
	memcpy(node->children,eightteenNulls,18*sizeof(void*));
  return NULL;
}

//This function is meant to be run by the parent thread, to insure that all of the threads running buildTree will have a first node to process
//It returns the number of nodes added to the queue
int buildOne(stateTreeNode_t *node,stateList_t *stateList,treeQueue_t *treeQueue){
  state_t hold;
  state_t **listSlip;
  int c,out=0;
  for(c=0;c<18;++c){
	(transformations[c])(&node->state,&hold);
	if((listSlip=addList(stateList,&hold))){
		++out;
		node->children[c]=malloc(sizeof(stateTreeNode_t));
		node->children[c]->state=hold;
		*listSlip=&node->children[c]->state;
		sem_post(stateList->turn);
		node->children[c]->tier=node->tier+1;
		node->children[c]->side=c/3;
		treeQueueAdd(treeQueue,node->children[c]);
	}
	else
		node->children[c]=NULL;
  }
  return out;
}

//This searches the mixed tree and returns a string of instructions to get to the target state of the tree. Return string must be freed
char* searchTree(stateTreeNode_t *tree,state_t *target){
  char *out=NULL;
  if(!compareStates(&tree->state,target)){
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
			out[strlen(out)-1]='\0';
			return out;
		}
  return NULL;
}

//This searches the solved tree and prints instructions on how to get from the target state to solved
int backwardsSearchTree(stateTreeNode_t *tree,state_t *target){
  if(!compareStates(&tree->state,target))
	return 1;
  int c;
  int hold;
  for(c=0;c<18;++c)
	if(tree->children[c])
		if(backwardsSearchTree(tree->children[c],target)){
			hold=c%3;
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

//Frees all maloced nodes of the tree
void recursiveFreeTree(stateTreeNode_t *tree){
  if(!tree)
	return;
  int c;
  for(c=0;c<18;++c)
  	recursiveFreeTree(tree->children[c]);
  free(tree);
}

void freeTree(stateTreeNode_t *tree){
  int c;
  for(c=0;c<18;++c)
  	recursiveFreeTree(tree->children[c]);
}


/*Two stateTrees, two stateLists, and two treeQueues are made. One for the mixed cube's state, and one for the solution state.
 *Half of the threads work on solving the mixed cube, and the other work on shuffling the solved cube, until they encounter a common state
 *When that common state is found, all threads terminate and the trees are compared to create a list of results.
 */
int main(){
  state_t shuffled=(state_t){{b,o,b,y,y,y,b,g,w,g,g,r,o,w,b,r,r,r,y,o,w,o,w,b,r,g,b,g,y,w,o,w,o,w,o,b,w,y,r,r,o,r,y,b,g,g,y,g}};
  stateTreeNode_t fromMixed=(stateTreeNode_t){.state=shuffled,.tier=0,.side=7};
  stateTreeNode_t fromSolved=(stateTreeNode_t){.state=solved,.tier=0,.side=7};
  stateList_t *mixedList=malloc(sizeof(stateList_t)+sizeof(sem_t));
  stateList_t *solvedList=malloc(sizeof(stateList_t)+sizeof(sem_t));
  *mixedList=(stateList_t){.state=&fromMixed.state,.next=NULL};
  *solvedList=(stateList_t){.state=&fromSolved.state,.next=NULL};
  sem_init(mixedList->turn,0,1);
  sem_init(solvedList->turn,0,1);
  treeQueue_t mixedQueue=(treeQueue_t){.head=NULL,.tail=NULL};
  treeQueue_t solvedQueue=(treeQueue_t){.head=NULL,.tail=NULL};
  sem_init(&mixedQueue.turn,0,1);
  sem_init(&solvedQueue.turn,0,1);
  sem_t matchSem;
  sem_init(&matchSem,0,0);
  buildTreeData_t mixedTreeData=(buildTreeData_t){.stateList=mixedList,.treeQueue=&mixedQueue,.sem=&matchSem};
  buildTreeData_t solvedTreeData=(buildTreeData_t){.stateList=solvedList,.treeQueue=&solvedQueue,.sem=&matchSem};
  pthread_t pids[NUM_THREADS-1];
  int c=0,s,m;
  m=buildOne(&fromMixed,mixedList,&mixedQueue);
  s=buildOne(&fromSolved,solvedList,&solvedQueue);
  while(c<NUM_THREADS-1){
	if(m<1)
		m=buildOne(treeQueueRemove(&mixedQueue),mixedList,&mixedQueue)-1;
	pthread_create(&pids[c++],NULL,buildTree,&mixedTreeData);
	--m;
	if(s<1)
		s=buildOne(treeQueueRemove(&solvedQueue),solvedList,&solvedQueue)-1;
	pthread_create(&pids[c++],NULL,buildTree,&solvedTreeData);
	--s;
  }
  state_t link=listMatch(mixedList,solvedList,&matchSem);
  for(--c;c>-1;--c)
  	pthread_join(pids[c],NULL);
  char *string;
  if(!(string=searchTree(&fromMixed,&link)))
	printf("No solution found\n");
  else{
	printf("%s\n",string);
	free(string);
	backwardsSearchTree(&fromSolved,&link);
  }
  sem_destroy(mixedList->turn);
  sem_destroy(solvedList->turn);
  freeStateList(mixedList);
  freeStateList(solvedList);
  sem_destroy(&mixedQueue.turn);
  sem_destroy(&solvedQueue.turn);
  freeTree(&fromMixed);
  freeTree(&fromSolved);
}
