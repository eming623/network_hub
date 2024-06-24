/*
 * srv_list.h
 *
 *  Created on: Aug 16, 2014
 *      Author: Yiming Xu
 *
 *  A helper utility provides double-linked list API.
 */

#ifndef SRV_LIST_H_
#define SRV_LIST_H_

#include <stdio.h>
#include "srv_common.h"

typedef struct srv_list_node_s srv_list_node_s;
struct srv_list_node_s
{
	struct srv_list_node_s *next;
	struct srv_list_node_s *prev;
	void *myself;
};

#define SRV_LIST_INIT_ROOT(R) (R)->myself = NULL; \
                              (R)->next = (R); \
                              (R)->prev = (R)

#define SRV_LIST_INIT_NODE(E, S) (E)->myself = (S); \
                                 (E)->next = NULL; \
                                 (E)->prev = NULL

#define SRV_LIST_INSERT_AFTER(P, E) (E)->next = (P)->next; \
	                                (E)->prev = (P); \
	                                (E)->prev->next = (E); \
	                                (E)->next->prev = (E)

#define SRV_LIST_INSERT_BEFORE(N, E) (E)->next = (N); \
	                                 (E)->prev = (N)->prev; \
	                                 (E)->prev->next = (E); \
	                                 (E)->next->prev = (E)

#define SRV_LIST_REMOVE(E) (E)->prev->next = (E)->next; \
	                       (E)->next->prev = (E)->prev; \
	                       (E)->prev = NULL; \
	                       (E)->next = NULL

#define SRV_LIST_GET_NEXT(E) ((E)->next)
#define SRV_LIST_GET_PREV(E) ((E)->prev)

#define SRV_LIST_GET_ENTRY(E) ((E)->myself)

#define SRV_LIST_IS_EMPTY(R) ((R)->next == (R))

#define SRV_LIST_FOR_EACH(E, R) for ((E)=(R)->next; (E)!=(R); (E)=(E)->next)

#define SRV_LIST_FOR_EACH_ENTRY(E, R, S) for ((E)=(R)->next, (S)=(E)->myself; (E)!=(R); (E)=(E)->next)

#endif /* SRV_LIST_H_ */
