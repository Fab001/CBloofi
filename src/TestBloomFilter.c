#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "bloofi.h"
#include "assert.h"
#include <sys/time.h>
#include <sys/resource.h>

#define DIM 1000
#define RUN 1
struct bloom* createBloomFilter(float falsePosProb, int expectedNbElemInFilter,struct hasher*h,int metric,int num);

int main(int argc,char**argv) {

	 clock_t t_start,t_end;
	 double t_passed;
	 double totInsertTime,totUpdateTime,totSearchTime,totDeleteTime;
	 struct InsDelUpdateStatistics *stat2;
	 stat2= InsDelUpdateStatistics();
	 struct SearchStatistics*stat;
	 int num[1];
	 stat=SearchStatistics();

	for(int i=0;i<RUN;i++){

	    struct bloofi *bl;
	    struct hasher *h = init_hasher();
	    list bfList=newList();
	    struct bloom*current;

	 //inizializzo una lista di bloom filter
	   for(int i=0;i<DIM;i++){
	       current=createBloomFilter(0.01,10000,h,1,i+1);
	       bfList=insertElement(bfList,current);

	   }

	  //inizializzo il Bloofi
	  struct bloom bloomRoot;
	  bloom_init(&bloomRoot,10000,0.01,1,h);
	  bl=bloom_filter_index(2,&bloomRoot,false);

	 //inserisco i BloomFilter nel bloofi
	 for(int i=0;i<DIM;i++){
	   current=getElement(bfList,i);
	   t_start=clock();
	   insertBloomFilter(bl,current,stat2);
	   t_end=clock();
	   t_passed = ((double)(t_end - t_start)) / CLOCKS_PER_SEC;
	   totInsertTime+=t_passed;
	  // fprintf(fd, "%f\n", t_passed*1000);
	   //fprintf(fd, "%d\n", i+1);
	 }
    //Verifico corretta costruzione albero
	 struct BFINode *node;
	 struct BFINode *childNode;
	 /*
	 printf("IDROOT:%d\n",bl->root->value->id);
	 for(int i=0;i<bl->root->children->size;i++){
	     node=getElement(bl->root->children,i);
	     printf("ID PARENT:%d\n",node->value->id);
	     printf("SIZE CHILD:%d\n",node->children->size);

	      for(int j=0;j<node->children->size;j++){
	          childNode=getElement(node->children,j);
	          printf("ID:%d\n",childNode->value->id);
	      }
	 }
*/


	 //aggiorno i BloomFilter nel Bloofi
	 //inserendo valori da 10 a 21
	 //un valore per ciascun BF in modo crescente
	 for(int i=0;i<DIM;i++){
	   current=getElement(bfList,i);
	   num[0]=i+1+1000;
	   bloom_add(current,num);
	   t_start=clock();
	   updateIndex(bl,current,stat2);
	   t_end=clock();
	   t_passed = ((double)(t_end - t_start)) / CLOCKS_PER_SEC;
	   totUpdateTime+=t_passed;

	 }



	 //cerca nell'indice i valori inseriti
	 int *find;
	 int numCerca;

	 for(int i=0;i<DIM;i++){
		 numCerca=i+1;
		 t_start=clock();
	     find=search(bl,&numCerca,stat);
	     free(find);
	     t_end=clock();
	     t_passed = ((double)(t_end - t_start)) / CLOCKS_PER_SEC;
	     totSearchTime+=t_passed;

	     //printf("Trovato ID:%d con valore ",*find);
	     //printf("%d inserito\n",numCerca);
	 }





	 //elimino 10 nodi.
	 //Radice rimane con due figli(ID 11 e 12)

	 for(int i=0;i<DIM;i++){
		 t_start=clock();
		 deleteFromIndex(bl,i+1,stat2);
		 t_end=clock();
		 t_passed = ((double)(t_end - t_start)) / CLOCKS_PER_SEC;
		 totDeleteTime+=t_passed;
		// fprintf(fd, "%f\n", t_passed*1000);
	 }

	 free(bl);
	}
	printf("Elapsed  insert time: %f ms.\n", totInsertTime*1000);
	printf("Elapsed  update time: %f ms.\n", totUpdateTime*1000);
	printf("Elapsed  search time: %f ms.\n", totSearchTime*1000);
	printf("Elapsed  delete time: %f ms.\n", totDeleteTime*1000);




	 return EXIT_SUCCESS;
}

 struct bloom* createBloomFilter(float falsePosProb, int expectedNbElemInFilter,
		                         struct hasher*h,int metric,int num){
  struct bloom* bf;
 // int num;
   bf=malloc(sizeof(struct bloom));
   bloom_init(bf,expectedNbElemInFilter,falsePosProb,metric,h);
  // for(int i=0;i<DIM;i++){
	 // num=i;
	  bloom_add(bf,&num);
   //}
   return bf;
 }


