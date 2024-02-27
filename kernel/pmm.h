#ifndef _PMM_H_
#define _PMM_H_

// Initialize phisical memeory manager
void pmm_init();
// Allocate a free phisical page
void* alloc_page();
// Free an allocated page
void free_page(void* pa);

// /********************start of RBTree*************************/
// #include "riscv.h"
// // 手搓红黑树(不太好管理内核空间)
// typedef enum {RED, BLACK} Color;

// typedef struct RBNode {
//     uint64 key;
//     uint64 value;
//     Color color;
//     struct RBNode* left;
//     struct RBNode* right;
//     struct RBNode* parent;
// } RBNode;
// typedef struct RBTree {
//     RBNode* root;
// }RBTree;

// extern RBTree tr;

// /********************end of RBTree***************************/

#endif