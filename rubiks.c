#include <stdio.h>
#include <stdlib.h>

enum color {white=0,blue=1,green=2,yellow=3,red=4,orange=5,w=0,b=1,g=2,y=3,r=4,o=5};
//This allows us to convert colors directly to integer types, and it also saves memory, since we only need 3 bits

typedef struct state{ //A state is 48 colors in a very specific order, which defines the placement of all of the colors on the cube
  enum color c[48];
}state_t;

void printState(state_t in){ //This allows us to print any given state for debugging
  int c;
  printf("{");
  for(c=0;c<48;++c){
	switch(in.c[c]){
	case white:
		printf("w,");
		break;
	case blue:
		printf("b,");
		break;
	case green:
		printf("g,");
		break;
	case yellow:
		printf("y,");
		break;
	case red:
		printf("r,");
		break;
	case orange:
		printf("o,");
		break;
	}
  }
  printf("}\n");
}
//This is the solved state, the state we attempt to achieve
state_t solved=(state_t){{white,white,white,white,white,white,white,white,blue,blue,blue,blue,blue,blue,blue,blue,green,green,green,green,green,green,green,green,yellow,yellow,yellow,yellow,yellow,yellow,yellow,yellow,red,red,red,red,red,red,red,red,orange,orange,orange,orange,orange,orange,orange,orange}}; 

typedef struct stateTreeNode{ //Has a pointer for all possible movements from this point and a state
  struct stateTreeNode *faceClock,*faceCounter,*faceTwice,*leftClock,*leftCounter,*leftTwice,*rightClock,*rightCounter,*rightTwice,*rearClock,*rearCounter,*rearTwice,*topClock,*topCounter,*topTwice,*bottomClock,*bottomCounter,*bottomTwice;
  state_t state;
}stateTreeNode_t;

typedef struct treeQueueNode{ //Nodes for treeQueue
  stateTreeNode_t *node;
  struct treeQueueNode *next;
}treeQueueNode_t;

typedef struct treeQueue{ //Queue so we can process the nodes in order for BFS search
  treeQueueNode_t *head;
  treeQueueNode_t *tail;
}treeQueue_t;

typedef struct stateList{ //List of states we have visited before, so none get repeated.
  state_t state;
  struct stateList *next;
}stateList_t;

//The following functions mutate the input state to simulate some specific turn of the cube and return the resulting state.

state_t faceClock(state_t in){
  return (state_t){{in.c[5],in.c[3],in.c[0],in.c[6],in.c[1],in.c[7],in.c[4],in.c[2],in.c[40],in.c[41],in.c[42],in.c[11],in.c[12],in.c[13],in.c[14],in.c[15],in.c[32],in.c[33],in.c[34],in.c[19],in.c[20],in.c[21],in.c[22],in.c[23],in.c[24],in.c[25],in.c[26],in.c[27],in.c[28],in.c[29],in.c[30],in.c[31],in.c[8],in.c[9],in.c[10],in.c[35],in.c[36],in.c[37],in.c[38],in.c[39],in.c[16],in.c[17],in.c[18],in.c[43],in.c[44],in.c[45],in.c[46],in.c[47]}};
}

state_t faceCounter(state_t in){
  return (state_t){{in.c[2],in.c[4],in.c[7],in.c[1],in.c[6],in.c[0],in.c[3],in.c[5],in.c[32],in.c[33],in.c[34],in.c[11],in.c[12],in.c[13],in.c[14],in.c[15],in.c[40],in.c[41],in.c[42],in.c[19],in.c[20],in.c[21],in.c[22],in.c[23],in.c[24],in.c[25],in.c[26],in.c[27],in.c[28],in.c[29],in.c[30],in.c[31],in.c[16],in.c[17],in.c[18],in.c[35],in.c[36],in.c[37],in.c[38],in.c[39],in.c[8],in.c[9],in.c[10],in.c[43],in.c[44],in.c[45],in.c[46],in.c[47]}};
}

