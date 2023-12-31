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
#include <sys/wait.h>
#include <stdlib.h>

#include "types.h"
#include "list_head.h"

#define READ 0
#define WRITE 1

/***********************************************************************
 * alias_list
 *
 * add_alias_list_entry()
 * dump_alias_list()
 * find_alias_list_entry
 */

LIST_HEAD(alias_list);

struct alias_list_entry{
	int nr_origin;
	char **origin;
	char *replace;
	struct list_head list;
};

void add_alias_list_entry(char **const org, const int nr_org, char * const rep)
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
	struct list_head *pos;
	list_for_each_prev(pos, &alias_list){
		if(strcmp(target, list_entry(pos, struct alias_list_entry, list)->replace)==0) return list_entry(pos, struct alias_list_entry, list);
	}
	return NULL;
}

/***********************************************************************
 * Useful fuctions
 */

void make_tokens_replacing_alias(char **tokens, int *nr_tokens){
	for(int i=0; i<*nr_tokens; i++){
		struct alias_list_entry *target_alias_entry = find_alias_list_entry(tokens[i]);
		if(target_alias_entry != NULL){
			for(int j=0; j<target_alias_entry->nr_origin-1; j++){
				for(int k=0; k<*nr_tokens-i; k++){
					free(tokens[*nr_tokens-k]);
					tokens[*nr_tokens-k] = strdup(tokens[*nr_tokens-k-1]);
				}
				(*nr_tokens)++;
			}
			for(int j=0; j<target_alias_entry->nr_origin; j++){
				free(tokens[i+j]);
				tokens[i+j] = strdup(target_alias_entry->origin[j]);
			}
			i+=target_alias_entry->nr_origin-1;
		}
	}
}

int find_token(char **tokens, char *target){
	for(int i=0; tokens[i]!=NULL; i++){
		if(strcmp(target, tokens[i])==0) return i;
	}
	return -1; //when tokens don't have target
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
	if (tokens[0] == NULL) return -1;
	else if(strcmp(tokens[0], "exit") == 0) return 0;
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
		else add_alias_list_entry(&tokens[2], nr_tokens-2, tokens[1]);
		return 1;
	}else{
		make_tokens_replacing_alias(tokens, &nr_tokens);
		int pip = find_token(tokens, "|");
		
		if(pip!=-1){
			free(tokens[pip]);
			tokens[pip] = NULL;
			int fd[2];
			if(pipe(fd) < 0){
                fprintf(stderr,"pipe error\n");
                return -1;
        	}
			int pid1 = fork();
			if(pid1 == 0){
				close(fd[READ]);
				dup2(fd[WRITE], 1);
				execvp(tokens[0], &tokens[0]);
				fprintf(stderr, "Unable to execute %s\n", tokens[0]);
				exit(1);
				return -1;
			}else if(pid1 == -1){
				fprintf(stderr, "Unable to execute %s\n", tokens[0]);
				return -1;
			}

			int pid2 = fork();
			if(pid2 == 0){
				close(fd[WRITE]);
				dup2(fd[READ], 0);					
				execvp(tokens[pip+1], &tokens[pip+1]);
				fprintf(stderr, "Unable to execute %s\n", tokens[pip+1]);
				exit(1);
				return -1;
			}else if(pid2 == -1){
				fprintf(stderr, "Unable to execute %s\n", tokens[pip+1]);
				return -1;
			}
			close(fd[READ]);
			close(fd[WRITE]);
			int status;
			waitpid(pid1, &status, 0);	
			waitpid(pid2, &status, 0);
			return 1;
		}else{
			int pid = fork();
			if(pid == 0){
				execvp(tokens[0], &tokens[0]);
				fprintf(stderr, "Unable to execute %s\n", tokens[0]);
				exit(1);
				return -1;
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
