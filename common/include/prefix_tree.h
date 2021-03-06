/*!
 * \file prefix_tree.c
 * \brief Prefix tree data structure for saving informaticons about domains.
 * \author Zdenek Rosa <rosazden@fit.cvut.cz>
 * \date 2014
 */
/*
 * Copyright (C) 2014 CESNET
 *
 * LICENSE TERMS
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of the Company nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * ALTERNATIVELY, provided that this notice is retained in full, this
 * product may be distributed under the terms of the GNU General Public
 * License (GPL) version 2 or later, in which case the provisions
 * of the GPL apply INSTEAD OF those given above.
 *
 * This software is provided ``as is'', and any express or implied
 * warranties, including, but not limited to, the implied warranties of
 * merchantability and fitness for a particular purpose are disclaimed.
 * In no event shall the company or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage.
 *
 */
#ifndef _PREFIX_TREE_
#define _PREFIX_TREE_



#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <time.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
//#include "tunnel_detection_dns_structs.h"



/*!
 * \name Default values
 *  Defines macros used by prefix tree
 * \{ */
#define COUNT_OF_LETTERS_IN_DOMAIN 256 /*< Max count of letter in domain. */
#define MAX_SIZE_OF_DOMAIN 256 /*< Max length of domain */
#define MAX_SIZE_OF_DEGREE 5 /*< Max size of stored information  */
#define ADD_TO_LIST_FROM_COUNT_OF_SEARCH 20 /*< Default number of histogram size. */
#define ADD_TO_LIST_FROM_COUNT_OF_DIFFERENT_SUBDOMAINS 10 /*< Default number of histogram size. */
#define MAX_COUNT_TO_BE_IN_JUST_ONE_SEARCHER 10 /*< Default number of histogram size. */
#define PREFIX  1
#define SUFFIX   0
/* /} */

/**
 * Inicialization parametters for allowing domain extension information.
 */
typedef enum {
	DOMAIN_EXTENSION_YES,
	DOMAIN_EXTENSION_NO
} domain_extension;

/**
 * Inicialization parametters for relaxation tree after deleting node.
 */
typedef enum {
    RELAXATION_AFTER_DELETE_YES,
    RELAXATION_AFTER_DELETE_NO
} relaxation_after_delete;

typedef struct prefix_tree_domain_t prefix_tree_domain_t ;
typedef struct prefix_tree_inner_node_t prefix_tree_inner_node_t;

/*!
 * \brief Structure - Prefix tree - inner node structure
 * Structure used to keep information about inner node of prefix tree.
 */
struct prefix_tree_inner_node_t{
	unsigned char length;  /*< length of stored string */
	unsigned int count_of_string; /*< count of different substrings */
	unsigned char count_of_children;
	char * string; /*< stored string in reverse way (end of string is on postion 0)*/
	prefix_tree_inner_node_t * parent; /*< Pointer to parent */
	prefix_tree_domain_t * parent_is_domain;   /*< Pointer to parent, if NULL parent is inner node, else parent is domain name node*/
	prefix_tree_inner_node_t ** child;     /*< Pointer to descendants*/
	prefix_tree_domain_t * domain;   /*< if this string is domain, this variable conatin poiter to domain name structure*/
	void  * value;	/*< pointer to user value */
};

/*!
 * \brief Structure - domain extension
 * Structure keeps pointers in linked list to other domain nodes.
 */
typedef  struct node_domain_extension_t {
	prefix_tree_domain_t * most_used_domain_less;  /*< position in linked list, pointer to next domain, which has lower count of seatching*/
	prefix_tree_domain_t * most_used_domain_more;  /*< position in linked list, pointer to previous domain, which has higher count of seatching*/
	prefix_tree_domain_t * most_subdomains_less;   /*< position in linked list, pointer to next domain, which has lower count of descendants*/
	prefix_tree_domain_t * most_subdomains_more;   /*< position in linked list, pointer to previous domain, which has higher count of descendants*/
} node_domain_extension_t;

/*!
 * \brief Structure - Prefix tree - domain name structure
 * Structure used to keep information about domain names. It is domain name structure, which contain information about domain name.
 */
