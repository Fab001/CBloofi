#include <stdio.h>
#include "bloofi.h"
#include "time.h"

//***********FUNZIONI DI INIZIALIZZAZIONE***************

/*
 * Crea un Bloofi vuoto con solo la radice
 */
struct bloofi* bloom_filter_index(int order,struct bloom *sampleFilter,bool splitFull){

  struct bloofi*bl;
  bl=malloc(sizeof(struct bloofi));
  bl->splitFull=splitFull;
  bl->order=order;
  bl->bfList=newList();
  for(int i=0;i<SIZE;i++)
	  bl->hashTable[i]=NULL;

  //Inizializza Bloofi con una radice con Bloom filters tutti settati a zero
  struct bloom* zeroFilter=createZeroBloomFilter(sampleFilter);
  bl->root=BFINode(zeroFilter,bl->order,bl->splitFull);
  return bl;
}


/*
 * Crea un Bloom filter settato a zero con quello ottenuto in input
 */
struct bloom* createZeroBloomFilter(struct bloom *filter){

 struct bloom *zeroFilter;
 zeroFilter=malloc(sizeof(struct bloom));
 bloom_init(zeroFilter,filter->entries, filter->error,filter->metric,filter->h);
 assert(zeroFilter->bits==filter->bits);
 return zeroFilter;
}


/*
 * Crea un Bloofi con i Bloom filters ricevuti in input
 */
struct bloofi* bloom_filter_index2(list bList,int order,bool splitFull,struct InsDelUpdateStatistics* stat){

 struct bloofi*bl;
 bl=malloc(sizeof(struct bloofi));
 bl->order=order;
 bl->splitFull=splitFull;
 for(int i=0;i<SIZE;i++)
 bl->hashTable[i]=NULL;
 bl->bfList=bulkload(bl,bList,stat);
 return bl;
}


/*
 * Carica un Bloofi. Cambia il campo radice
 */
list bulkload(struct bloofi *bl,list mbfList,struct InsDelUpdateStatistics* stat){

	//ordina la lista ricevuta secondo una certa metrica
	mbfList=sort(mbfList);

	//prende un puntatore alla foglia più a destra
	struct BFINode*rightmost;

	//inizializza il Bloofi con una radice con un Bloom filter settato a zero
	struct bloom*sampleFilter=getElement(mbfList,0);
	struct bloom*zeroFilter=createZeroBloomFilter(sampleFilter);
	bl->root=BFINode(zeroFilter,bl->order,bl->splitFull);
	rightmost=bl->root;

	//inserisce ciascun Bloom filter nella foglia più a destra
	//se necessario esegue lo split
	struct BFINode*current;
	for(int i=0;i<mbfList->size;i++){
		struct bloom*bf=getElement(mbfList,i);
		//crea un nodo per questo Bloom filter
		current=BFINode(bf,bl->order,bl->splitFull);

		//inserisce l'ID mappando il nodo nella tabella hash
		insert(bl->hashTable,bf->id,current);
		rightmost=insertRight(bl,true,rightmost,current,rightmost,stat);

	}

	return mbfList;
}


//***********FUNZIONI DI RICERCA***************

/*
 * Cerca un oggetto nel Bloofi e restituisce i Bloom filters che lo contengono
 */
list searchBloomFilters(struct bloofi*bl,const void *o,struct SearchStatistics *stat){

	return findMatches(bl->root,o,stat);

}

int* search(struct bloofi*bl,const void*o,struct SearchStatistics *stat){

	list x=searchBloomFilters(bl,o,stat);
	int size=x->size;
	int *ans;
	ans=malloc(sizeof(int)*size);
	struct bloom *bf;
	for(int i=0;i<size;i++){
		bf=getElement(x,i);
		ans[i]=bf->id;
	}
	return ans;
}


/*
 * Cerca un oggetto nel sottoalbero radicato al nodo in input
 * e restituisce i Bloom filters che lo contengono
 */
list findMatches(struct BFINode *node,const void*o,struct SearchStatistics *stat){

 list result;
 stat->nbBFChecks++;
 result=newList();

 //se non c'è match con l'oggetto
 //restituisce l'insieme vuoto, altrimenti cerca tra i discendenti
 if(!bloom_check(node->value,o)){
  return result;
 }

 //se il nodo è una foglia restituisce il valore
 if(isLeaf(node)){
  result=insertElement(result,node->value);
  return result;
 }

 //se non è una foglia, cerca tra i discendenti
 for(int i=0;i<node->children->size;i++){
     result=addAll(result,findMatches(getElement(node->children,i),o,stat));
 }

 return result;
}


