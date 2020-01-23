#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

#define NUM_THREADS 2000

enum color {white=0,blue=1,green=2,yellow=3,red=4,orange=5,w=0,b=1,g=2,y=3,r=4,o=5};
//This allows us to convert colors directly to integer types, and it also saves memory, since we only need 3 bits

typedef struct state{ //A state is 48 colors in a very specific order, which defines the placement of all of the colors on the cube
  enum color c[48];
}state_t;

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

typedef struct stateTreeNode{ //Has a pointer for all possible movements from this point and a state
  state_t state;
  struct stateTreeNode *children[18];
  unsigned char tier;
}stateTreeNode_t;

typedef struct treeQueueNode{ //Nodes for treeQueue
  stateTreeNode_t *node;
  struct treeQueueNode *next;
}treeQueueNode_t;

typedef struct treeQueue{ //Queue so we can process the nodes in order for BFS search
  treeQueueNode_t *head;
  treeQueueNode_t *tail;
  sem_t turn;
}treeQueue_t;

typedef struct stateList{ //List of states we have visited before, so none get repeated.
  state_t *state;
  struct stateList *next;
  sem_t turn[0];
}stateList_t;

//The following functions mutate the input state to simulate some specific turn of the cube and return the resulting state.
//Pointers are used to prevent the machine from having to copy so much

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

void (*transformations[])(state_t*,state_t*)={faceClock,faceCounter,faceTwice,leftClock,leftCounter,leftTwice,rightClock,rightCounter,rightTwice,rearClock,rearCounter,rearTwice,topClock,topCounter,topTwice,bottomClock,bottomCounter,bottomTwice};

char *descriptions[]={"Face clockwise","Face counter-clockwise","Face twice","Left clockwise","Left counter-clockwise","Left twice","Right clockwise","Right counter-clockwise","Right Twice","Rear clockwise","Rear counter-clockwise","Rear twice","Top clockwise","Top counter-clockwise","Top twice","Bottom clockwise","Bottom counter-clockwise","Bottom twice"};

int seed=0;

state_t shuffle(int in,int verbose){
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

int compareStates(state_t *a,state_t *b){ //returns -1 if less, 1 if greater, and 0 if equal. This allows us to keep our state list in order.
  int i;
  for(i=0;i<48;++i)
	if(a->c[i]!=b->c[i])
		return a->c[i]<b->c[i]?-1:1;
  return 0;
}

void printStateList(stateList_t *list){
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
  treeQueueNode_t *node=(treeQueueNode_t*)malloc(sizeof(treeQueueNode_t));
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
  stateTreeNode_t *node;
  stateList_t *stateList;
  treeQueue_t *treeQueue;
}buildTreeData_t;

volatile unsigned char currentTier=0;

void* buildTree(void *data){
  stateTreeNode_t *node=((buildTreeData_t*)data)->node;
  stateList_t *stateList=((buildTreeData_t*)data)->stateList;
  treeQueue_t *treeQueue=((buildTreeData_t*)data)->treeQueue;
  state_t hold;
  state_t **listSlip;
  int c;
  while(!solutionFound){
	for(c=0;c<18;++c){
		if(node->tier>currentTier){
			currentTier=node->tier;
			printf("Beginning tier %d\n",(int)node->tier);
		}
		(transformations[c])(&node->state,&hold);
		if((listSlip=addList(stateList,&hold))){
			node->children[c]=malloc(sizeof(stateTreeNode_t));
			node->children[c]->state=hold;
			*listSlip=&node->children[c]->state;
			sem_post(stateList->turn);
			node->children[c]->tier=node->tier+1;
			if(compareStates(&solved,&hold))
				treeQueueAdd(treeQueue,node->children[c]);
			else{
				solutionFound=1;
				printf("\n\nSolution found!! please wait as instructions are generated:\n\n");
				for(++c;c<18;++c)
					node->children[c]=NULL;
				break;
			}
		}
		else
			node->children[c]=NULL;
	}
	if(!(node=treeQueueRemove(treeQueue)))
		solutionFound=1;
  }
  while((node=treeQueueRemove(treeQueue)))
	for(c=0;c<18;++c)
		node->children[c]=NULL;
  return NULL;
}

char* searchTree(stateTreeNode_t *tree){
  char *out=NULL;
  if(!compareStates(&tree->state,&solved)){
	out=malloc(1);
	*out='\0';
	return out;
  }
  int c;
  char *hold;
  for(c=0;c<18;++c){
	if(tree->children[c])
		if((hold=searchTree(tree->children[c]))){
			out=malloc(strlen(hold)+strlen(descriptions[c])+2);
			sprintf(out,"%s\n%s",descriptions[c],hold);
			free(hold);
			return out;
		}
  }
  return NULL;
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
  printf("sizeof(void*)=%lu\n",sizeof(void*));
  printf("sizeof(enum color)=%lu\n",sizeof(enum color));
  printf("sizeof(state_t)=%lu\n",sizeof(state_t));
  printf("sizeof(stateTreeNode_t)=%lu\n",sizeof(stateTreeNode_t));
  printf("sizeof(treeQueue_t)=%lu\n",sizeof(treeQueue_t));
  printf("sizeof(treeQueueNode_t)=%lu\n",sizeof(treeQueueNode_t));
  printf("sizeof(stateList_t)=%lu\n",sizeof(stateList_t));
  stateTreeNode_t tree; //Root of the tree lives of the stack, but the rest of it is on the heap
  tree.state=shuffle(5,1);
  tree.tier=0;
  stateList_t *stateList=(stateList_t*)malloc(sizeof(stateList_t)+sizeof(sem_t)); //This is the only stateList_t that actually has a sem_t in it
  stateList->state=&tree.state; //stateList_t's don't actually have a copy of a state_t, just a pointer to a valid one
  stateList->next=NULL;
  sem_init(stateList->turn,0,1);
  treeQueue_t treeQueue;
  treeQueue.head=NULL;
  treeQueue.tail=NULL;
  sem_init(&treeQueue.turn,0,1);
  buildTreeData_t buildTreeData=(buildTreeData_t){.node=&tree,.stateList=stateList,.treeQueue=&treeQueue};
  pthread_t pids[NUM_THREADS];
  pthread_create(&pids[0],NULL,buildTree,&buildTreeData);
  sleep(5);
  int c;
  for(c=1;c<NUM_THREADS;++c){
	if(!(buildTreeData.node=treeQueueRemove(&treeQueue)))
		break;
	pthread_create(&pids[c],NULL,buildTree,&buildTreeData);
  }
  for(--c;c>-1;--c)
  	pthread_join(pids[c],NULL);
  sem_destroy(stateList->turn);
  freeStateList(stateList);
  char *string;
  if(!(string=searchTree(&tree)))
	printf("No solution found\n");
  else{
	printf("%s\n",string);
	free(string);
  }
  sem_destroy(&treeQueue.turn);
  freeTree(&tree);
}
