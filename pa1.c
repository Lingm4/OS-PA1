/**********************************************************************
 * Copyright (c) 2020-2023
 *  Sang-Hoon Kim <sanghoonkim@ajou.ac.kr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTIABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 **********************************************************************/

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include "types.h"
#include "list_head.h"

/***********************************************************************
 * list struct
 *
 * add_list_entry()
 * remove_list_entry()
 * RETURN VALUE
 *   Return 1 on successful command execution
 *   Return 0 when user inputs "exit"
 *   Return <0 on error
 */

LIST_HEAD(alias_list);

struct alias_list_entry{
	int nr_origin;
	char **origin;
	char *replace;
	struct list_head list;
};

void add_list_entry(char **const org, const int nr_org, char * const rep)
{
	struct alias_list_entry *new_entry = (struct alias_list_entry *)malloc(sizeof(struct alias_list_entry));
	
	new_entry->nr_origin = nr_org;
	
	new_entry->replace = (char *)malloc(sizeof(char)*strlen(rep) +1); // +1 for '\0'
	strcpy(new_entry->replace, rep);

	new_entry->origin = (char **)malloc(sizeof(char*)*nr_org);
	for(int i = 0; i < nr_org; i++){
		(new_entry->origin)[i] = (char *)malloc(sizeof(char)*strlen(org[i]) +1); // +1 for '\0'
		strcpy((new_entry->origin)[i], org[i]);
	}
	list_add(&(new_entry->list), &alias_list);
}

void dump_alias_list(void)
{
	struct list_head *pos;
	list_for_each_prev(pos, &alias_list){
		fprintf(stderr, "%s:", list_entry(pos, struct alias_list_entry, list)->replace);
		for(int i=0; i<list_entry(pos, struct alias_list_entry, list)->nr_origin; i++){
			fprintf(stderr, " %s", (list_entry(pos, struct alias_list_entry, list)->origin)[i]);
		}
		fprintf(stderr, "\n");
	}
}

struct alias_list_entry *find_alias_list_entry(const char *const target){
	//printf("finding %s...\n", target); //지워야함
	struct list_head *pos;
	list_for_each_prev(pos, &alias_list){
		if(strcmp(target, list_entry(pos, struct alias_list_entry, list)->replace)==0) return list_entry(pos, struct alias_list_entry, list);
	}
	return NULL;
}

void make_tokens_replacing_alias(char **tokens, int *nr_tokens){
	for(int i=0; i<*nr_tokens; i++){
		struct alias_list_entry *target_alias_entry = find_alias_list_entry(tokens[i]);
		if(target_alias_entry != NULL){
			//printf("replacing...\n");
			for(int j=0; j<target_alias_entry->nr_origin-1; j++){
				for(int k=0; k<*nr_tokens-i; k++){
					//printf("push tokens...\n");
					free(tokens[*nr_tokens-k]);
					tokens[*nr_tokens-k] = strdup(tokens[*nr_tokens-k-1]);
				}
				(*nr_tokens)++;
			}
			for(int j=0; j<target_alias_entry->nr_origin; j++){
				//printf("push origin...\n");
				free(tokens[i+j]);
				tokens[i+j] = strdup(target_alias_entry->origin[j]);
			}
			i+=target_alias_entry->nr_origin-1;
		}
	}
}

/***********************************************************************
 * run_command()
 *
 * DESCRIPTION
 *   Implement the specified shell features here using the parsed
 *   command tokens.
 *
 * RETURN VALUE
 *   Return 1 on successful command execution
 *   Return 0 when user inputs "exit"
 *   Return <0 on error
 */

int run_command(int nr_tokens, char *tokens[])
{
	if (strcmp(tokens[0], "exit") == 0) return 0;
	else if(strcmp(tokens[0], "cd") == 0){
		if(nr_tokens == 1 || strcmp(tokens[1], "~") == 0){
			chdir(getenv("HOME"));
			return 1;
		}else{
			chdir(tokens[1]);
			return 1;
		}
	}else if(strcmp(tokens[0], "alias") == 0){
		if(nr_tokens == 1) dump_alias_list();
		else add_list_entry(&tokens[2], nr_tokens-2, tokens[1]);
		return 1;
	}else{
		make_tokens_replacing_alias(tokens, &nr_tokens);
		/*
		for(int i=0; i<nr_tokens; i++){
			printf("%s ", tokens[i]);
		}printf("\n");
		*/
		int pid = fork();
		if(pid == 0){
			execvp(tokens[0], &tokens[0]);
			fprintf(stderr, "Unable to execute %s\n", tokens[0]);
			exit(1);
		}else if(pid == -1){
			fprintf(stderr, "Unable to execute %s\n", tokens[0]);
			return -1;
		}else{
			int status;
			waitpid(pid, &status, 0);
			return 1;
		}
	}
}


/***********************************************************************
 * initialize()
 *
 * DESCRIPTION
 *   Call-back function for your own initialization code. It is OK to
 *   leave blank if you don't need any initialization.
 *
 * RETURN VALUE
 *   Return 0 on successful initialization.
 *   Return other value on error, which leads the program to exit.
 */
int initialize(int argc, char * const argv[])
{
	return 0;
}


/***********************************************************************
 * finalize()
 *
 * DESCRIPTION
 *   Callback function for finalizing your code. Like @initialize(),
 *   you may leave this function blank.
 */
void finalize(int argc, char * const argv[])
{
}
