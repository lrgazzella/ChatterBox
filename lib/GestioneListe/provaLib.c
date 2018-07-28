#include "linklist.h"
#include <stdio.h>

linked_list_t * usr;

int main() {
    usr = list_create();
    char usr1[50] = "Lorenzo";
    char usr2[50] = "Gianni";
    char usr3[50] = "Paolo";
    char usr4[50] = "Vincenzo";
    char usr5[50] = "Federico";
    list_push_value(usr, usr1);
    list_push_value(usr, usr2);
    list_push_value(usr, usr3);
    list_push_value(usr, usr4);
    list_push_value(usr, usr5);

    char usr6[50] = "Giulio";
    list_insert_value(usr, usr6, 3);

    int i=0;
    for(i=0; i<list_count(usr); i++){
        printf("USR[%d]: %s\n", i, (char *)list_pick_value(usr, i));
    }

    list_destroy(usr);
    return 0;
}
