#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "wc.h"
#include <string.h>
#include <ctype.h>

typedef struct Item Item;

struct Item {
	char *word; //key
	int freq; //value
};

struct wc {
	/* you can define this struct to have whatever fields you want. */
	int size;
	Item** table;
};

struct Item* create(char *key){
	Item* item = (Item*) malloc(sizeof(Item));
	item->word = (char*) malloc(strlen(key)+1);
	strcpy(item->word, key);
	item->freq = 1; // initialize on creation
	return item;
}

unsigned long hash_function(char *word, int size){
	unsigned long hash = 7;
	for (int i = 0, k = strlen(word); i < k; i++) {
		hash = (hash*31 + word[i]);
	}
    return hash % size;
}

void update_table(struct wc *wc, char* word, int key){
	int loc = key;
	int search = 0;
	Item* current_item = wc->table[loc]; 
	int c = 1;
	while(current_item != NULL && search<wc->size){
		if (strcmp(current_item->word, word)==0){
			current_item->freq+=1;
			return;
		}
		loc = (loc + c) % wc->size;
		current_item = wc->table[loc];
		search+=1;
	}
	wc->table[loc] = create(word);
}

struct wc *
wc_init(char *word_array, long size)
{
	struct wc *wc;
 
	wc = (struct wc *)malloc(sizeof(struct wc));
	assert(wc);

	int word_count = 1;
	for (int i=0; i<size; i++){
		if (word_array[i]==' ' || word_array[i]=='\n')
			word_count++;
	}

	wc->size = word_count*5;
	wc->table = (Item**) malloc(wc->size*sizeof(Item*));

	for (long i = 0; i < wc->size; i++){
		wc->table[i] = NULL;
	}

	int ctr = 0, consecutive=0;
    int key;
    int startidx = 0;
    for (int i = 0; i < size; i++){
        if (isspace(word_array[i]) && consecutive==1){ // end of word
            char *word = malloc((ctr+1)*sizeof(char));
            memcpy(word, word_array+startidx, ctr);
			word[ctr] = '\0';
            key = hash_function(word, wc->size);
            update_table(wc, word, key);
            ctr = 0;
            startidx = i+1;
			consecutive = 0;
        }
		else if (isspace(word_array[i])){ // space after space, just keep incrementing start index
			startidx = i+1;
		}
        else{	// normal character(not space)
            ctr++;
			consecutive = 1;
		}
    }
	if (consecutive==1){
		char *word = malloc((ctr+1)*sizeof(char));
		memcpy(word, word_array+startidx, ctr);
		word[ctr] = '\0';
		key = hash_function(word, wc->size);
		update_table(wc, word, key);
	}

	return wc;
}

void
wc_output(struct wc *wc)
{
	int count = 0;
	for (int i = 0; i < wc->size; i++){
		Item* tmp = wc->table[i];
		if (tmp != NULL){
			if (count==0)
				count=1;
			else
			 	printf("\n");
			printf("%s:", tmp->word);
			printf("%d", tmp->freq);	
		}
	}
}

void item_destroy(Item* item){
	free(item->word);
	free(item);
}

void
wc_destroy(struct wc *wc)
{
	for (int i = 0; i < wc->size; i++){
		if (wc->table[i]!=NULL){
			item_destroy(wc->table[i]);
		}
	}
	free(wc->table);
	free(wc);
}