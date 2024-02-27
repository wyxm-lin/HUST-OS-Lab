#include "pmm.h"
#include "util/functions.h"
#include "riscv.h"
#include "config.h"
#include "util/string.h"
#include "memlayout.h"
#include "spike_interface/spike_utils.h"

// _end is defined in kernel/kernel.lds, it marks the ending (virtual) address of PKE kernel
extern char _end[];
// g_mem_size is defined in spike_interface/spike_memory.c, it indicates the size of our
// (emulated) spike machine. g_mem_size's value is obtained when initializing HTIF.
extern uint64 g_mem_size;

static uint64 free_mem_start_addr; // beginning address of free memory
static uint64 free_mem_end_addr;   // end address of free memory (not included)

/***************************start of 线性数组实现引用计数************************/
typedef struct RefCount
{
	uint64 pa;
	uint64 ref;
} RefCount;
RefCount RefCountList[0x4000];
uint64 RefTotal = 0;

enum Bool ref_find(uint64 pa)
{
	for (int i = 0; i < RefTotal; i++)
	{
		if (RefCountList[i].pa == pa)
		{
			RefCountList[i].ref++;
			return True;
		}
	}
	return False;
}

void ref_insert(uint64 pa)
{
	if (ref_find(pa) == True)
		return;
	if (RefTotal >= 0x4000)
		panic("RefTotal is out of range\n");
	RefCountList[RefTotal].pa = pa;
	RefCountList[RefTotal].ref = 1;
	RefTotal++;
}

enum Bool ref_erase(uint64 pa)
{
	for (int i = 0; i < RefTotal; i++)
	{
		if (RefCountList[i].pa == pa)
		{
			RefCountList[i].ref--;
			if (RefCountList[i].ref == 0)
			{
				RefCountList[i].pa = RefCountList[RefTotal - 1].pa;
				RefCountList[i].ref = RefCountList[RefTotal - 1].ref;
				RefTotal--;
				return True;
			}
			return False;
		}
	}
	return True;
}

/*************************end of 线性数组实现引用计数************************************/

/************************start of 红黑树实现引用计数*******************/
#define RBNodeMax 0x1000
RBNode AllRBNode[RBNodeMax];
RBTree tr;
uint64 AllRBNodeTotal = 0;

// 创建一个新节点
RBNode *rb_node_create(int key)
{
	if (AllRBNodeTotal >= RBNodeMax)
		panic("AllRBNodeTotal is out of range\n");
	RBNode *node = &AllRBNode[AllRBNodeTotal++];
	node->key = key;
	node->color = RED; // 新插入的节点默认为红色
	node->parent = NULL;
	node->left = NULL;
	node->right = NULL;
	return node;
}

// 左旋操作
void rb_left_rotate(RBTree *tree, RBNode *x)
{
	RBNode *y = x->right;
	x->right = y->left;
	if (y->left != NULL)
	{
		y->left->parent = x;
	}
	y->parent = x->parent;
	if (x->parent == NULL)
	{
		tree->root = y;
	}
	else if (x == x->parent->left)
	{
		x->parent->left = y;
	}
	else
	{
		x->parent->right = y;
	}
	y->left = x;
	x->parent = y;
}

// 右旋操作
void rb_right_rotate(RBTree *tree, RBNode *y)
{
	RBNode *x = y->left;
	y->left = x->right;
	if (x->right != NULL)
	{
		x->right->parent = y;
	}
	x->parent = y->parent;
	if (y->parent == NULL)
	{
		tree->root = x;
	}
	else if (y == y->parent->left)
	{
		y->parent->left = x;
	}
	else
	{
		y->parent->right = x;
	}
	x->right = y;
	y->parent = x;
}

