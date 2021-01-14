

struct InsDelUpdateStatistics{
	long nbBFAccessed;
	long nbBFNodesAccessed;
	int nbSplits;
	int nbMerges;
	int nbRedistributes;
};
struct InsDelUpdateStatistics* InsDelUpdateStatistics();
void clear(struct InsDelUpdateStatistics *stat);
void toString(struct InsDelUpdateStatistics *stat);