state_t faceTwice(state_t in){
  return (state_t){{in.c[7],in.c[6],in.c[5],in.c[4],in.c[3],in.c[2],in.c[1],in.c[0],in.c[16],in.c[17],in.c[18],in.c[11],in.c[12],in.c[13],in.c[14],in.c[15],in.c[8],in.c[9],in.c[10],in.c[19],in.c[20],in.c[21],in.c[22],in.c[23],in.c[24],in.c[25],in.c[26],in.c[27],in.c[28],in.c[29],in.c[30],in.c[31],in.c[40],in.c[41],in.c[42],in.c[35],in.c[36],in.c[37],in.c[38],in.c[39],in.c[32],in.c[33],in.c[34],in.c[43],in.c[44],in.c[45],in.c[46],in.c[47]}};
}

state_t leftClock(state_t in){
  return (state_t){{in.c[39],in.c[1],in.c[2],in.c[36],in.c[4],in.c[34],in.c[6],in.c[7],in.c[13],in.c[11],in.c[8],in.c[14],in.c[9],in.c[15],in.c[12],in.c[10],in.c[16],in.c[17],in.c[18],in.c[19],in.c[20],in.c[21],in.c[22],in.c[23],in.c[24],in.c[25],in.c[45],in.c[27],in.c[43],in.c[29],in.c[30],in.c[40],in.c[32],in.c[33],in.c[26],in.c[35],in.c[28],in.c[37],in.c[38],in.c[31],in.c[0],in.c[41],in.c[42],in.c[3],in.c[44],in.c[5],in.c[46],in.c[47]}};
}

state_t leftCounter(state_t in){
  return (state_t){{in.c[40],in.c[1],in.c[2],in.c[43],in.c[4],in.c[45],in.c[6],in.c[7],in.c[10],in.c[12],in.c[15],in.c[9],in.c[14],in.c[8],in.c[11],in.c[13],in.c[16],in.c[17],in.c[18],in.c[19],in.c[20],in.c[21],in.c[22],in.c[23],in.c[24],in.c[25],in.c[34],in.c[27],in.c[36],in.c[29],in.c[30],in.c[39],in.c[32],in.c[33],in.c[5],in.c[35],in.c[3],in.c[37],in.c[38],in.c[0],in.c[31],in.c[41],in.c[42],in.c[28],in.c[44],in.c[26],in.c[46],in.c[47]}};
}

state_t leftTwice(state_t in){
  return (state_t){{in.c[31],in.c[1],in.c[2],in.c[28],in.c[4],in.c[26],in.c[6],in.c[7],in.c[15],in.c[14],in.c[13],in.c[12],in.c[11],in.c[10],in.c[9],in.c[8],in.c[16],in.c[17],in.c[18],in.c[19],in.c[20],in.c[21],in.c[22],in.c[23],in.c[24],in.c[25],in.c[5],in.c[27],in.c[3],in.c[29],in.c[30],in.c[0],in.c[32],in.c[33],in.c[45],in.c[35],in.c[43],in.c[37],in.c[38],in.c[40],in.c[39],in.c[41],in.c[42],in.c[36],in.c[44],in.c[34],in.c[46],in.c[47]}};
}

state_t rightClock(state_t in){
  return (state_t){{in.c[0],in.c[1],in.c[42],in.c[3],in.c[44],in.c[5],in.c[6],in.c[47],in.c[8],in.c[9],in.c[10],in.c[11],in.c[12],in.c[13],in.c[14],in.c[15],in.c[21],in.c[19],in.c[16],in.c[22],in.c[17],in.c[23],in.c[20],in.c[18],in.c[32],in.c[25],in.c[26],in.c[35],in.c[28],in.c[37],in.c[30],in.c[31],in.c[7],in.c[33],in.c[34],in.c[4],in.c[36],in.c[2],in.c[38],in.c[39],in.c[40],in.c[41],in.c[29],in.c[43],in.c[27],in.c[45],in.c[46],in.c[24]}};
}

