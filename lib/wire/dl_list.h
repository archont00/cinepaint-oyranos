/* wire/dl_list.h
// Doubly-linked node for linked list support
// Copyright May 25, 2003, Robin.Rowe@MovieEditor.com
// License MIT (http://opensource.org/licenses/mit-license.php)
*/

#ifndef DL_LIST_H
#define DL_LIST_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DL_node DL_node;
typedef struct DL_list DL_list;

/* Always make DL_node first data member so it can be cast to your type! */
struct DL_node
{
#ifdef _DEBUG
	int is_trashed;
#endif
	DL_node* prev;
	DL_node* next;
};

struct DL_list
{	DL_node* head;
	DL_node* tail;
#ifdef _DEBUG
	int count;
#endif
};

void DL_init(DL_node* dl_node);
int DL_is_used_node(DL_list* dl_list,DL_node* dl_node);
void DL_append(DL_list* dl_list,DL_node* dl_node);
void DL_remove(DL_list* dl_list,DL_node* dl_node);
int DL_is_empty(DL_list* dl_list);
void DL_VerifyNodeInList(DL_list* dl_list,DL_node* dl_node);

#ifdef __cplusplus
}
#endif

#endif

