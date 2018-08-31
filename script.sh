#!/bin/bash

#An additional binary operator, =~, is available, with the same precedence as == and !=. When it is used, the string to the right of the operator is considered an extended regular expression and matched accordingly (as in regex(3)). The return value is 0 if the string matches the pattern, and 1 otherwise. If the regular expression is syntactically incorrect, the conditional expression's return value is 2. If the shell option nocasematch is enabled, the match is performed without regard to the case of alphabetic characters. Any part of the pattern may be quoted to force it to be matched as a string.

# REGEXP: https://regexr.com/
# ^ Matcha la stringa vuota all'inizio di una riga; Rappresenta anche i caratteri non nell'intervallo di una lista (tutti tranne quelli che sono nell'intervallo)
# [^#] Matcha qualsiasi carattere che non è nel set -> Esclude le linee che iniziano con #
# * match 0 or more of the preceding token
# = mathca solo le righe che contengono il simbolo uguale

#/** @file script.sh
#  * @author Lorenzo Gazzella 546890
#  * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
#  * originale dell'autore
#  */

case $@ in  *"--help"*)
	echo "Usa $0 file_conf t"
	exit 1 ;; esac

if [[ $# != 2 ]]; then
    echo "Errore. Usa $0 file_conf t"
    exit 1
fi

if ! [ -e $1 ]; then
    echo "Errore. $1 file non esistente."
    exit 1;
fi


# Parso tutto il file
i=0
while read line; do
  if [[ "$line" =~ ^[^\#]*= ]]; then
    name[i]=${line%% =*} # ${<var>%%<pattern>}: se <pattern> occorre alla fine di $<var> ritorna la stringa ottenuta eliminando da  $<var> la più lunga occorrenza finale  di <pattern>
    value[i]=${line#*= } # ${<var>#<pattern>}:  se <pattern> occorre all’inizio di $<var> ritorna la stringa ottenuta eliminando da $<var> la più corta occorrenza iniziale di <pattern>
    ((i++))
  fi
done < $1

# Cerco l'indice in cui è contenuto DirName
for ((j=0; j<$i; j++)) ; do
    case ${name[$j]} in  *"DirName"*)
	dir=$j
	break
	exit 1 ;; esac
done

if [ $2 = 0 ]; then
    ls ${value[$dir]}
    exit 0
fi
# A questo punto value[dir] contiene il valore di DirName

daEliminare=$(find "${value[$dir]}" -type f,d -mmin +$2) # Restituisce la lista dei file che sono più vecchi di t minuti.
if [ $? != 0 ]; then # $? contiene il valore di ritorno della chiamata di funzione. 0 indicates success, others indicates error
    echo "Errore comando find"
    exit 1
fi

tar cf history.tar.gz $daEliminare
if [ $? != 0 ]; then
    echo "Errore comando tar"
    exit 1
fi

rm $daEliminare
if [ $? != 0 ]; then
    echo "Errore comando rm"
    exit 1
fi

echo "Completato"