state_t rightCounter(state_t in){
  return (state_t){{in.c[0],in.c[1],in.c[37],in.c[3],in.c[35],in.c[5],in.c[6],in.c[32],in.c[8],in.c[9],in.c[10],in.c[11],in.c[12],in.c[13],in.c[14],in.c[15],in.c[18],in.c[20],in.c[23],in.c[17],in.c[22],in.c[16],in.c[19],in.c[21],in.c[47],in.c[25],in.c[26],in.c[44],in.c[28],in.c[42],in.c[30],in.c[31],in.c[24],in.c[33],in.c[34],in.c[27],in.c[36],in.c[24],in.c[38],in.c[39],in.c[40],in.c[41],in.c[2],in.c[43],in.c[4],in.c[45],in.c[46],in.c[7]}};
}

state_t rightTwice(state_t in){
  return (state_t){{in.c[0],in.c[1],in.c[29],in.c[3],in.c[27],in.c[5],in.c[6],in.c[24],in.c[8],in.c[9],in.c[10],in.c[11],in.c[12],in.c[13],in.c[14],in.c[15],in.c[23],in.c[22],in.c[21],in.c[20],in.c[19],in.c[18],in.c[17],in.c[16],in.c[7],in.c[25],in.c[26],in.c[4],in.c[28],in.c[2],in.c[30],in.c[31],in.c[47],in.c[33],in.c[34],in.c[44],in.c[36],in.c[42],in.c[38],in.c[39],in.c[40],in.c[41],in.c[37],in.c[43],in.c[35],in.c[45],in.c[46],in.c[32]}};
}

state_t rearClock(state_t in){
  return (state_t){{in.c[0],in.c[1],in.c[2],in.c[3],in.c[4],in.c[5],in.c[6],in.c[7],in.c[8],in.c[9],in.c[10],in.c[11],in.c[12],in.c[37],in.c[38],in.c[39],in.c[16],in.c[17],in.c[18],in.c[19],in.c[20],in.c[45],in.c[46],in.c[47],in.c[29],in.c[27],in.c[24],in.c[30],in.c[25],in.c[31],in.c[28],in.c[26],in.c[32],in.c[33],in.c[34],in.c[35],in.c[36],in.c[21],in.c[22],in.c[23],in.c[40],in.c[41],in.c[42],in.c[43],in.c[44],in.c[13],in.c[14],in.c[15]}};
}

state_t rearCounter(state_t in){
  return (state_t){{in.c[0],in.c[1],in.c[2],in.c[3],in.c[4],in.c[5],in.c[6],in.c[7],in.c[8],in.c[9],in.c[10],in.c[11],in.c[12],in.c[45],in.c[46],in.c[47],in.c[16],in.c[17],in.c[18],in.c[19],in.c[20],in.c[37],in.c[38],in.c[39],in.c[26],in.c[28],in.c[31],in.c[25],in.c[30],in.c[24],in.c[27],in.c[29],in.c[32],in.c[33],in.c[34],in.c[35],in.c[36],in.c[13],in.c[14],in.c[15],in.c[40],in.c[41],in.c[42],in.c[43],in.c[44],in.c[21],in.c[22],in.c[23]}};
}

state_t rearTwice(state_t in){
  return (state_t){{in.c[0],in.c[1],in.c[2],in.c[3],in.c[4],in.c[5],in.c[6],in.c[7],in.c[8],in.c[9],in.c[10],in.c[11],in.c[12],in.c[21],in.c[22],in.c[23],in.c[16],in.c[17],in.c[18],in.c[19],in.c[20],in.c[13],in.c[14],in.c[15],in.c[31],in.c[30],in.c[29],in.c[28],in.c[27],in.c[26],in.c[25],in.c[24],in.c[32],in.c[33],in.c[34],in.c[35],in.c[36],in.c[45],in.c[46],in.c[47],in.c[40],in.c[41],in.c[42],in.c[43],in.c[44],in.c[37],in.c[38],in.c[39]}};
}