/*
 * Cerca un oggetto nel sottoalbero radicato al nodo in input e restituisce
 * i Bloom filters che lo contengono, usando un approccio naive in cui
 * solo i nodi foglia vengono controllati
 */
list naivefindMatches(struct BFINode *node,const void*o,struct SearchStatistics *stat){

	list result;
	result=newList();

	//incrementa il numero di Bloom filters controllati,
	//dato che questo nodo verrà controllato
	stat->nbBFChecks++;
	if(isLeaf(node)){
		if(bloom_check(node->value,o)){
			result=insertElement(result,node->value);
		}
		return result;
	}
	for(int i=0;i<node->children->size;i++){
		result=addAll(result,naivefindMatches(getElement(node->children,i),o,stat));
	}
	return result;
}


//***********FUNZIONI DI AGGIORNAMENTO***************

/*
 * Aggiorna il valore del Bloofi con il nuovo valore
 * di un Bloom filter dato in input
 */
int updateIndex(struct bloofi* bl,struct bloom *newBloomFilter,struct InsDelUpdateStatistics* stat){

int id=newBloomFilter->id;
//trova il nodo che corrisponde all'ID
struct BFINode *node;
node=searchHash(bl->hashTable,id);
if(node ==NULL){
	printf("ERRORE: Nodo con ID %d non trovato",id);
 return -1;
}
updateValueToTheRoot(node,newBloomFilter,stat);
return 0;

}


/*
 * Aggiorna il valore del nodo corrente e dei suoi ancestori
 * per contenere il nuovo valore
 */
void updateValueToTheRoot(struct BFINode *current,struct bloom *newValue,struct InsDelUpdateStatistics* stat){

 //aggiorna il valore del nodo corrente
 or_bloom_filter(current->value,newValue);
 stat->nbBFAccessed+=2;
 //se necessario, aggiorna i genitori ricorsivamente
 if(current->parent!=NULL){
  updateValueToTheRoot(current->parent,newValue,stat);
 }
}


//***********FUNZIONI DI INSERIMENTO***************

/*
 * Inserisce un Bloom filter come foglia nel Bloofi
 */
void insertBloomFilter(struct bloofi *bl,struct bloom *b,struct InsDelUpdateStatistics* stat){

 //crea un nuovo nodo per il Bloom filter inserito
 struct BFINode *newBFINode;
 newBFINode=BFINode(b,bl->order,bl->splitFull);

 //mappa nell'hashtable il nodo col Bloom filter
 insert(bl->hashTable,b->id,newBFINode);
 bl->bfList=insertElement(bl->bfList,b);

 //caso speciale in cui è il primo figlio della radice
 if(bl->root->children->first == NULL){

  //aggiunge il nuovo figlio a destra
  bl->root->children=insertElement(bl->root->children,newBFINode);

  newBFINode->parent=bl->root;

  //aggiorna il valore in modo che sia il valore corrente o il valore del figlio
  or_bloom_filter(bl->root->value,b);

  //aggiorna le statistiche
  stat->nbBFNodesAccessed++; //accessi alla radice
  stat->nbBFNodesAccessed++; //accessi al nuovo nodo
  stat->nbBFAccessed+=2; //accessi alla radice e ai nuovi Bloom filters
 }

 else{

  insert2(bl,bl->root,newBFINode,stat);

 }

}


/*
 * Inserisce un nuovo figlio nel sottoalbero radicato nel nodo "current",
 * restituisce NULL o un puntatore ad un nuovo figlio se lo split è necessario
 */
