/* wire/dl_list.cpp
// Doubly-linked node for linked list support
// Copyright May 25, 2003, Robin.Rowe@MovieEditor.com
// License MIT (http://opensource.org/licenses/mit-license.php)
*/

#include "dl_list.h"

void DL_init(DL_node* dl_node)
{	dl_node->prev=dl_node->next=0;
#ifdef _DEBUG
	dl_node->is_trashed=0;
#endif
}

int DL_is_used_node(DL_list* dl_list,DL_node* dl_node)
{	return ((dl_list->head==dl_node) || (0!=dl_node->prev));
}

int DL_is_empty(DL_list* dl_list)
{	return 0==dl_list->head;
}

#ifdef _DEBUG
#define CHECK(node) if(node->is_trashed) __asm int 3
#else
#define CHECK(node) 
#endif

void DL_append(DL_list* dl_list,DL_node* dl_node)
{	DL_init(dl_node);
	CHECK(dl_node);
#ifdef _DEBUG
	dl_list->count++;
#endif
	if(DL_is_empty(dl_list))
	{	dl_list->head=dl_list->tail=dl_node;
		return;
	}
	dl_node->prev=dl_list->tail;
	if(dl_list->tail)
	{	dl_list->tail->next=dl_node;
	}
	dl_list->tail=dl_node;
}

void DL_remove(DL_list* dl_list,DL_node* dl_node)
{	CHECK(dl_node);
#ifdef _DEBUG
	dl_list->count--;
	DL_VerifyNodeInList(dl_list,dl_node);
#endif
	/* If last node in list, empty list: */
	if(dl_list->tail==dl_list->head)
	{	dl_list->tail=dl_list->head=0;
		return;/* next and prev already 0 */
	}
	/* Check head and tail: */
	if(dl_list->head==dl_node)
	{	dl_list->head=dl_node->next;
	}
	if(dl_list->tail==dl_node)
	{	dl_list->tail=dl_node->prev;
	}
	/* Unlink node from chain: */
	if(dl_node->prev)
	{	dl_node->prev->next=dl_node->next;
	}
	if(dl_node->next)
	{	dl_node->next->prev=dl_node->prev;
	}
	/* Clear node pointers: */
	DL_init(dl_node);
}

void DL_VerifyNodeInList(DL_list* dl_list,DL_node* dl_node)
{	DL_node* next=dl_list->head;
	while(next)
	{	CHECK(dl_node);
		if(dl_node==next)
		{	return;
		}
		next=next->next;
	}
#ifdef WIN32
	__asm int 3;
#endif
}