state_t topClock(state_t in){
  return (state_t){{in.c[18],in.c[20],in.c[23],in.c[3],in.c[4],in.c[5],in.c[6],in.c[7],in.c[2],in.c[9],in.c[10],in.c[1],in.c[12],in.c[0],in.c[14],in.c[15],in.c[16],in.c[17],in.c[24],in.c[19],in.c[25],in.c[21],in.c[22],in.c[26],in.c[13],in.c[11],in.c[8],in.c[27],in.c[28],in.c[29],in.c[30],in.c[31],in.c[37],in.c[35],in.c[32],in.c[38],in.c[33],in.c[39],in.c[36],in.c[34],in.c[40],in.c[41],in.c[42],in.c[43],in.c[44],in.c[45],in.c[46],in.c[47]}};
}

state_t topCounter(state_t in){
  return (state_t){{in.c[13],in.c[11],in.c[8],in.c[3],in.c[4],in.c[5],in.c[6],in.c[7],in.c[26],in.c[9],in.c[10],in.c[25],in.c[12],in.c[24],in.c[14],in.c[15],in.c[16],in.c[17],in.c[0],in.c[19],in.c[1],in.c[21],in.c[22],in.c[2],in.c[18],in.c[20],in.c[23],in.c[27],in.c[28],in.c[29],in.c[30],in.c[31],in.c[34],in.c[36],in.c[39],in.c[33],in.c[38],in.c[32],in.c[35],in.c[37],in.c[40],in.c[41],in.c[42],in.c[43],in.c[44],in.c[45],in.c[46],in.c[47]}};
}

state_t topTwice(state_t in){
  return (state_t){{in.c[24],in.c[25],in.c[26],in.c[3],in.c[4],in.c[5],in.c[6],in.c[7],in.c[23],in.c[9],in.c[10],in.c[20],in.c[12],in.c[18],in.c[14],in.c[15],in.c[16],in.c[17],in.c[13],in.c[19],in.c[11],in.c[21],in.c[22],in.c[8],in.c[0],in.c[1],in.c[2],in.c[27],in.c[28],in.c[29],in.c[30],in.c[31],in.c[39],in.c[38],in.c[37],in.c[36],in.c[35],in.c[34],in.c[33],in.c[32],in.c[40],in.c[41],in.c[42],in.c[43],in.c[44],in.c[45],in.c[46],in.c[47]}};
}

state_t bottomClock(state_t in){
  return (state_t){{in.c[0],in.c[1],in.c[2],in.c[3],in.c[4],in.c[15],in.c[12],in.c[10],in.c[8],in.c[9],in.c[31],in.c[11],in.c[30],in.c[13],in.c[14],in.c[29],in.c[5],in.c[17],in.c[18],in.c[6],in.c[20],in.c[7],in.c[22],in.c[23],in.c[24],in.c[25],in.c[26],in.c[27],in.c[28],in.c[16],in.c[19],in.c[21],in.c[32],in.c[33],in.c[34],in.c[35],in.c[36],in.c[37],in.c[38],in.c[39],in.c[45],in.c[43],in.c[40],in.c[46],in.c[41],in.c[47],in.c[44],in.c[42]}};
}

state_t bottomCounter(state_t in){
  return (state_t){{in.c[0],in.c[1],in.c[2],in.c[3],in.c[4],in.c[16],in.c[19],in.c[21],in.c[8],in.c[9],in.c[7],in.c[11],in.c[6],in.c[13],in.c[14],in.c[5],in.c[29],in.c[17],in.c[18],in.c[30],in.c[20],in.c[31],in.c[22],in.c[23],in.c[24],in.c[25],in.c[26],in.c[27],in.c[28],in.c[15],in.c[12],in.c[10],in.c[32],in.c[33],in.c[34],in.c[35],in.c[36],in.c[37],in.c[38],in.c[39],in.c[42],in.c[44],in.c[47],in.c[41],in.c[46],in.c[40],in.c[43],in.c[45]}};
}