struct BFINode* insert2(struct bloofi *bl,struct BFINode *current,struct BFINode *newChild,struct InsDelUpdateStatistics* stat){

stat->nbBFNodesAccessed++;//nodo corrente acceduto

//se non è una foglia è necessario direzionare la ricerca
if(!isLeaf(current)){
  //aggiorna il valore del nodo corrente
  //appena verrà inserito in quel sottoalbero
  or_bloom_filter(current->value,newChild->value);
  stat->nbBFAccessed+=2; //valore di current e new child

  //trova il figlio più simile a newChild e inserisce lì
  struct BFINode* closestChild=findClosest(newChild,current->children,stat);

  //inserisce in quel sottoalbero
  struct BFINode *newSibling=insert2(bl,closestChild,newChild,stat);

  //se newSibling è NULL(no split), restituisce NULL
  if(newSibling==NULL){
    return NULL;
  }
  //c'è uno split
  else{
   //controlla se è necessaria una nuova radice
   if(current->parent==NULL){
    struct bloom *tempFilter=createZeroBloomFilter(current->value);
    //la radice è splittata, creata una nuova radice
    struct BFINode *newRoot=BFINode(tempFilter,bl->order,bl->splitFull);
    or_bloom_filter(newRoot->value,current->value);
    or_bloom_filter(newRoot->value,newSibling->value);
    newRoot->children=insertElement(newRoot->children,current);
    current->parent=newRoot;
    newRoot->children=insertElement(newRoot->children,newSibling);
    newSibling->parent=newRoot;
    bl->root=newRoot;
    stat->nbBFAccessed+=3;
    stat->nbBFNodesAccessed+=3;

   return NULL;

   //se non è la radice
  }else{

   newSibling=insertEntryIntoParent(bl,newSibling,current,stat);
   return newSibling;
  }

 }//fine split

}
//se il nodo current è una foglia, necessita di essere inserito nel padre
else{
struct BFINode *newSibling=insertEntryIntoParent(bl,newChild,current,stat);
	 return newSibling;
}

}

/*
 * Inserisce un nuovo figlio nel genitore del nodo fornito
 */
struct BFINode* insertEntryIntoParent(struct bloofi *bl,struct BFINode *newChild,struct BFINode *node,struct InsDelUpdateStatistics* stat){

//trova la posizione del nodo corrente tra i suoi vicini
int index=indexOfElement(node->parent->children,node);
stat->nbBFNodesAccessed++;//genitori acceduto

//inserisce il nuovo figlio dopo questo
node->parent->children=insertElementByIndex(node->parent->children,index+1,newChild);
newChild->parent=node->parent;
stat->nbBFNodesAccessed+=2; //accesso a genitore e nuovo vicino


//controlla se è necessario lo split
stat->nbBFNodesAccessed++;
if(!needSplit(node->parent)){

 return NULL;
}
//altrimenti, è necessario splittare il nodo

struct BFINode *nuovoNodo= split(bl,node->parent,stat);
return nuovoNodo;

}


/*
 * Esegue split di un nodo
 */
struct BFINode *split(struct bloofi *bl,struct BFINode *current,struct InsDelUpdateStatistics* stat){

 struct BFINode *newNode;
 struct BFINode *newChild;
 struct bloom* zeroFilter;

 zeroFilter=malloc(sizeof(struct bloom));
 bloom_init(zeroFilter,current->value->entries,current->value->error,current->value->metric,current->value->h);
 newNode=BFINode(zeroFilter,bl->order,bl->splitFull);
 stat->nbSplits++; //incrementa numero di split

 //inserisce la seconda metà dei figli del nodo current
 //nel nuovo nodo
 for(int i=bl->order+1;i<current->children->size;i++){
  //preleva il nuovo figlio
  newChild=getElement(current->children,i);
  //inserisce il nuovo figlio a destra
  newNode->children=insertElement(newNode->children,newChild);
  newChild->parent=newNode;

  or_bloom_filter(newNode->value,newChild->value);
 }

 stat->nbBFNodesAccessed+=newNode->children->size+1;
 stat->nbBFAccessed+=newNode->children->size+1;
 //primo estremo incluso-secondo secondo escluso
 current->children=removeLastHalf(current->children,bl->order+1);
 stat->nbBFNodesAccessed++;
 recomputeValue(current,stat);
 return newNode;
}


/**************************************************************************/

//***********FUNZIONI DI CANCELLAZIONE***************


/*
 * Elimina il Bloom filter con l'ID in input
 */
int deleteFromIndex(struct bloofi* bl,int id,struct InsDelUpdateStatistics* stat){
 struct BFINode *node;

 node=searchHash(bl->hashTable,id);

 if(node==NULL){
  return -1;
 }
 else{

 }
 deleteNode(bl,node,stat);

 //elimina dalla bfList e dall'idMap
 delete(bl->hashTable,id);
 bl->bfList=deleteElement(bl->bfList,node->value);

 return 0;
}


/*
 * Elimina il nodo in input dal Bloofi. L'eliminazione agisce in modo bottom-up
 */
