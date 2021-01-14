/*
 * Nodo che contiene un Bloom filter
 */

#include "BFINode.h"
#include <stdlib.h>
#include <time.h>

/*
 * Costruisce un nodo
 */
struct BFINode* BFINode(struct bloom *value,int order,bool splitFull){
	struct BFINode *b;
	b=malloc(sizeof(struct BFINode));
	b->order=order;
	b->value=value;
	b->parent=NULL;
	b->splitFull=splitFull;
	b->children=newList();
	return b;
}


/*
 * Controlla se un nodo è una foglia
 */
bool isLeaf(struct BFINode *bfn){

	return (bfn->children->first==NULL);
}


/*
 * Restituisce il livello di un nodo nell'albero
 * Una foglia ha livello zero
 * La radice ha livello massimo
 */
int getLevel(struct BFINode *bfn){

	if(isLeaf(bfn)){
		//se foglia, ritorna 0
		return 0;
	}else{
		//calcola il livello dei figli e aggiunge 1
		struct BFINode *child=getElement(bfn->children,0);
		int childLevel=getLevel(child);
		return 1+childLevel;
	}
}


/*
 * Restituisce il numero di nodi nel sottoalbero
 * radicato in questo nodo
 */
int getTreeSize(struct BFINode *bfn){
	struct BFINode *currentNode;
	//se è foglia, ritorna 1
	if(isLeaf(bfn)){
		return 1;
	}
	//altrimenti,calcola il numero di nodi in ciascun sottoalbero
	//e li aggiunge in cima
	else{
		int size=1; //questo nodo
		for(int i=0;i<getSize(bfn->children);i++){
			currentNode=getElement(bfn->children,i);
			size+=getTreeSize(currentNode);
		}
		return size;
	}
}


/*
 * Restituisce il numero di bit nel Bloom filter
 */
int getBloomFilterSize(struct BFINode *bfn){
	return bfn->value->bits;
}


/*
 * Ricalcola il valore del Bloom filter in modo da essere l'OR dei suoi figli
 */
void recomputeValue(struct BFINode *bfn,struct InsDelUpdateStatistics* stat){

	bloom_clear(bfn->value);
	struct BFINode *currentNode;
	for(int i=0;i<bfn->children->size;i++){
		currentNode=getElement(bfn->children,i);
		or_bloom_filter(bfn->value,currentNode->value);
		stat->nbBFAccessed++;//usato il valore corrente
	}
	stat->nbBFAccessed;//computato questo valore
}


/*
 * Restituisce true se è necessario uno split
 */
bool needSplit(struct BFINode *bfn){

	if(bfn->splitFull){
		return !(bfn->children->first==NULL||bfn->children->size<=2*bfn->order);
	}else{
		return !(bfn->children->first==NULL||bfn->children->size<=2*bfn->order||isFull(bfn->value));
	}
}


/*
 * Restituisce true se non è la radice(una radice non ha mai bisogno di split)
 * e il numero di figli è inferiore del parametro order+1
 */
bool needMerge(struct BFINode *bfn){

	return bfn->parent!=NULL && bfn->children->size<bfn->order;
}


/*
 * Restituisce true se il nodo può essere ridistribuito
 * Cioè se ha una quantità di figli maggiore di order
 */
bool canRedistribute(struct BFINode *bfn){

	return bfn->children->size>bfn->order;
}


/*
 * Trova il nodo nella lista con il valore del BloomFilter più "vicino" al valore in input
 * È usata per direzionare la ricerca durante l'inserimento. Se la distanza tra il BloomFilter
 * in input e diversi Bloom filters nella lista è la stessa, restituisce uno di questi Bloom
 * filters in modo casuale
 */
struct BFINode* findClosest(struct BFINode *bfn,list nodeList,struct InsDelUpdateStatistics* stat){

	int index=findClosestIndex(bfn,nodeList,stat);
	if(index>=0){
		return getElement(nodeList,index);
	}
	else{
		return NULL;
	}
}


/*
 * Trova l'indice del nodo nella lista con il valore del BloomFilter più "vicino" al valore in input
 * È usata per direzionare la ricerca durante l'inserimento. Se la distanza tra il BloomFilter
 * in input e diversi Bloom filters nella lista è la stessa, restituisce uno di questi Bloom
 * filters in modo casuale
 */
int findClosestIndex(struct BFINode *b,list nodeList,struct InsDelUpdateStatistics* stat){

	//ritorna NULL se non ci sono elementi con cui compararlo
	if(isEmptyList(nodeList))
		return -1;

	struct BFINode *currentNode;

	//inizializza distanza minima che sarà la distanza del primo elemento
	currentNode=getElement(nodeList,0);
	double minDistance=computeDistance(b->value,currentNode->value);
	stat->nbBFAccessed+=2;//questo valore e il valore di currentNode
    int minIndex=0;
    double currentDistance;

    //loop tra tutti gli elementi per trovare il più simile
    for(int i=1;i<nodeList->size;i++){
    	currentNode=getElement(nodeList,i);
    	currentDistance=computeDistance(b->value,currentNode->value);
    	stat->nbBFAccessed+=2;//questo valore e il valore di currentNode

    	//rimpiazza il minimo corrente se ne trova uno più piccolo,
    	//o se è lo stesso, rimpiazza casualmente quello attuale
    	//Si valuta la probabilità:
    	//Se x nodi sono alla stessa distanza dal valore corrente,
    	//ciascun nodo dovrebbe essere restituito con probabilità 1/x
    	if(currentDistance<minDistance||(minDistance-currentDistance<0.00001&&((double)(rand())/((double)RAND_MAX+1.0))<1.0/nodeList->size)){
    		minDistance=currentDistance;
    		minIndex=i;
    	}
    }

    return minIndex;
}


