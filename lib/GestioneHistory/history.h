#include <stdlib.h>
#include "./../../struttureCondivise.h"
#include "./../../message.h"

typedef struct history{
    message_t ** list; // Inizializzati tutti a NULL
    int pos; // posizione corrente, ovvero quella in cui devo scrivere. Per leggere basta che parto da (pos + 1)  e leggo fino alla lunghezza dell'array. Quando arrivo alla fine ricomincio a leggere da 0 fino a (pos - 1). Devo leggere solo gli elementi null. Questo funziona perchè non ci sono mai delle pop. Le uniche operazioni sono la lettura e la scrittura, niente eliminazione
    pthread_mutex_t list_m;
} history_s;


history_s * initHistory();
int add(history_s * h, message_t * m);
void deleteHistory(void * r);
void freeMsg(message_t * elem);