struct prefix_tree_domain_t {
 	unsigned char exception;  /*< 1 for exception in detection, 0 for classic domain */
    unsigned char degree;  /*< degree of domain */
	unsigned int count_of_insert;  /*< count of searching this domain name */
	unsigned int count_of_different_subdomains;    /*< count of descendants - subdomains */
	prefix_tree_inner_node_t * parent; /*< pointer to parent (last character of domain) */
	prefix_tree_domain_t * parent_domain;  /*< pointer to parent (domain name) */
	prefix_tree_inner_node_t *child;   /*< pointer to descendats */
	void  * value;	/*< pointer to user value - specified by init function */
	node_domain_extension_t * domain_extension; /*< pointer to structure with linked list of domains*/
};

/*!
 * \brief Structure - Domain names extension
 * Structure used to keep lists of most searched domains, subdomains and least searched subdomains
 * information about prexix tree.
 */
typedef struct tree_domain_extension_t {
		prefix_tree_domain_t * list_of_most_used_domains;  /*< list of most searched domains*/
		prefix_tree_domain_t * list_of_most_used_domains_end;  /*< list of most searched domains - pointer to end of the list*/
		prefix_tree_domain_t * list_of_most_unused_domains;    /*< list of most searched domains end - most unsearched domains*/
		prefix_tree_domain_t ** list_of_most_subdomains;   /*< list of domains, which has most subdomains, it is 2D linked list. 1.D is degree of domain name, 2. is linked list*/
		prefix_tree_domain_t ** list_of_most_subdomains_end;   /*< list of domains, which has most subdomains, it is 2D linked list.  Pointers on the end of lists*/
} tree_domain_extension_t;

/*!
 * \brief Structure - Prefix tree main structure
 * Structure used to keep information about prefix tree. It is main structure, witch contains all
 * information about prexix tree.
 */
typedef struct prefix_tree_t {
	prefix_tree_inner_node_t * root;   /*< Pointer to root node. */
	unsigned int size_of_value;	/*< size of value stored for every domain node */
	int domain_separator;	/*< separator in text, which creares domain node */
	unsigned char prefix_suffix;  /*< prefix or suffix tree */
	unsigned int count_of_domain_searched_just_ones;   /*< Count of domain, witch were searcehd just ones */
	unsigned int count_of_inserting;   /*< Count of inserting and searching (all domains) */
    unsigned int count_of_inserting_for_just_ones; /*< Count of inserting, but for percent value with variable count_of_domain_searched_just_ones*/
	unsigned int count_of_different_domains; /*< Count of unique domains*/
	tree_domain_extension_t * domain_extension; /*< Lists of most used domains etc.*/
	relaxation_after_delete relaxation;
} prefix_tree_t;


/*!
 * \brief Map character to index
 * Function maps character to index in descendants.
 * \param[in] letter character
 * \return letter mapped index.
 */
int prefix_tree_map_character_to_number(unsigned char  letter);

/*!
 * \brief Init function for prefix tree
 * Function that incialize prefix tree.
 * \param[in] PREFIX for prefix tree or SUFFIX for suffix tree
 * \param[in] size of space for value
 * \param[in] character of domain separator, when this value is in string, special domain node for value is created
 * \param[in] domain_extension can has value of DOMAIN_EXTENSION_YES or DOMAIN_EXTENSION_NO. If is it set on YES it will store addintional structure for searching domains via their usage.
 * \return pointer to prefix tree structure
 */
prefix_tree_t * prefix_tree_initialize(unsigned char prefix_suffix, unsigned int size_of_value, int domain_separator, int domain_extension, relaxation_after_delete relaxation);
/*!
 * \brief Decrease counters in prefix tree, if deleting a node.
 * Function that goes from the node to the root and decreases counters.
 * \param[in] node from which to start decreasing.
 * \param[in] deleted_strings count of deleted strings.
 * \param[in] deleted_domains count of deleted domain nodes.
 */
void prefix_tree_decrease_counters_deleted_inner_node(prefix_tree_inner_node_t * node, int deleted_strings, int deleted_domains);

/*!
 * \brief Function joins two nodes into one.
 * If parent has just one child, this function join them into one node.
 * \param[in] node parent node, which would be joined with son.
 * \return joined inner node
 */
