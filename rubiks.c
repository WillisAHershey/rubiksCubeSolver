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

typedef struct state{ //A state is 48 colors in a very specific order, which defines the placement of all of the colors on the cube
  enum color c[48];
}state_t;

int compareStates(state_t *a,state_t *b){ //returns -1 if less, 1 if greater, and 0 if equal. This allows us to keep our state list in order.
  int i;
  for(i=0;i<48;++i)
	if(a->c[i]!=b->c[i])
		return a->c[i]<b->c[i]?-1:1;
  return 0;
}

void printState(state_t *in){ //This allows us to print any given state for debugging
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

typedef struct stateTreeNode{ //Contains a state_t, a pointer to all 18 possible children, a tier, and the side it was computed on.
  state_t state;
  struct stateTreeNode *children[18];
  unsigned char tier;
  char side;
}stateTreeNode_t;

typedef struct treeQueueNode{ //Node for BFS search queue
  stateTreeNode_t *node;
  struct treeQueueNode *next;
}treeQueueNode_t;

typedef struct treeQueue{ //Queue so we can process the nodes in order for BFS search
  treeQueueNode_t *head;
  treeQueueNode_t *tail;
  sem_t turn;
}treeQueue_t;

typedef struct stateList{ //List of states we have visited before, so we can ignore repetitive cases.
  state_t *state;
  struct stateList *next;
  sem_t turn[0];
}stateList_t;

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

void printStateList(stateList_t *list){ //Prints all values of the stateList from smallest to largest
  stateList_t *pt;
  for(pt=list;pt;pt=pt->next)
	printState(pt->state);
}

state_t** addList(stateList_t *stateList,state_t *state){ //Returns pointer to place in the list where the pointer to the state_t should be stored
  stateList_t *pt; //The state_t pointer that we're passed is to a temporary variable in another function, so it is up to the calling function to save a permanent
  int comp;        //Pointer to state_t at the address returned, and then release the semaphore to the list.
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

void freeStateList(stateList_t *list){
  stateList_t *pt,*lead;
  for(pt=list;pt;pt=lead){
	lead=pt->next;
	free(pt);
  }
}

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

volatile int solutionFound=0;

typedef struct buildTreeData{
  stateList_t *stateList;
  treeQueue_t *treeQueue;
}buildTreeData_t;

volatile unsigned char currentTier=0;

state_t listMatch(stateList_t *a,stateList_t *b){
  stateList_t *apt,*bpt;
  int comp;
  while(!solutionFound){
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

void* buildTree(void *data){
  stateList_t *stateList=((buildTreeData_t*)data)->stateList;
  treeQueue_t *treeQueue=((buildTreeData_t*)data)->treeQueue;
  stateTreeNode_t *node=treeQueueRemove(treeQueue);
  state_t hold;
  state_t **listSlip;
  int c;
  char currentSide;
  while(!solutionFound){
	currentSide=node->side*3;
 	if(node->tier>currentTier){
		currentTier=node->tier;
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
  while((node=treeQueueRemove(treeQueue)))
	memcpy(node->children,eightteenNulls,18*sizeof(void*));
  return NULL;
}

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
  buildTreeData_t mixedTreeData=(buildTreeData_t){.stateList=mixedList,.treeQueue=&mixedQueue};
  buildTreeData_t solvedTreeData=(buildTreeData_t){.stateList=solvedList,.treeQueue=&solvedQueue};
  pthread_t pids[NUM_THREADS-1];
  int c=0,s,m;
  m=buildOne(&fromMixed,mixedList,&mixedQueue);
  s=buildOne(&fromSolved,solvedList,&solvedQueue);
  while(c<NUM_THREADS-1){
	if(!m){
		m=buildOne(treeQueueRemove(&mixedQueue),mixedList,&mixedQueue);
	}
	pthread_create(&pids[c++],NULL,buildTree,&mixedTreeData);
	--m;
	if(!s){
		s=buildOne(treeQueueRemove(&solvedQueue),solvedList,&solvedQueue);
	}
	pthread_create(&pids[c++],NULL,buildTree,&solvedTreeData);
	--s;
  }
  state_t link=listMatch(mixedList,solvedList);
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
