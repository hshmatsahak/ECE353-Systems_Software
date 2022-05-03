#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

int hash_function(char *word, int size){
	int hash = 7;
	for (int i = 0; i < strlen(word); i++) {
        printf("%d\n",word[i]);
    	// hash = hash*31 + int(word[i]);
        // printf("%d", hash);
	}
    return hash % size;
}

int main(){
    if (isspace('\n'))
        printf("hughf");
    // char *s = "I eat chole eat";
    // int ctr = 0;
    // int key;
    // int startidx = 0;
    // for (int i = 0; i < size; i++){
    //     if (isspace(s[i])){
    //         char *word = malloc((ctr)*sizeof(char));
    //         memcpy(word, s+startidx, ctr);
    //         key = hash_function(dest);
    //         update_table(wc, key);
    //         ctr = 0;
    //         startidx = i+1;
    //     }
    //     else
    //         ctr++;
    // }
    // ctr++;
    // char *dest = malloc((ctr)*sizeof(char));
    // memcpy(dest, s+startidx, ctr);
    // key = hash_function(dest);
    // update_table(wc, key);

    // int j = 0;
    // char *word;
    // for (int i = 0; i<strlen(s); i++){
    //     if (isspace(s[i]) || i==strlen(s)-1){
    //         if (j==0){
    //             continue;
    //         }
    //         else{
    //             word = (char *)malloc(sizeof(char)*j+1);
    //             memcpy(word, &(s[i-j]), j);
    //             word[j]=0;
    //             printf("%s\n", word);
    //             j=0;
    //         }
    //     }
    //     else{
    //         j++;
    //     }
    // }
    // printf("%d\n", hash_function(s, 10));
}