prefix_tree_inner_node_t * join_nodes(prefix_tree_inner_node_t * node);

/*!
 * \brief Function which removes inner node and his descendats.
 * Function will erase specified node and all his decendats.
 * Counters will be changed.
 * \param[in] tree pointer to tree in which is the deleted node.
 * \param[in] node parent node, which would be joined with son.
 */
void prefix_tree_delete_inner_node(prefix_tree_t * tree, prefix_tree_inner_node_t * node);

/*!
 * \brief Destroy all items in prefix tree
 * Function destroy recursively destroies all nodes.
 * \param[in] tree pointer to prefix tree
 * \param[in] node pointer to inner node, which will be destroied
 * \return count of deleted domains
 */
int prefix_tree_destroy_recursive(prefix_tree_t * tree, prefix_tree_inner_node_t *  node);

/*!
 * \brief Destroy function for prefix tree
 * Function destroy prefix tree and all item inside
 * \param[in] tree pointer to prefix tree
 */
void prefix_tree_destroy(prefix_tree_t * tree);

/*!
 * \brief Recursive change info about parent doimains
 * Function actualize information in parent domains.
 * \param[in] domain_parent domain where to actualize inforamtion
 * \param[in] tree pointer to prefix tree
 */
void prefix_tree_recursive_plus_domain(prefix_tree_domain_t * domain_parent, prefix_tree_t * tree);

/*!
 * \brief Create domain node structure
 * Function Create domian and connects it to the tree.
 * \param[in] node parent node (contain last letter of domain)
 * \param[in] domain_parent  pointer to parent domain
 * \param[in] tree pointer to prefix tree
 * \return pointer to domain structure
 */
prefix_tree_domain_t * prefix_tree_new_domain(prefix_tree_inner_node_t * node, prefix_tree_domain_t * domain_parent, prefix_tree_t * tree);

/*!
 * \brief Create inner node structure
 * Function Create inned node and connects it to the tree.
 * \param[in] parent parent node
 * \param[in] map_number  number of first character of new node (index on this node in parent)
 * \return pointer to inner node structure
 */
prefix_tree_inner_node_t * prefix_tree_new_node(prefix_tree_inner_node_t * parent, int map_number);

/*!
 * \brief Alloc memory for descendats in inner node
 * Function allocs memory for descendats in inner node.
 * \param[in] parent parent node
 * \return pointer to inner node structure, which was given in parametter
 */
prefix_tree_inner_node_t * prefix_tree_add_children_array(prefix_tree_inner_node_t * parent);

/*!
 * \brief Create descendant of domain
 * Function creates descendant of domain, (domain has other subdomains).
 * \param[in] domain domain where to add descendant
 * \return pointer to descendant inner node
 */
prefix_tree_inner_node_t * prefix_tree_new_node_parent_is_domain(prefix_tree_domain_t * domain);

/*!
 * \brief Count length of string to dot
 * Function counts length of string to dot.
 * \param[in] string
 * \param[in] length of string
 * \param[in] separator of domain
 * \param[in] prefix or suffix
 * \return length to dot
 */
int prefix_tree_count_to_domain_separator(const char *string, int length, int domain_separator, char prefix);

/*!
 * \brief Add new item to prefix tree
 * Function add new item to prefix tree (place where to add new domain has to be given).
 * \param[in] node node where to add new item
 * \param[in] domain nearest parent domain
 * \param[in] string string with new item
 * \param[in] length length of string
 * \param[in] tree pointer to the prefix tree
 * \return pointer to new domain structure
 */
prefix_tree_domain_t * prefix_tree_add_new_item(prefix_tree_inner_node_t * node ,prefix_tree_domain_t * domain ,const char *string, int length, prefix_tree_t * tree);


/*!
 * \brief Split node into two nodes
 * Function splits node into two nodes, on the given position.
 * This function is needed when inserting new node, which has coomon part of string with some node.
 * \param[in] node node which will be splitted
 * \param[in] index position in string (where to split the node)
 * \return pointer to first node. Seccond splitted node is descendant of first node.
 */
prefix_tree_inner_node_t * prefix_tree_split_node_into_two(prefix_tree_inner_node_t * node, int index);