state_t bottomTwice(state_t in){
  return (state_t){{in.c[0],in.c[1],in.c[2],in.c[3],in.c[4],in.c[29],in.c[30],in.c[31],in.c[8],in.c[9],in.c[21],in.c[11],in.c[19],in.c[13],in.c[14],in.c[16],in.c[15],in.c[17],in.c[18],in.c[12],in.c[20],in.c[10],in.c[22],in.c[23],in.c[24],in.c[25],in.c[26],in.c[27],in.c[28],in.c[5],in.c[6],in.c[7],in.c[32],in.c[33],in.c[34],in.c[35],in.c[36],in.c[37],in.c[38],in.c[39],in.c[47],in.c[46],in.c[45],in.c[44],in.c[43],in.c[42],in.c[41],in.c[40]}};
}

int compareStates(state_t a,state_t b){ //returns -1 if less, 1 if greater, and 0 if equal. This allows us to keep our state list in order.
  int i;
  for(i=0;i<48;++i)
	if(a.c[i]!=b.c[i])
		return a.c[i]<b.c[i]?-1:1;
  return 0;
}

void printStateList(stateList_t *list){
  stateList_t *pt;
  for(pt=list;pt;pt=pt->next)
	printState(pt->state);
}

int addList(stateList_t *stateList,state_t state){ //Assumes stateList already has one element
  stateList_t *pt;
  int comp;
  for(pt=stateList;(comp=compareStates(pt->state,state))==-1&&pt->next;pt=pt->next)
	;
  if(comp==0)
	  return 0;
  else if(comp==-1){
	pt->next=(stateList_t*)malloc(sizeof(stateList_t));
	pt=pt->next;
	pt->state=state;
	pt->next=NULL;
  }
  else{	
	state_t hold=pt->state;
	pt->state=state;
	stateList_t *n=(stateList_t*)malloc(sizeof(stateList_t));
	n->state=hold;
	n->next=pt->next;
	pt->next=n;
  }
  return 1;
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
  if(treeQueue->tail){
	treeQueue->tail->next=node;
	treeQueue->tail=node;
  }
  else
	treeQueue->head=treeQueue->tail=node;
}

stateTreeNode_t* treeQueueRemove(treeQueue_t *treeQueue){
  if(!treeQueue->head)
	return NULL;
  stateTreeNode_t *out=treeQueue->head->node;
  treeQueueNode_t *hold=treeQueue->head;
  treeQueue->head=hold->next;
  free(hold);
  if(!treeQueue->head)
	treeQueue->tail=NULL;
  return out;
}

