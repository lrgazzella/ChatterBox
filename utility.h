#ifndef UTILITY_H_
#define UTILITY_H_
#endif /* UTILIY_H */

/* controlla -1; stampa errore e termina */
#define ec_meno1(s,m)  if ((s) == - 1) {\
    perror(m);\
    exit(EXIT_FAILURE);\
}
/* controlla NULL; stampa errore e  termina la funzione   */
#define ec_null_return(s,m) if ((s) == NULL) {\
    perror(m);\
    return -1;\
}

#define ec_meno1_return(s,m) if ((s) == -1) {\
    perror(m);\
    return -1;\
}
/* controlla -1; stampa errore ed esegue c */
#define ec_meno1_c(s,m,c) if ((s) == -1) {\
    perror(m);\
    c;\
}
