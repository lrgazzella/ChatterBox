#include <stdio.h>
#include "./codaCircolare.h"

typedef struct conta{
    int c;
} conta_s;

void freeConta_s(void * elem);

int main(){

    coda_circolare_s * c = initCodaCircolare(5, freeConta_s);

    conta_s * c1 = malloc(sizeof(conta_s));
    conta_s * c2 = malloc(sizeof(conta_s));
    conta_s * c3 = malloc(sizeof(conta_s));
    conta_s * c4 = malloc(sizeof(conta_s));
    conta_s * c5 = malloc(sizeof(conta_s));
    conta_s * c6 = malloc(sizeof(conta_s));
    conta_s * c7 = malloc(sizeof(conta_s));

    c1->c = 1;
    c2->c = 2;
    c3->c = 3;
    c4->c = 4;
    c5->c = 5;
    c6->c = 6;
    c7->c = 7;

    inserisci(c, c1);
    inserisci(c, c2);
    inserisci(c, c3);
    inserisci(c, c4);
    inserisci(c, c5);
    inserisci(c, c6);
    inserisci(c, c7);

    coda_circolare_iteratore_s * it = initIteratore(c);
    conta_s * c_tmp;
    while((c_tmp = (conta_s *)next(it)) != NULL){
        printf("ELEMENTO #%d\n", c_tmp->c);
    }

    eliminaIteratore(it);
    eliminaCoda(c);

}

void freeConta_s(void * elem){
    free(elem);
}