void deleteNode(struct bloofi* bl,struct BFINode *childNode,struct InsDelUpdateStatistics* stat){

 if(bl->root->children->size<2){
	// printf("ERROR:nb children of root is:%d\n",getSize(bl->root->children));
	// assert(false);
 }

 //elimina il nodo dalla lista dei figli del padre
 struct BFINode *node=childNode->parent;
 stat->nbBFNodesAccessed+=2;
 node->children=deleteElement(node->children,childNode);

 //controlla se l'altezza dell'albero deve essere ridotta
 //è il caso in cui il genitore è la radice
 //e rimangono solo figli non foglia
 if(node==bl->root&&node->children->size==1){
  if(!isLeaf(getElement(node->children,0))){
     bl->root=getElement(node->children,0);
     bl->root->parent=NULL;
     stat->nbBFNodesAccessed++; //radice cambiata
     return;
  }
 }
 stat->nbBFNodesAccessed++; //merge eseguito

 //controlla se si verifica un underflow nel genitore
 if(!needMerge(node)){
	 //nessun underflow, aggiorna i valori
    recomputeValueToTheRoot(node,stat);
 }else{
	//prova a redistribuire
	//prende un vicino del nodo
	int index=indexOfElement(node->parent->children,node);
	stat->nbBFNodesAccessed+=2; //controllo della posizione per trovare il vicino
    struct BFINode *sibling;
    bool isRightSibling=false;
    //prova con il vicino a destra. Se non esiste prova con il successivo
    if(index+1<node->parent->children->size){
      isRightSibling=true;
      sibling=getElement(node->parent->children,index+1);
    }else{
	    if(index-1<0){
		  printf("Errore\n");
	    }
    isRightSibling=false;
    sibling=getElement(node->parent->children,index-1);
     }
    stat->nbBFNodesAccessed++; //prende il vicino
    //vede se il vicino può ridistribuire
    stat->nbBFNodesAccessed++; //controlla se può ridistribuire
    if(canRedistribute(sibling)){
	   redistribute(node,sibling,isRightSibling,stat);

    }else{
	  merge(node,sibling,isRightSibling,stat);
	  //cancella il nodo
	  deleteNode(bl,node,stat);
    }
 }

 return;
}


/*
 * Ridistribuisce le entrate tra due vicini e aggiorna i valori alla radice
 */
void redistribute(struct BFINode *node,struct BFINode *sibling,bool isRightSibling,struct InsDelUpdateStatistics* stat){

	stat->nbRedistributes++;

	//prende il numero di entrate in entrambi
	int nbChildren=node->children->size+sibling->children->size;
	int nbChildren1=nbChildren/2;
	int nbChildren2=nbChildren-nbChildren1;
	int nbChildrenToGive=sibling->children->size-nbChildren2;

	stat->nbBFNodesAccessed+= 2; //vicini acceduti per prendere la dimensione
	struct BFINode*childToMove;
	if(isRightSibling){
		//muove prima "nbChildrenToGive da un vicino al nodo
		for(int i=0;i<nbChildrenToGive;i++){
			childToMove=getElement(sibling->children,0);
			sibling->children=deleteElementByIndex(sibling->children,0);
			node->children=insertElement(node->children,childToMove);
			childToMove->parent=node;
		}
	}else{
		//muove gli ultimi nbChildrenToGive da un vicino al nodo
		for(int i=0;i<nbChildrenToGive;i++){
			childToMove=getElement(sibling->children,sibling->children->size-1);
			sibling->children=deleteElementByIndex(sibling->children,sibling->children->size-1);
			node->children=insertElementByIndex(node->children,0,childToMove);
			childToMove->parent=node;
		}


	}
	//aggiorna le statistiche
	stat->nbBFNodesAccessed+=nbChildrenToGive + 2; //acceduti il nodo, il vicino e tutti i nuovi figli

	//ricalcola i valori per tutti i nodi coinvolti, fino alla radice
	recomputeValue(sibling,stat);
	recomputeValueToTheRoot(node,stat);
}



void recomputeValueToTheRoot(struct BFINode *current,struct InsDelUpdateStatistics* stat){

	//aggiorna il valore del nodo corrente
	recomputeValue(current,stat);

	//se necessario, aggiorna ricorsivamente il genitore
	if(current->parent!=NULL){
		recomputeValueToTheRoot(current->parent,stat);
	}
}
/*
 *  Fonde i figli di un nodo con un altro; tutti i figli di un nodo
 *  vengono forniti ad un vicino, aggiornando poi i valori in OR
 */
