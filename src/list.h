typedef short boolean;
struct nodeList_c{

	struct nodeList*first;
	int size;
};

struct nodeList{

	void *bf;
	struct nodeList *next;

};

typedef struct nodeList_c *list;
list newList();
list insertElement(list l,void *x);
boolean isFullList(list l);
boolean isEmptyList(list l);
int getSize(list l);
void* getElement(list l,int index);
int compareElement(void *a,void *b);
list addAll(list l,list new);
list insertElementByIndex(list l,int index,void *newChild);
list deleteElement(list l,void *x);
list deleteElementByIndex(list l,int index);
int indexOfElement(list l,void *bfn);
list removeLastHalf(list l,int from);
