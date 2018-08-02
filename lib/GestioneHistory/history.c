#include "./history.h"


history_s * initHistory(){
    history_s * r = malloc(sizeof(history_s)); // TODO controllare errori
    r->list = malloc(sizeof(message_t) * configurazione.MaxHistMsgs);
    int i;
    for(i=0; i<configurazione.MaxHistMsgs; i++){
        r->list[i] = NULL;
    }
    r->pos = 0;
    pthread_mutex_init(&(r->list_m), NULL);
    return r;
}

int add(history_s * h, message_t * m) {
    if(h->list[h->pos] != NULL)
        freeMsg(h->list[h->pos]);
    h->list[h->pos] = m;
    if(h->pos + 1 == configurazione.MaxHistMsgs) h->pos = 0;
    else h->pos ++; 
}

void freeMsg(message_t *elem){
    free(elem->data.buf);
    free(elem);
    /* Posso fare la free dell'intero messaggio dato che, in caso di POSTTEXTALL_OP,
       io farei tante copie del messaggio e le aggiungerei ad ogni client.
       Altrimenti se facessi un'unica copia del messaggio, dovrei vedere, prima di fare la free, se
       ci sono altre history di altri utenti che hanno il riferimento attivo*/
}

void deleteHistory(void * r){
    history_s * h = (history_s *)r;
    int i;
    for(i=0; i<configurazione.MaxHistMsgs; i++){
        freeMsg(h->list[i]);
    }
    pthread_mutex_destroy(&(h->list_m));
    free(h);
}