// 插入节点修正
void rb_insert_fixup(RBTree *tree, RBNode *z)
{
	while (z != tree->root && (z == NULL || z->color == RED))
	{
		if (z == z->parent->left)
		{
			RBNode *y = z->parent->right;
			if (y != NULL && y->color == RED)
			{
				z->parent->color = BLACK;
				y->color = BLACK;
				z->parent->parent->color = RED;
				z = z->parent->parent;
			}
			else
			{
				if (z == z->parent->right)
				{
					z = z->parent;
					rb_left_rotate(tree, z);
				}
				z->parent->color = BLACK;
				z->parent->parent->color = RED;
				rb_right_rotate(tree, z->parent->parent);
			}
		}
		else
		{
			RBNode *y = z->parent->left;
			if (y != NULL && y->color == RED)
			{
				z->parent->color = BLACK;
				y->color = BLACK;
				z->parent->parent->color = RED;
				z = z->parent->parent;
			}
			else
			{
				if (z == z->parent->left)
				{
					z = z->parent;
					rb_right_rotate(tree, z);
				}
				z->parent->color = BLACK;
				z->parent->parent->color = RED;
				rb_left_rotate(tree, z->parent->parent);
			}
		}
	}
	tree->root->color = BLACK;
}

// 插入节点
void rb_insert(RBTree *tree, int key)
{
	RBNode *z = rb_node_create(key);
	RBNode *y = NULL;
	RBNode *x = tree->root;
	while (x != NULL)
	{
		y = x;
		if (z->key < x->key)
		{
			x = x->left;
		}
		else
		{
			x = x->right;
		}
	}
	z->parent = y;
	if (y == NULL)
	{
		tree->root = z;
	}
	else if (z->key < y->key)
	{
		y->left = z;
	}
	else
	{
		y->right = z;
	}
	rb_insert_fixup(tree, z);
}

// 删除节点修正
void rb_delete_fixup(RBTree *tree, RBNode *x)
{
	while (x != tree->root && (x == NULL || x->color == BLACK))
	{
		if (x == x->parent->left)
		{
			RBNode *w = x->parent->right;
			if (w->color == RED)
			{
				w->color = BLACK;
				x->parent->color = RED;
				rb_left_rotate(tree, x->parent);
				w = x->parent->right;
			}
			if ((w->left == NULL || w->left->color == BLACK) &&
				(w->right == NULL || w->right->color == BLACK))
			{
				w->color = RED;
				x = x->parent;
			}
			else
			{
				if (w->right == NULL || w->right->color == BLACK)
				{
					w->left->color = BLACK;
					w->color = RED;
					rb_right_rotate(tree, w);
					w = x->parent->right;
				}
				w->color = x->parent->color;
				x->parent->color = BLACK;
				if (w->right != NULL)
				{
					w->right->color = BLACK;
				}
				rb_left_rotate(tree, x->parent);
				x = tree->root;
			}
		}
		else
		{
			RBNode *w = x->parent->left;
			if (w->color == RED)
			{
				w->color = BLACK;
				x->parent->color = RED;
				rb_right_rotate(tree, x->parent);
				w = x->parent->left;
			}
			if ((w->right == NULL || w->right->color == BLACK) &&
				(w->left == NULL || w->left->color == BLACK))
			{
				w->color = RED;
				x = x->parent;
			}
			else
			{
				if (w->left == NULL || w->left->color == BLACK)
				{
					w->right->color = BLACK;
					w->color = RED;
					rb_left_rotate(tree, w);
					w = x->parent->left;
				}
				w->color = x->parent->color;
				x->parent->color = BLACK;
				if (w->left != NULL)
				{
					w->left->color = BLACK;
				}
				rb_right_rotate(tree, x->parent);
				x = tree->root;
			}
		}
	}
	if (x != NULL)
	{
		x->color = BLACK;
	}
}

