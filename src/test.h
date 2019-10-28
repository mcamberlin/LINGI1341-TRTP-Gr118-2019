#ifndef __CONNEXIONLIST_H_
#define __CONNEXIONLIST_H_


#include <stdio.h> 
#include <stdlib.h> // pour calloc, malloc, free
#include <string.h>
#include "packet_interface.h"


typedef struct C_node C_node_t;

extern int numeroFichier;

C_node_t** create();

#endif
