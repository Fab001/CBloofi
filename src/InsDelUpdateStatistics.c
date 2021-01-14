#include "InsDelUpdateStatistics.h"
#include <stdlib.h>
#include <stdio.h>

struct InsDelUpdateStatistics* InsDelUpdateStatistics(){
	struct InsDelUpdateStatistics* stat;
	stat=malloc(sizeof(struct InsDelUpdateStatistics));
	clear(stat);
	return stat;
}

void clear(struct InsDelUpdateStatistics *stat){
	stat->nbBFAccessed=0;
	stat->nbBFNodesAccessed=0;
	stat->nbSplits=0;
	stat->nbMerges=0;
	stat->nbRedistributes=0;
}

void toString(struct InsDelUpdateStatistics *stat){
	//printf("_______________________\n");
	printf("| nbBFAccessed:%ld |\n",stat->nbBFAccessed);
	printf("| nbBFNodesAccessed:%ld |\n",stat->nbBFNodesAccessed);
	printf("| nbSplits:%d |\n",stat->nbSplits);
	printf("| nbMerges:%d |\n",stat->nbMerges);
	printf("| nbRedistributes:%d |\n",stat->nbRedistributes);
	//printf("_______________________\n");
	//char *str;
	//str="| nbBFAccessed:%d | | nbBFNodesAccessed:%d | | nbSplits:%d | | nbMerges:%d | | nbRedistributes:%d |\n";

}

