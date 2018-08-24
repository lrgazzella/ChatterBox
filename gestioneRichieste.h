/** @file gestioneRichieste.h
  * @author Lorenzo Gazzella 546890
  * @brief Contiene la dichiarazione di tutti i metodi per la gestione delle singole operazioni che il server gestisce.
  * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
  * originale dell'autore
  */
#ifndef GESTIONE_RICHIESTE_
#define GESTIONE_RICHIESTE_

#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <strings.h>
#include "message.h"
#include "ops.h"
#include "./config.h"
#include "./struttureCondivise.h"
#include "./gestioneRichieste.h"
#include "./chatty.h"
#include "./parser.h"
#include "./message.h"
#include "./connections.h"
#include "./struttureCondivise.h"
#include "./lib/GestioneHistory/codaCircolare.h"


/**
  * @function register_op
  * @brief Funzione che gestisce la richiesta di registrazione di un client. Il client verrà automaticamente connesso e riceverà la lista degli utenti online
  *
  * @param fd Fd del client che sta facendo la richiesta
  * @param m Messaggio inviato dal client
  *
  * @return 0 in caso di successo
  *         -1 in caso di errore
  */
int register_op(long fd, message_t m);

/**
  * @function connect_op
  * @brief Funzione che gestisce la richiesta di connessione di un client. Il client riceverà automaticamente la lista degli utenti online
  *
  * @param fd Fd del client che sta facendo la richiesta
  * @param m Messaggio inviato dal client
  * @param atomica Variabile booleana. Deve valere 1 se si invoca con la lock sulla hash degli utenti registrati, 0 altirmenti.
  *
  * @return 0 in caso di successo
  *         -1 in caso di errore
  */
int connect_op(long fd, message_t m, int atomica);

/**
  * @function posttxt_op
  * @brief Funzione che gestisce la richiesta di invio di un messaggio da parte di un client a un altro.
  *        Il messaggio sarà inviato direttamente al client destinatario nel caso in cui sia connesso.
  *        In ogni caso il messaggio verrà aggiunto alla history del client destinatario.
  *
  * @param fd Fd del client che sta facendo la richiesta
  * @param m Messaggio inviato dal client
  *
  * @return 0 in caso di successo
  *         -1 in caso di errore
  */
int posttxt_op(long fd, message_t m);

/**
  * @function posttxtall_op
  * @brief Funzione che gestisce la richiesta da parte di un client dell'invio a tutti gli utenti registrati di un certo messaggio.
  *        L'invio del messaggio segue le stesse regole della funzione posttxt_op
  *
  * @param fd Fd del client che sta facendo la richiesta
  * @param m Messaggio inviato dal client
  *
  * @return 0 in caso di successo
  *         -1 in caso di errore
  */
int posttxtall_op(long fd, message_t m);

/**
  * @function postfile_op
  * @brief Funzione che gestisce la richiesta di invio di un file da parte di un client a un altro.
  *        Il client destinatario sarà notificato nel caso in cui sia connesso.
  *        In ogni caso verrà aggiunto alla history del client destinatario il nome del file.
  *
  * @param fd Fd del client che sta facendo la richiesta
  * @param m Messaggio inviato dal client
  *
  * @return 0 in caso di successo
  *         -1 in caso di errore
  */
int postfile_op(long fd, message_t m);

/**
  * @function getfile_op
  * @brief Funzione che gestisce la richiesta da parte di un client del download di un file
  *
  * @param fd Fd del client che sta facendo la richiesta
  * @param m Messaggio inviato dal client
  *
  * @return 0 in caso di successo
  *         -1 in caso di errore
  */
int getfile_op(long fd, message_t m);

/**
  * @function getprevmsgs_op
  * @brief Funzione che gestisce l'invio della history relativa al client che ne ha fatto richiesta
  *
  * @param fd Fd del client che sta facendo la richiesta
  * @param m Messaggio inviato dal client
  *
  * @return 0 in caso di successo
  *         -1 in caso di errore
  */
int getprevmsgs_op(long fd, message_t m);

/**
  * @function usrlist_op
  * @brief Funzione che invia la lista degli utenti connessi al client che ne fa richiesta
  *
  * @param fd Fd del client che sta facendo la richiesta
  * @param m Messaggio inviato dal client
  * @param atomica Variabile booleana.
  *        Deve valere 1 se si invoca con la lock sulla hash degli utenti registrati, con la lock sull'array degli utenti connessi e con la lock sull'fd, 0 altirmenti.
  *
  * @return 0 in caso di successo
  *         -1 in caso di errore
  */
int usrlist_op(long fd, message_t m, int atomica);

/**
  * @function register_op
  * @brief Funzione che gestisce la richiesta di deregistrazione da parte di un client
  *
  * @param fd Fd del client che sta facendo la richiesta
  * @param m Messaggio inviato dal client
  *
  * @return 0 in caso di successo
  *         -1 in caso di errore
  */
int unregister_op(long fd, message_t m);

/**
  * @function register_op
  * @brief Funzione che gestisce la richiesta di disconnessione da parte di un client
  *
  * @param fd Fd del client che sta facendo la richiesta
  * @param m Messaggio inviato dal client
  *
  * @return 0 in caso di successo
  *         -1 in caso di errore
  */
int disconnect_op(long fd);


#endif
