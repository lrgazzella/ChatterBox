int register_op(message_t m, icl_hash_t * registrati, pthread_mutex_t * lockRegistrati,  linked_list_t * connessi){
    /*
        controllo se esiste un utente con quel nickname
        se non esiste creo un nuovo utente (struct e quindi devo allocare un array circolare)
        Invio la lista degli utenti
   */
   if(icl_hash_find(registrati, (void *)m.hdr.sender) == NULL){

   }

}