void merge(struct BFINode *node,struct BFINode *sibling,bool isRightSibling,struct InsDelUpdateStatistics* stat){

	stat->nbMerges++;

	//prende il numero di entrate da muovere
	int nbChildrenToGive=node->children->size;


	stat->nbBFNodesAccessed++;//accesso al nodo per la dimensione


	struct BFINode *childToMove;
	if(isRightSibling){
		//muove gli ultimi nbChildrenToGive dal nodo al vicino
		for(int i=0;i<nbChildrenToGive;i++){
			childToMove=getElement(node->children,node->children->size-1);
			node->children=deleteElementByIndex(node->children,node->children->size-1);
			sibling->children=insertElementByIndex(sibling->children,0,childToMove);
			or_bloom_filter(sibling->value,childToMove->value);
			childToMove->parent=sibling;

		}
	}else{
		//muove i primi nbChildrenToGive dal nodo al vicino
		for(int i=0;i<nbChildrenToGive;i++){
			childToMove=getElement(node->children,0);
			node->children=deleteElementByIndex(node->children,0);
			sibling->children=insertElement(sibling->children,childToMove);
			or_bloom_filter(sibling->value,childToMove->value);
			childToMove->parent=sibling;
		}


	}

	//aggiorna le statistiche
	stat->nbBFNodesAccessed+=nbChildrenToGive + 2;//accesso a nodo, vicino e tutti i nuovi figli
	stat->nbBFAccessed+=nbChildrenToGive + 1; //aggiunto nuovo figlio al valore

}

/*************************FUNZIONI GENERICHE****************************************

/*
 * Controllo di errori
 */
void validate(struct bloofi *bl){
	aggregateChildren(bl,bl->root);
}

/*
 * Aggrega i figli e controlla la consistenza
 */
struct bloom* aggregateChildren(struct bloofi *bl,struct BFINode *node){
	if(node->children->first==NULL){
		return node->value; //nulla da controllare
	}
	struct bloom* first=getElement(bl->bfList,0);
	struct bloom* current=createZeroBloomFilter(first);

	for(int i=0;i<node->children->size;i++){
		struct BFINode*c=getElement(node->children,i);
		struct bloom *r=aggregateChildren(bl,c);
		bitset_inplace_union(current->b,r->b);

	}
	if(node->value->b!=current->b){
		printf("Errore\n");
	}


	return node->value;
}


struct BFINode* splitRight(struct bloofi*bl,struct BFINode *current,struct BFINode *rightmost,struct InsDelUpdateStatistics* stat){

	stat->nbSplits++; //incrementa il numero di split
	struct BFINode *newNode;
	//inizializza il nuovo nodo con un Bloom filter settato a zero
	struct bloom *sampleFilter=current->value;
	struct bloom *zeroFilter=createZeroBloomFilter(sampleFilter);
	newNode=BFINode(zeroFilter,bl->order,bl->splitFull);

	//inserisce l'ultima metà dei figli correnti nel nuovo nodo
	struct BFINode *receivedRight;
	for(int i=bl->order+1;i<current->children->size;i++){
		receivedRight=insertRight(bl,false,newNode,getElement(current->children,i),rightmost,stat);

	}

	//rimuove l'ultima metà dei figli correnti nel nodo corrente
	current->children=removeLastHalf(current->children,bl->order+1);

	stat->nbBFNodesAccessed++; //cambiati i figli correnti

	//aggiorna il valore del nodo corrente in modo da essere l'OR dei suoi rimanenti figli
	recomputeValue(current,stat);

	// se current != radice, inserisce il nuovo vicino nel genitore
	if(current->parent!=NULL){
		receivedRight=insertRight(bl,true,current->parent,newNode,rightmost,stat);

	}
	//altrimenti c'è bisogno di creare una nuova radice
	else{

		struct BFINode *newRoot;
		//inizializza il nuovo nodo con un Bloom filter tutto settato a zero
		struct bloom *sampleFilter1=current->value;
		struct bloom *zeroFilter1=createZeroBloomFilter(sampleFilter1);
		newRoot=BFINode(zeroFilter1,bl->order,bl->splitFull);
		rightmost=insertRight(bl,false,newRoot,current,rightmost,stat);
		rightmost=insertRight(bl,false,newRoot,newNode,rightmost,stat);
		bl->root=newRoot;

	}
	//aggiorna i valori più a destra se il nodo corrente che splitta è più a destra
	if(current==rightmost){
		rightmost=newNode;
	}

	return rightmost;
}
/*
 * Inserisce un nuovo figlio nel nodo corrente come ultimo figlio
 */
struct BFINode* insertRight(struct bloofi *bl,bool isInBFI,struct BFINode *current,struct BFINode *newChild,struct BFINode *rightmost,struct InsDelUpdateStatistics* stat){