void buildTree(stateTreeNode_t *node,stateList_t *stateList){
  state_t hold;
  treeQueue_t onStack=(treeQueue_t){NULL,NULL};
  treeQueue_t *treeQueue=&onStack;
  do{
	hold=faceClock(node->state);
	if(addList(stateList,hold)){
		node->faceClock=malloc(sizeof(stateTreeNode_t));
		node->faceClock->state=hold;
		treeQueueAdd(treeQueue,node->faceClock);
	}
	else
		node->faceClock=NULL;
	hold=faceCounter(node->state);
	if(addList(stateList,hold)){
		node->faceCounter=malloc(sizeof(stateTreeNode_t));
		node->faceCounter->state=hold;
		treeQueueAdd(treeQueue,node->faceCounter);
	}
	else
		node->faceCounter=NULL;
	hold=faceTwice(node->state);
	if(addList(stateList,hold)){
		node->faceTwice=malloc(sizeof(stateTreeNode_t));
		node->faceTwice->state=hold;
		treeQueueAdd(treeQueue,node->faceTwice);
	}
	else
		node->faceTwice=NULL;
	hold=leftClock(node->state);
	if(addList(stateList,hold)){
		node->leftClock=malloc(sizeof(stateTreeNode_t));
		node->leftClock->state=hold;
		treeQueueAdd(treeQueue,node->leftClock);
	}
	else
		node->leftClock=NULL;
	hold=leftCounter(node->state);
	if(addList(stateList,hold)){
		node->leftCounter=malloc(sizeof(stateTreeNode_t));
		node->leftCounter->state=hold;
		treeQueueAdd(treeQueue,node->leftCounter);
	}
	else
		node->leftCounter=NULL;
	hold=leftTwice(node->state);
	if(addList(stateList,hold)){
		node->leftTwice=malloc(sizeof(stateTreeNode_t));
		node->leftTwice->state=hold;
		treeQueueAdd(treeQueue,node->leftTwice);
	}
	else
		node->leftTwice=NULL;
	hold=rightClock(node->state);
	if(addList(stateList,hold)){
		node->rightClock=malloc(sizeof(stateTreeNode_t));
		node->rightClock->state=hold;
		treeQueueAdd(treeQueue,node->rightClock);
	}
	else
		node->rightClock=NULL;
	hold=rightCounter(node->state);
	if(addList(stateList,hold)){
		node->rightCounter=malloc(sizeof(stateTreeNode_t));
		node->rightCounter->state=hold;
		treeQueueAdd(treeQueue,node->rightCounter);
	}
	else
		node->rightCounter=NULL;
	hold=rightTwice(node->state);
	if(addList(stateList,hold)){
		node->rightTwice=malloc(sizeof(stateTreeNode_t));
		node->rightTwice->state=hold;
		treeQueueAdd(treeQueue,node->rightTwice);
	}
	else
		node->rightTwice=NULL;
	hold=rearClock(node->state);
	if(addList(stateList,hold)){
		node->rearClock=malloc(sizeof(stateTreeNode_t));
		node->rearClock->state=hold;
		treeQueueAdd(treeQueue,node->rearClock);
	}
	else
		node->rearClock=NULL;
	hold=rearCounter(node->state);
	if(addList(stateList,hold)){
		node->rearCounter=malloc(sizeof(stateTreeNode_t));
		node->rearCounter->state=hold;
		treeQueueAdd(treeQueue,node->rearCounter);
	}
	else
		node->rearCounter=NULL;
	hold=rearTwice(node->state);
	if(addList(stateList,hold)){
		node->rearTwice=malloc(sizeof(stateTreeNode_t));
		node->rearTwice->state=hold;
		treeQueueAdd(treeQueue,node->rearTwice);
	}
	else
		node->rearTwice=NULL;
	hold=topClock(node->state);
	if(addList(stateList,hold)){
		node->topClock=malloc(sizeof(stateTreeNode_t));
		node->topClock->state=hold;
		treeQueueAdd(treeQueue,node->topClock);
	}
	else
		node->topClock=NULL;
	hold=topCounter(node->state);
	if(addList(stateList,hold)){
		node->topCounter=malloc(sizeof(stateTreeNode_t));
		node->topCounter->state=hold;
		treeQueueAdd(treeQueue,node->topCounter);
	}
	else
		node->topCounter=NULL;
	hold=topTwice(node->state);
	if(addList(stateList,hold)){
		node->topTwice=malloc(sizeof(stateTreeNode_t));
		node->topTwice->state=hold;
		treeQueueAdd(treeQueue,node->topTwice);
	}
	else
		node->topTwice=NULL;
	hold=bottomClock(node->state);
	if(addList(stateList,hold)){
		node->bottomClock=malloc(sizeof(stateTreeNode_t));
		node->bottomClock->state=hold;
		treeQueueAdd(treeQueue,node->bottomClock);
	}
	else
		node->bottomClock=NULL;
	hold=bottomCounter(node->state);
	if(addList(stateList,hold)){
		node->bottomCounter=malloc(sizeof(stateTreeNode_t));
		node->bottomCounter->state=hold;
		treeQueueAdd(treeQueue,node->bottomCounter);
	}
	else
		node->bottomCounter=NULL;
	hold=bottomTwice(node->state);
	if(addList(stateList,hold)){
		node->bottomTwice=malloc(sizeof(stateTreeNode_t));
		node->bottomTwice->state=hold;
		treeQueueAdd(treeQueue,node->bottomTwice);
	}
	else
		node->bottomTwice=NULL;
	node=treeQueueRemove(treeQueue);
	if(!node){
		printf("Solution not found\n");
		printStateList(stateList);
		return;
	}
  }while(compareStates(solved,node->state));
  printf("Solution found!!!\n");
  while((node=treeQueueRemove(treeQueue)))
  node->faceClock=node->faceCounter=node->faceTwice=node->leftClock=node->leftCounter=node->leftTwice=node->rightClock=node->rightCounter=node->rightTwice=node->rearClock=node->rearCounter=node->rearTwice=node->topClock=node->topCounter=node->topTwice=node->bottomClock=node->bottomCounter=node->bottomTwice=NULL;
}

