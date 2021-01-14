#include <stdbool.h>
#include <assert.h>
#include "hashtable.h"
#ifndef _BLOOFI_H
#define _BLOOFI_H

#ifdef __cplusplus
extern "C" {
#endif
struct bloofi
{

  int order;
  bool splitFull;
  list bfList;
  struct DataItem*hashTable[SIZE];
  struct BFINode *root;
};

struct bloofi* bloom_filter_index(int order,struct bloom *sampleFilter,bool splitFull);
struct bloofi* bloom_filter_index2(list bList,int order,bool splitFull,struct InsDelUpdateStatistics* stat);
list findMatches(struct BFINode *node,const void*o,struct SearchStatistics *stat);
list naivefindMatches(struct BFINode *node,const void*o,struct SearchStatistics *stat);
struct BFINode *split(struct bloofi *bl,struct BFINode *current,struct InsDelUpdateStatistics* stat);
int updateIndex(struct bloofi* bl,struct bloom *newBloomFilter,struct InsDelUpdateStatistics* stat);
void updateValueToTheRoot(struct BFINode *current,struct bloom *newValue,struct InsDelUpdateStatistics* stat);
int deleteFromIndex(struct bloofi* bl,int id,struct InsDelUpdateStatistics* stat);
void deleteNode(struct bloofi* bl,struct BFINode *childNode,struct InsDelUpdateStatistics* stat);
struct bloom* createZeroBloomFilter(struct bloom *filter);
struct BFINode* insertEntryIntoParent(struct bloofi *bl,struct BFINode *newChild,struct BFINode *node,struct InsDelUpdateStatistics* stat);
void redistribute(struct BFINode *node,struct BFINode *sibling,bool isRightSibling,struct InsDelUpdateStatistics* stat);
void merge(struct BFINode *node,struct BFINode *sibling,bool isRightSibling,struct InsDelUpdateStatistics* stat);
void validate(struct bloofi *bl);
struct BFINode* splitRight(struct bloofi*bl,struct BFINode *current,struct BFINode *rightmost,struct InsDelUpdateStatistics* stat);
struct bloom* aggregateChildren(struct bloofi *bl,struct BFINode *node);
void recomputeValueToTheRoot(struct BFINode *current,struct InsDelUpdateStatistics* stat);
struct BFINode* insertRight(struct bloofi *bl,bool isInBFI,struct BFINode *current,struct BFINode *newChild,struct BFINode *rightmost,struct InsDelUpdateStatistics* stat);
list bulkload(struct bloofi *bl,list mbfList,struct InsDelUpdateStatistics* stat);
int* search(struct bloofi*bl,const void*o,struct SearchStatistics *stat);
list searchBloomFilters(struct bloofi*bl,const void *o,struct SearchStatistics *stat);
int findClosestBloom(struct bloom *bf,list bloomList);
int getHeightBloofi(struct bloofi *bf);
int getSizeBloofi(struct bloofi *bf);
int getBloomFilterSizeBloofi(struct bloofi *bf);
int getNbChildrenRootBloofi(struct bloofi *bf);
list getBFListBloofi(struct bloofi *bf);
void insertBloomFilter(struct bloofi *bl,struct bloom *b,struct InsDelUpdateStatistics* stat);
struct BFINode* insert2(struct bloofi *bl,struct BFINode *current,struct BFINode *newChild,struct InsDelUpdateStatistics* stat);
list sortIterative(list bf);
list sort(list bf);
#ifdef __cplusplus
}
#endif

#endif
