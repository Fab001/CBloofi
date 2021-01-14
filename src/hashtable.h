#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "BFINode.h"

//dimensione dell'hashTable
#define SIZE 10000

struct DataItem {
   struct BFINode *data;
   int key;
};


struct DataItem* dummyItem;
struct DataItem* item;

int hashCode(int key);

struct BFINode *searchHash(struct DataItem** hashArray,int key);

void insert(struct DataItem** hashArray,int key,struct BFINode* data);

void delete(struct DataItem** hashArray,int key);

void display(struct DataItem** hashArray);
