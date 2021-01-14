#include "SearchStatistics.h"

struct SearchStatistics* SearchStatistics(){
	struct SearchStatistics* stat;
	stat=malloc(sizeof(struct SearchStatistics));
	stat->nbBFChecks=0;
	return stat;
}

void clearSearch(struct SearchStatistics* stat){
	stat->nbBFChecks=0;
}