// 删除节点
void rb_delete(RBTree *tree, int key)
{
	RBNode *z = rb_search(tree, key);
	if (z == NULL)
	{
		return; // 如果找不到要删除的节点，则直接返回
	}
	RBNode *y = z;
	Color original_color = y->color;
	RBNode *x;
	if (z->left == NULL)
	{
		x = z->right;
		rb_transplant(tree, z, z->right);
	}
	else if (z->right == NULL)
	{
		x = z->left;
		rb_transplant(tree, z, z->left);
	}
	else
	{
		y = rb_minimum(z->right);
		original_color = y->color;
		x = y->right;
		if (y->parent == z)
		{
			if (x != NULL)
			{
				x->parent = y;
			}
		}
		else
		{
			rb_transplant(tree, y, y->right);
			y->right = z->right;
			if (y->right != NULL)
			{
				y->right->parent = y;
			}
		}
		rb_transplant(tree, z, y);
		y->left = z->left;
		y->left->parent = y;
		y->color = z->color;
	}
	free(z);
	if (original_color == BLACK)
	{
		rb_delete_fixup(tree, x);
	}
}

// 查找节点
RBNode *rb_search(RBTree *tree, int key)
{
	RBNode *x = tree->root;
	while (x != NULL && x->key != key)
	{
		if (key < x->key)
		{
			x = x->left;
		}
		else
		{
			x = x->right;
		}
	}
	return x; // 返回找到的节点指针，如果未找到则返回空指针
}

/************************end of 红黑树实现引用计数****************************************/
typedef struct node
{
	struct node *next;
} list_node;

// g_free_mem_list is the head of the list of free physical memory pages
static list_node g_free_mem_list;

//
// actually creates the freepage list. each page occupies 4KB (PGSIZE), i.e., small page.
// PGSIZE is defined in kernel/riscv.h, ROUNDUP is defined in util/functions.h.
//
static void create_freepage_list(uint64 start, uint64 end)
{
	g_free_mem_list.next = 0;
	for (uint64 p = ROUNDUP(start, PGSIZE); p + PGSIZE < end; p += PGSIZE)
		free_page((void *)p);
}

//
// place a physical page at *pa to the free list of g_free_mem_list (to reclaim the page)
//
void free_page(void *pa)
{
	if (((uint64)pa % PGSIZE) != 0 || (uint64)pa < free_mem_start_addr || (uint64)pa >= free_mem_end_addr)
		panic("free_page 0x%lx \n", pa);
	if (ref_erase((uint64)pa) == False)
		return;
	// insert a physical page to g_free_mem_list
	list_node *n = (list_node *)pa;
	n->next = g_free_mem_list.next;
	g_free_mem_list.next = n;
}

//
// takes the first free page from g_free_mem_list, and returns (allocates) it.
// Allocates only ONE page!
//
void *alloc_page(void)
{
	list_node *n = g_free_mem_list.next;
	if (n)
		g_free_mem_list.next = n->next;
	ref_insert((uint64)n);
	return (void *)n;
}

//
// pmm_init() establishes the list of free physical pages according to available
// physical memory space.
//
void pmm_init()
{
	// start of kernel program segment
	uint64 g_kernel_start = KERN_BASE; // KERN_BASE = 0x80000000
	uint64 g_kernel_end = (uint64)&_end;

	uint64 pke_kernel_size = g_kernel_end - g_kernel_start;
	sprint("PKE kernel start 0x%lx, PKE kernel end: 0x%lx, PKE kernel size: 0x%lx .\n",
		   g_kernel_start, g_kernel_end, pke_kernel_size);
	// DEBUG
	// sprint("                                                    RefCountList address is %lx\n", RefCountList );
	// sprint("                                                    RefTotal address is %lx\n", &RefTotal );
	// free memory starts from the end of PKE kernel and must be page-aligined
	free_mem_start_addr = ROUNDUP(g_kernel_end, PGSIZE);

	// recompute g_mem_size to limit the physical memory space that our riscv-pke kernel
	// needs to manage
	g_mem_size = MIN(PKE_MAX_ALLOWABLE_RAM, g_mem_size);
	if (g_mem_size < pke_kernel_size)
		panic("Error when recomputing physical memory size (g_mem_size).\n");

	free_mem_end_addr = g_mem_size + DRAM_BASE;
	sprint("free physical memory address: [0x%lx, 0x%lx] \n", free_mem_start_addr,
		   free_mem_end_addr - 1);

	sprint("kernel memory manager is initializing ...\n");
	// create the list of free pages
	create_freepage_list(free_mem_start_addr, free_mem_end_addr);
}