	//crea un array di figli se non esiste
	if(current->children==NULL){
	 	current->children=newList();

	}

	//aggiunge il nuovo figlio a destra
	current->children=insertElement(current->children,newChild);
	newChild->parent=current;
	stat->nbBFNodesAccessed+=2; // nodo corrente e nuovo figlio
	//aggiorna il valore in modo da avere il valore del nodo corrente o del figlio
	if(!isInBFI){
		or_bloom_filter(current->value,newChild->value);
		stat->nbBFAccessed+=2;
	}

	//se il figlio inserito è una foglia, aggiorna il valore di tutti i genitori fino alla radice
	//se il figlio inserito non è foglia, non c'è bisogno di aggiornare
	//dato che hanno già il valore corretto
	if(isLeaf(newChild)){
		updateValueToTheRoot(current,newChild->value,stat);
	}

	stat->nbBFNodesAccessed++; //controllo per lo split
	//controlla se è necessario lo split. Se non lo è restituisce il vecchio nodo
	if(!needSplit(current)){
		return rightmost;
	}

	//altrimenti esegue split del nodo
	rightmost=splitRight(bl,current,rightmost,stat);
	return rightmost;
}

/*
 * Ordina la lista in input in accordo a qualche metrica di distanza
 * Noi ora usiamo Hamming. Ordina in modo tale che il primo elemento
 * sia il più vicino a zero, il successivo il più vicino al primo
 * e così via
 */
list sort(list bf){
	return sortIterative(bf);
}


/*
 * Ordina la lista in input sulla base della distanza di Hamming
 */
list sortIterative(list bf){

	clock_t t_start, t_end;
	double t_passed;
	t_start = clock();

	list sorted;
	newList(&sorted);

	struct bloom*first=getElement(bf,0);

	//crea un Bloom filter settato a zero
	struct bloom *current=createZeroBloomFilter(first);

	struct bloom *closest;
	int closestIndex;

	//Trova in modo iterativo il Bloom filter più simile al corrente
	//spostandolo nella lista ordinata.
	//Il Bloom filter più simile diventa il corrente
	while(!isEmptyList(bf)){
		//trova il bloom filter più vicino al corrente
		closestIndex=findClosestBloom(current,bf);
		closest=getElement(bf,closestIndex);
		//lo aggiunge alla lista ordinata
		sorted=insertElement(sorted,closest);
		//lo rimuove dalla lista iniziale
		bf=deleteElementByIndex(bf,closestIndex);
		//viene reso nuovo corrente
		current=closest;
	}
	t_end = clock();

	t_passed = ((double)(t_end - t_start)) / CLOCKS_PER_SEC;
	printf("Elapsed time: %f seconds.\n", t_passed);
	return sorted;
}


/*
 * Trova il nodo nella lista con il valore del Bloom filter più simile
 *al valore in input. Viene usato per direzionare la ricerca durante l'inserimento.
 *Se la distanza tra più bloom filters è la stessa, viene restituito uno dei tanti
 *in modo casuale
 */
int findClosestBloom(struct bloom *b,list bloomList){

	if(isEmptyList(bloomList))
		return -1;
	struct bloom *currentBloom;
	currentBloom=getElement(bloomList,0);
	double minDistance=computeDistance(b,currentBloom);
    int minIndex=0;
    double currentDistance;
    for(int i=1;i<bloomList->size;i++){
    	currentBloom=getElement(bloomList,i);
    	currentDistance=computeDistance(b,currentBloom);
    	if(currentDistance<minDistance){
    		minDistance=currentDistance;
    		minIndex=i;
    	}
    }
    return minIndex;
}


/*
 * Restituisce l'altezza del Bloofi. (Un albero con solo radice e foglie ha altezza 1)
 */
int getHeightBloofi(struct bloofi *bf){
 return getLevel(bf->root);
}

/*
 * Restituisce il numero di nodi nel Bloofi
 */
int getSizeBloofi(struct bloofi *bf){

 return getTreeSize(bf->root);
}

/*
 * Restituisce il numero di bit di un Bloom filter nel Bloofi
 */
int getBloomFilterSizeBloofi(struct bloofi *bf){

 return getBloomFilterSize(bf->root);
}


/*
 * Restituisce il numero di figli della radice
 */
int getNbChildrenRootBloofi(struct bloofi *bf){

 return getSize(bf->root->children);
}

/*
 * Restituisce la lista di tutti i Bloom filters indicizzati
 */
list getBFListBloofi(struct bloofi *bf){

 return bf->bfList;
}
