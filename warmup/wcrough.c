// #include <assert.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include "common.h"
// #include "wc.h"
// #include <string.h>
// #include <ctype.h>

// typedef struct Item Item;

// struct Item {
// 	char *word; //key
// 	int freq; //value
// 	struct Item* next;
// };

// struct wc {
// 	/* you can define this struct to have whatever fields you want. */
// 	Item** table;
// 	int size;
// };

// struct Item* create(char *key){
// 	Item* item = (Item*) malloc(sizeof(Item));
// 	item->word = (char*) malloc(strlen(key)+1);
// 	strcpy(item->word, key);
// 	item->freq = 1; // initialize on creation
// 	item->next = NULL;
// 	return item;
// }

// int hash_function(char *word, int size){
// 	int hash = 7;
// 	for (int i = 0; i < strlen(word); i++) {
//     	hash = hash + word[i]; //hash*31 + word[i]
// 		// hash %= size;
// 	}
//     return hash % size;
// }

// void update_table(struct wc *wc, char* word, int key){
// 	// printf("DEALING WITH %s\n", word);
// 	Item* current_item = wc->table[key]; 
// 	if (current_item==NULL){
// 		// printf("Newly adding %s without collisions\n", word);
// 		wc->table[key]=create(word);
// 		return;
// 	}
// 	while(current_item->next != NULL){
// 		// printf("Collision, currently at %s\n", current_item->word);
// 		if (strcmp(current_item->word, word)==0){
// 			// printf("Found %s already in LL\n", word);
// 			current_item->freq+=1;
// 			return;
// 		}
// 		current_item = current_item->next;
// 	}
// 	if (strcmp(current_item->word, word)==0){
// 			// printf("Found %s already in LL\n", word);
// 			current_item->freq+=1;
// 			return;
// 	}
// 	current_item->next = create(word);
// 	// printf("Chained %s\n", word);
// }

// struct wc *
// wc_init(char *word_array, long size)
// {
// 	struct wc *wc;
 
// 	wc = (struct wc *)malloc(sizeof(struct wc));
// 	assert(wc);

// 	int word_count = 1;
// 	for (int i=0; i<size; i++){
// 		if (word_array[i]==' ' || word_array[i]=='\n')
// 			word_count++;
// 	}

// 	wc->size = (int)(word_count*1.5);
// 	wc->table = (Item**) malloc(wc->size*sizeof(Item*));

// 	for (int i = 0; i < wc->size; i++){
// 		wc->table[i] = NULL;
// 	}

// 	int ctr = 0, consecutive=0;
//     int key;
//     int startidx = 0;
//     for (int i = 0; i < size; i++){
//         if (isspace(word_array[i]) && consecutive==1){
//             char *word = malloc((ctr+1)*sizeof(char));
//             memcpy(word, word_array+startidx, ctr);
//             key = hash_function(word, wc->size);
//             update_table(wc, word, key);
//             ctr = 0;
//             startidx = i+1;
// 			consecutive = 0;
//         }
// 		else if (isspace(word_array[i])){
// 			startidx = i+1;
// 		}
//         else{
//             ctr++;
// 			consecutive = 1;
// 		}
//     }
// 	if (consecutive==1){
// 		ctr++;
// 		char *word = malloc((ctr+1)*sizeof(char));
// 		memcpy(word, word_array+startidx, ctr);
// 		// printf("Updated:%s\n", word);
// 		key = hash_function(word, wc->size);
// 		update_table(wc, word, key);
// 	}

// 	return wc;
// }

// void
// wc_output(struct wc *wc)
// {
// 	// int count = 0;
// 	for (int i = 0; i < wc->size; i++){
// 		Item* tmp = wc->table[i];
// 		while (tmp != NULL){
// 			// if (count==0)
// 			// 	count=1;
// 			// else
// 			//  	printf("\n");
// 			printf("%s:", tmp->word);
// 			printf("%d\n", tmp->freq);	
// 			tmp = tmp->next;
// 		}
// 	}
// }

// void item_destroy(Item* item){
// 	free(item->word);
// 	free(item);
// }

// void
// wc_destroy(struct wc *wc)
// {
// 	for (int i = 0; i < wc->size; i++){
// 		if (wc->table[i]!=NULL){
// 			item_destroy(wc->table[i]);
// 		}
// 	}
// 	free(wc->table);
// 	free(wc);
// }