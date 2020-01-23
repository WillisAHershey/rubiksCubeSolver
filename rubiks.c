#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>

#define NUM_THREADS 800

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

typedef struct volatileData{
  volatile state_t *state;
  volatile stateTreeNode_t **put;
  void (*transformation)(state_t*,state_t*);
  sem_t done,restart;
}volatileData_t;

typedef struct threadData{
  stateList_t *stateList;
  treeQueue_t *treeQueue;
  volatile volatileData_t *volatiles;
  sem_t done;
}threadData_t;

volatile int solutionFound=0;

void* threadBuild(void *data){
  volatileData_t volatiles;
  sem_init(&volatiles.done,0,0);
  sem_init(&volatiles.restart,0,0);
  ((threadData_t*)data)->volatiles=&volatiles;
  sem_post(&((threadData_t*)data)->done);
  stateList_t *stateList=((threadData_t*)data)->stateList;
  treeQueue_t *treeQueue=((threadData_t*)data)->treeQueue;
  state_t transformed;
  state_t *vstate;
  stateTreeNode_t **vput;
  state_t **listSlip;
  sem_wait(&volatiles.restart);
  int c;
  while(!solutionFound){
	vstate=(state_t*)volatiles.state;
	vput=(stateTreeNode_t**)volatiles.put;
  	(volatiles.transformation)(vstate,&transformed);
	if((listSlip=addList(stateList,&transformed))){
		(*vput)=malloc(sizeof(stateTreeNode_t));
		(*vput)->state=transformed;
		*listSlip=&(*vput)->state;
		sem_post(stateList->turn);
		if(compareStates(&solved,&transformed)){
			treeQueueAdd(treeQueue,*vput);
		}
		else{
			solutionFound=1;
			for(c=0;c<18;++c)
				(*vput)->children[c]=NULL;
		}
	}
	else
		*vput=NULL;
	sem_post(&volatiles.done);
	sem_wait(&volatiles.restart);
  }
  sem_wait(&((threadData_t*)data)->done);
  sem_destroy(&volatiles.done);
  sem_destroy(&volatiles.restart);
  return NULL;
}

typedef struct buildTreeData{
  stateTreeNode_t *node;
  stateList_t *stateList;
  treeQueue_t *treeQueue;
}buildTreeData_t;

void* buildTree(void *data){
  stateTreeNode_t *node=((buildTreeData_t*)data)->node;
  stateList_t *stateList=((buildTreeData_t*)data)->stateList;
  treeQueue_t *treeQueue=((buildTreeData_t*)data)->treeQueue;
  pthread_t pids[18];
  volatileData_t *volatiles[18];
  threadData_t threadData=(threadData_t){.stateList=stateList,.treeQueue=treeQueue,.volatiles=NULL,};
  sem_init(&threadData.done,0,0);
  int c;
  for(c=0;c<18;++c){
	pthread_create(&pids[c],NULL,threadBuild,(void*)&threadData);
	sem_wait(&threadData.done);
	volatiles[c]=(volatileData_t*)(volatileData_t*)(volatileData_t*)(volatileData_t*)(volatileData_t*)(volatileData_t*)(volatileData_t*)(volatileData_t*)(volatileData_t*)threadData.volatiles;
	volatiles[c]->state=&node->state;
	volatiles[c]->put=(volatile stateTreeNode_t**)&node->children[c];
	volatiles[c]->transformation=transformations[c];
	sem_post(&volatiles[c]->restart);
  }
  while(!solutionFound&&(node=treeQueueRemove(treeQueue))){
	for(c=0;c<18;++c){
		sem_wait(&volatiles[c]->done);
		volatiles[c]->state=&node->state;
		volatiles[c]->put=(volatile stateTreeNode_t**)&node->children[c];
		sem_post(&volatiles[c]->restart);
	}
  }
  if(!solutionFound){
	printf("Solution not found\n");
	solutionFound=1;
  }
  else{
	printf("SOLUTION FOUND\n");
  	while((node=treeQueueRemove(treeQueue)))
		for(c=0;c<18;++c)
			node->children[c]=NULL;
  }
  for(c=0;c<18;++c){
	sem_post(&volatiles[c]->restart);
	sem_post(&threadData.done);
  }
  for(c=0;c<18;++c)
	pthread_join(pids[c],NULL);
  sem_destroy(&threadData.done);
  return NULL;
}

int searchTree(stateTreeNode_t *tree){
  if(!compareStates(&tree->state,&solved)){
	return 1;
  }
  int c;
  for(c=0;c<18;++c){
	if(tree->children[c])
		if(searchTree(tree->children[c])){
			printf("%s\n",descriptions[c]);
			return 1;
		}
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
  stateTreeNode_t tree; //Root of the tree lives of the stack, but the rest of it is on the heap
  tree.state=shuffle(5,1);
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
  if(!solutionFound){
  	for(c=1;c<NUM_THREADS;++c){
		buildTreeData.node=treeQueueRemove(&treeQueue);
		pthread_create(&pids[c],NULL,buildTree,&buildTreeData);
	}
  }
  pthread_join(pids[0],NULL);
  freeStateList(stateList);
  sem_destroy(stateList->turn);
  sem_destroy(&treeQueue.turn);
  searchTree(&tree);
  freeTree(&tree);
  for(c=1;c<NUM_THREADS;++c)
	pthread_join(pids[c],NULL);
}