int searchTree(stateTreeNode_t *tree){
  if(!compareStates(tree->state,solved))
	return 1;	  
  if(tree->faceClock)
	if(searchTree(tree->faceClock)){
		printf("Face clockwise\n");
		return 1;
	}
  if(tree->faceCounter)
	if(searchTree(tree->faceCounter)){
		printf("Face counter-clockwise\n");
		return 1;
	}
  if(tree->faceTwice)
	if(searchTree(tree->faceTwice)){
		printf("Face twice\n");
		return 1;
	}
  if(tree->leftClock)
	if(searchTree(tree->leftClock)){
		printf("Left clockwise \n");
		return 1;
	}
  if(tree->leftCounter)
	if(searchTree(tree->leftCounter)){
		printf("Left counter-clockwise\n");
		return 1;
	}
  if(tree->leftTwice)
	if(searchTree(tree->leftTwice)){
		printf("Left twice\n");
		return 1;
	}
  if(tree->rightClock)
	if(searchTree(tree->rightClock)){
		printf("Right clockwise\n");
		return 1;
	}
  if(tree->rightCounter)
	if(searchTree(tree->rightCounter)){
		printf("Right counter-clockwise\n");
		return 1;
	}
  if(tree->rightTwice)
	if(searchTree(tree->rightTwice)){
		printf("Right twice\n");
		return 1;
	}
  if(tree->rearClock)
	if(searchTree(tree->rearClock)){
		printf("Rear clockwise\n");
		return 1;
	}
  if(tree->rearCounter)
	if(searchTree(tree->rearCounter)){
		printf("Rear counter-clockwise\n");
		return 1;
	}
  if(tree->rearTwice)
	if(searchTree(tree->rearTwice)){
		printf("Rear twice\n");
		return 1;
	}
  if(tree->topClock)
	if(searchTree(tree->topClock)){
		printf("Top clockwise\n");
		return 1;
	}
  if(tree->topCounter)
	if(searchTree(tree->topCounter)){
		printf("top counter-clockwise\n");
		return 1;
	}
  if(tree->topTwice)
	if(searchTree(tree->topTwice)){
		printf("Top twice\n");
		return 1;
	}
  if(tree->bottomClock)
	if(searchTree(tree->bottomClock)){
		printf("Bottom clockwise\n");
		return 1;
	}
  if(tree->bottomCounter)
	if(searchTree(tree->bottomCounter)){
		printf("Bottom counter-clockwise\n");
		return 1;
	}
  if(tree->bottomTwice)
	if(searchTree(tree->bottomTwice)){
		printf("Botttom twice\n");
		return 1;
	}
  return 0;
}

int main(){
  state_t start=(state_t){{o,w,r,y,g,g,b,b,y,b,w,w,o,g,g,y,w,y,g,w,o,w,o,w,g,r,y,w,r,b,r,b,y,b,b,g,r,r,b,o,o,o,o,y,g,r,y,r}};
  stateTreeNode_t tree;
  tree.state=start;
  stateList_t *stateList=(stateList_t*)malloc(sizeof(stateList_t));
  stateList->state=start;
  stateList->next=NULL;
  printf("Building tree\n");
  buildTree(&tree,stateList);
  freeStateList(stateList);
  searchTree(&tree);
}