/*!
 * \brief Read domain from tree
 * Function return string with the domain name.
 * \param[in] pointer to the tree
 * \param[in] domain pointer to domain, which should be returned in string
 * \param[in] string pointer on memory where to store string
 * \return pointer to string
 */
char * prefix_tree_read_string(prefix_tree_t * tree, prefix_tree_domain_t * domain, char * string);



/*!
 * \brief Read string from inner node
 * Function return string stored in given inner node
 * \param[in] pointer to the tree
 * \param[in] node pointer to node
 * \param[in] string pointer on memory where to store string
 * \return pointer to string
 */
char * prefix_tree_read_inner_node(prefix_tree_t * tree, prefix_tree_inner_node_t * node, char * string);

/*!
 * \brief Add domain recursive (prefix tree)
 * Function adds domain to the prefix tree. This function is called from  prefix_tree_add_domain and needs
 * more parametters.
 * \param[in] node inner node where to insert of find domain
 * \param[in] domain_parent neares domain parent
 * \param[in] string string to add to prefix tree
 * \param[in] length length of string
 * \param[in] tree pointer to the prefix tree
 * \return added or found domain
 */
prefix_tree_domain_t * prefix_tree_add_domain_recursive_prefix(prefix_tree_inner_node_t * node, prefix_tree_domain_t * domain_parent, const char *string, int length, prefix_tree_t * tree);


/*!
 * \brief Add domain recursive (suffix tree)
 * Function adds domain to the prefix tree. This function is called from  prefix_tree_add_domain and needs
 * more parametters.
 * \param[in] node inner node where to insert of find domain
 * \param[in] domain_parent neares domain parent
 * \param[in] string string to add to prefix tree
 * \param[in] length length of string
 * \param[in] tree pointer to the prefix tree
 * \return added or found domain
 */
prefix_tree_domain_t * prefix_tree_add_domain_recursive_suffix(prefix_tree_inner_node_t * node, prefix_tree_domain_t * domain_parent,const char *string, int length, prefix_tree_t * tree);

/*!
 * \brief Add domain to prefix tree
 * Function adds domain to the prefix tree.
 * \param[in] tree pointer to the prefix tree
 * \param[in] string string witch should be added
 * \param[in] length length of string
 * \return added or found domain
 */
prefix_tree_domain_t * prefix_tree_insert(prefix_tree_t * tree, const char *string, int length);

/*!
 * \brief Seacrh domain in prefix tree
 * Function adds domain to the prefix tree.
 * \param[in] tree pointer to the prefix tree
 * \param[in] string string witch should be added
 * \param[in] length length of string
 * \return added or found domain
 */
prefix_tree_domain_t * prefix_tree_search(prefix_tree_t * tree, const char *string, int length);

/*!
 * \brief Add domain to prefix tree and set it to the exception state
 * Function adds domain to the prefix tree  and set it to the exception state
 * \param[in] tree pointer to the prefix tree
 * \param[in] string string witch should be added
 * \param[in] length length of string
 * \return added or found domain
 */
prefix_tree_domain_t * prefix_tree_add_string_exception(prefix_tree_t * tree, const char *string, int length);

/*!
 * \brief Test domain if is in exception state
 * Function tests domain if is in exception state.
 * \param[in] tree pointer to the prefix tree
 * \param[in] string string witch should be added
 * \param[in] length length of string
 * \return 1 is in exception, 0 not in exception
 */
int prefix_tree_is_string_in_exception(prefix_tree_t * tree, const char *string, int length);

/*!
 * \brief Statistic function percent od subdomains in certain depth
 * Function returns percent of subdomains in most searched domain in given depth.
 * \param[in] tree pointer to the prefix tree
 * \param[in] depth
 * \return added or found domain
 */
double prefix_tree_most_used_domain_percent_of_subdomains(prefix_tree_t * tree, int depth);


/*!
 * \brief Returns inner node with most of different strings
 * Function returns pointer to inner node, which has the most of different strings.
 * \param[in] node pointer to the node, from which the searching starts.
 * \return NULL if the node does not exist(does not have inner node child) or pointer to inner node.
 */
prefix_tree_inner_node_t * prefix_tree_most_substring(prefix_tree_inner_node_t * node);






 #endif /* _PREFIX_TREE_ */
