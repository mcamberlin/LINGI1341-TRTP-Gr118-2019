#include "packet_interface.h"
#include <stdio.h> 
#include <stdlib.h> // pour calloc, malloc, free
#include <string.h>
#include <arpa/inet.h>
#include <stddef.h>
#include <stdint.h>
#include <zlib.h>
//realise avec l'aide de Antoine Caytan 

struct __attribute__((__packed__)) pkt 
{
	// 1er Octet
	uint8_t window:5;
	uint8_t tr:1;
	uint8_t type:2;

	// 2eme et 3eme Octet
	uint16_t length; // ! Attention ! length est un varuint: son bit de poid fort indique s'il mesure un ou deux octets

	uint8_t seqnum;
	uint32_t timestamp;
	uint32_t crc1;
	char* payload ; // utiliser un pointeur est plus intéressant au point de vue des performances et de la consommation de la mémoire. L'espace memoire est alloue grace a un malloc (paylaod=mallo(...)) (pas oublier free(...))
	uint32_t crc2;
};

/* Alloue et initialise une struct pkt
 * @return: NULL en cas d'erreur */
pkt_t* pkt_new()
{
    pkt_t* p = (pkt_t*) calloc(1,sizeof(pkt_t)); //initialise à zero
    if(p==NULL)
    {
        fprintf(stderr,"allocation du nouveau paquet a echoue. \n");
        return NULL;
    }
    
    return p;
}


/* Libere le pointeur vers la struct pkt, ainsi que toutes les
 * ressources associees
 */
void pkt_del(pkt_t *pkt)
{
	if(pkt!=NULL)
	{
		if(pkt->payload!=NULL)
		{
			free(pkt->payload);
			pkt->payload=NULL;
		}
        free(pkt);
        pkt=NULL;
    }
}


/*
 * Decode des donnees recues et cree une nouvelle structure pkt.
 * Le paquet recu est en network byte-order.
 * La fonction verifie que:
 * - Le CRC32 du header recu est le même que celui decode a la fin
 *   du header (en considerant le champ TR a 0)
 * - S'il est present, le CRC32 du payload recu est le meme que celui
 *   decode a la fin du payload
 * - Le type du paquet est valide
 * - La longueur du paquet et le champ TR sont valides et coherents
 *   avec le nombre d'octets recus.
 *
 * @data: L'ensemble d'octets constituant le paquet recu
 * @len: Le nombre de bytes recus
 * @pkt: Une struct pkt valide
 * @post: pkt est la representation du paquet recu
 *
 * @return: Un code indiquant si l'operation a reussi ou representant l'erreur rencontree.
 Valeur de retours des fonctions 
typedef enum {
    PKT_OK = 0,      Le paquet a ete traite avec succes 
    E_TYPE,          Erreur liee au champs Type 
    E_TR,            Erreur liee au champ TR 
    E_LENGTH,        Erreur liee au champs Length 
    E_CRC,           CRC invalide 
    E_WINDOW,        Erreur liee au champs Window
    E_SEQNUM,        Numero de sequence invalide 
    E_NOMEM,         Pas assez de memoire
    E_NOHEADER,      Le paquet n'a pas de header (trop court)
    E_UNCONSISTENT,  Le paquet est incoherent 
} pkt_status_code;
*/
pkt_status_code pkt_decode(const char *data, const size_t len, pkt_t *pkt)
{
	if(len <7)
	{
		return E_NOHEADER;
	}
	if(len <11) // La taille du header est inferieure a celle minimale.
	{
		return E_UNCONSISTENT; 
	}
	

    int ptr=0; //pointeur servant à parcourir data byte par byte
    
    // 1. type?
	uint8_t type = (uint8_t) (*data)>>6;
    if(type == 1)
    {
        pkt_set_type(pkt,PTYPE_DATA);
    }
    else if(type == 2)
    {
		pkt_set_type(pkt,PTYPE_ACK);
    }
    else if(type == 3)
    {
		pkt_set_type(pkt,PTYPE_NACK);
	}
    else
    {
		// Un paquet avec un autre type doit etre ignore.
        return E_TYPE;
    }

    // 2. tr?
    uint8_t tr = (*data<<2)>>7;
	pkt_set_tr(pkt,tr);

	if( type == PTYPE_DATA && tr!=0)
	{
		return E_TR;
	}

	// 3. window?
	uint8_t window = (*data<<3)>>3;
	pkt_set_window(pkt,window);
    
	ptr = ptr +1; // on deplace le pointeur de lecture sur le deuxieme byte de data
	
	// 4. length?
	uint8_t varLength [2]; //varuint16 representant length
	memcpy(varLength,data+ptr, 2);
	uint16_t leng;
	int l = (int) varuint_decode(varLength, 2,&leng); 
	//ssize_t varuint_decode(const uint8_t *data, const size_t len, uint16_t *retval); // l = nbre de byte(s) que fait length
	if(l == -1) //cas ou varuint_decode a plante
	{
		return E_LENGTH;
	}
	pkt_set_length(pkt,leng);

	ptr = ptr + l;
	
	// 5. seqnum? sur 1 octet
	uint8_t seqnum = *(data+ptr);
	pkt_set_seqnum(pkt,seqnum);

    ptr = ptr + 1;
    
    // 6. timestamp? encode sur 4 octets
	uint32_t timestamp;
	memcpy(&timestamp, data+ptr,4);
	pkt_set_timestamp(pkt,timestamp);
	
    ptr = ptr + 4;
   
    // 7. crc1? encode sur 4 octets
	uint32_t crc1;
	memcpy(&crc1,data+ptr,4);
    crc1 = ntohl(crc1);
	pkt_set_crc1(pkt,crc1);
    
		// 7.1 check crc1? 
	// Mettre a 0 TR
	char copie[ptr];
	memcpy(copie, data, ptr);
	copie[0] = copie[0] & 0b11011111;
	
	uint32_t crc1check = crc32((uint32_t) 0,(const Bytef *) copie, ptr);
    if(crc1!=crc1check)
	{
      return E_CRC;
    }

	ptr = ptr + 4;	
	
    // 8. payload? 
	if( pkt_get_type(pkt) == PTYPE_DATA && pkt_get_tr(pkt) == 0) // Si il s'agit bien d'un payload
	{
		pkt_status_code codePayload = pkt_set_payload(pkt, data+ptr, leng);
		if( codePayload != PKT_OK)
		{
		  return codePayload;
		}
	
		ptr = ptr + leng;
	
		// 9. crc2? encode sur 4 octets
		uint32_t crc2;
		memcpy(&crc2,data+ptr,4);
		crc2 = ntohl(crc2);
		pkt_status_code codeCrc2 = pkt_set_crc2(pkt,crc2);
		if( codeCrc2 != PKT_OK)
		{
		  return codeCrc2;
		}
			// 9.1 check crc2? 
		uint32_t crc2check = crc32((uint32_t) 0,(const Bytef *) pkt_get_payload(pkt) ,leng);

		if(crc2!=crc2check)
		{
		  return E_CRC;
		}

	}
	return PKT_OK;
}
/*
 * Encode une struct pkt dans un buffer, prÃƒÂªt a ÃƒÂªtre envoye sur le reseau
 * (c-a-d en network byte-order), incluant le CRC32 du header et
 * eventuellement le CRC32 du payload si celui-ci est non nul.
 *
 * @pkt: La structure a encoder
 * @buf: Le buffer dans lequel la structure sera encodee
 * @len: La taille disponible dans le buffer
 * @len-POST: Le nombre de d'octets ecrit dans le buffer
 * @return: Un code indiquant si l'operation a reussi ou E_NOMEM si
 *         le buffer est trop petit.
 */
pkt_status_code pkt_encode(const pkt_t* pkt, char *buf, size_t *len)
{
	//Verifier si la taille de buf est suffisante
	int tailleCrc2 = 0;
	if(pkt_get_tr(pkt) == 0 && pkt_get_length(pkt) != 0) //si il y a un crc2
	{
		tailleCrc2 = 4;
	}
	
	if((int) *len< (predict_header_length(pkt) + 4 + pkt_get_length(pkt) +  tailleCrc2)) //verifier espace suffisant dans le buf
	{
		return E_NOMEM;
	}

	//a ignorer
	if(pkt_get_type(pkt)==PTYPE_DATA && pkt_get_tr(pkt)!=0)
	{
		return E_TR;
	}
	
	*len = 0;
    
	//1. type?
    uint8_t type = pkt_get_type(pkt);
	if (type == PTYPE_DATA)
    {
        type = 1;
    }
    else if (type == PTYPE_ACK)
    {
        type = 2;
    }
    else if (type == PTYPE_NACK)
    {
        type = 3;
    }
    else
    {
        return E_TYPE;
    }
	//2. tr?
	uint8_t tr = pkt_get_tr(pkt);
	
	//3. type?

	uint8_t window = pkt_get_window(pkt);
	uint8_t premierbyte = (type | tr | window);
	memcpy(buf, &premierbyte,1);
    
	*len = *len +1;

	//4. length?
	uint8_t varLength [2]; //varuint16 representant length
	size_t l = varuint_encode(pkt_get_length(pkt),(uint8_t*) varLength,2); 
	//ssize_t varuint_encode(uint16_t val, uint8_t *data, const size_t len)

	if(l == 1) //cas ou la longueur est sur 1 byte
	{
		memcpy(buf + (*len), varLength,1);
		
		*len = *len +1;

	}
	else if(l ==2) //cas ou la longueur est sur 2 bytes
	{
		memcpy(buf + (*len), varLength,2);
		
		*len = *len +2;

	}
	
	//5. seqnum?
	uint8_t seqnum = pkt_get_seqnum(pkt);
    memcpy(buf + (*len), &seqnum, 1);
	
    *len = *len +1;
	
	//6. timestamp?
	uint32_t timestamp = pkt_get_timestamp(pkt);
    memcpy(buf + (*len), &timestamp, 4);
    
	*len = *len + 4;

	//7. crc1?
	// Mettre a 0 TR
	char copie[predict_header_length(pkt)];
	memcpy(copie, buf, *len);
	copie[0] = copie[0] & 0b11011111;
	
	
 	uint32_t crc1=crc32(0,(const Bytef *) copie, *len);
    	crc1=htonl(crc1);
    	memcpy(buf + (*len), &crc1,4);
	
	*len = *len +4;

	//payload?
    if(pkt_get_payload(pkt)==NULL) // si il n'y a pas de payload
	{
      return PKT_OK;
    }

    memcpy(buf+ *(len),pkt_get_payload(pkt),pkt_get_length(pkt));
	
	*len = *len + pkt_get_length(pkt);

	//crc2?
	if(pkt_get_tr(pkt) == 0 && pkt_get_payload(pkt) != NULL) // si il y a un payload
	{
		uint32_t crc2=crc32(0,(const Bytef *) (buf + *(len) - pkt_get_length(pkt)), pkt_get_length(pkt));
		crc2=htonl(crc2);
		memcpy(buf+ (*len),&crc2,4);

		*len = *len + 4;
	}

    return PKT_OK;

}

ptypes_t pkt_get_type  (const pkt_t* pkt)
{
    return pkt->type;
}

uint8_t  pkt_get_tr(const pkt_t* pkt)
{
    return pkt->tr;
}

uint8_t  pkt_get_window(const pkt_t* pkt)
{
    return pkt->window;
}

uint8_t  pkt_get_seqnum(const pkt_t* pkt)
{
    return pkt->seqnum;
}

uint16_t pkt_get_length(const pkt_t* pkt)
{
    return pkt->length;
}

uint32_t pkt_get_timestamp   (const pkt_t* pkt)
{
    return pkt->timestamp;
}

uint32_t pkt_get_crc1   (const pkt_t* pkt)
{
    return pkt->crc1;
}

uint32_t pkt_get_crc2   (const pkt_t* pkt)
{
    return pkt->crc2;
}

const char* pkt_get_payload(const pkt_t* pkt)
{
    return pkt->payload;
}


pkt_status_code pkt_set_type(pkt_t *pkt, const ptypes_t type)
{
	if(type==PTYPE_DATA)
	{
		pkt->type=1;
		return PKT_OK;
	}
	else if(type==PTYPE_ACK)
	{
		pkt->type=2;
		return PKT_OK;
    	}
	else if(type==PTYPE_NACK)
	{
		pkt->type=3;
		return PKT_OK;
	}
	else
	{
		return E_TYPE;
	}
}

pkt_status_code pkt_set_tr(pkt_t *pkt, const uint8_t tr)
{
	if(pkt == NULL)
	{
		return E_TR;
	}
	else if(pkt->type != 1)
	{	
		return E_TR;
	} 
	else
	{
		pkt->tr=tr;
		return PKT_OK;
	}	
}


pkt_status_code pkt_set_window(pkt_t *pkt, const uint8_t window)
{
	if(pkt == NULL)
	{
		fprintf(stderr,"Le pointeur pkt est NULL \n");
		return E_WINDOW;
	}
	if(window >31 )
	{
		fprintf(stderr,"La fenetre n'est pas comprise entre 0 et 31 alors qu'elle est encodee sur 5 bits \n");
		return E_WINDOW;
	}
	
	pkt->window=window;
	return PKT_OK;
}

pkt_status_code pkt_set_seqnum(pkt_t *pkt, const uint8_t seqnum)
{
    pkt->seqnum=seqnum;
    return PKT_OK;
}

pkt_status_code pkt_set_length(pkt_t *pkt, const uint16_t length)
{
	if((length & 0b0111111111111111)>MAX_PAYLOAD_SIZE)
	{
		return E_LENGTH;
	}
	pkt->length = length & 0b0111111111111111;
	return PKT_OK;
	
}

pkt_status_code pkt_set_timestamp(pkt_t *pkt, const uint32_t timestamp)
{
    pkt->timestamp=timestamp;
    return PKT_OK;
}

pkt_status_code pkt_set_crc1(pkt_t *pkt, const uint32_t crc1)
{
    pkt->crc1=crc1;
    return PKT_OK;
}

/* Defini la valeur du champs payload du paquet.
 * @data: Une succession d'octets representants le payload
 * @length: Le nombre d'octets composant le payload
 * @POST: pkt_get_length(pkt) == length */
pkt_status_code pkt_set_payload(pkt_t *pkt, const char *data, const uint16_t length)
{
	if(length>MAX_PAYLOAD_SIZE) //voir packet_interface.h
	{
		return E_NOMEM;
	}
	if((data==NULL)||(length == 0)) // s'il n'y a pas de payload a setter
	{
	        return PKT_OK;
    	}
	if(pkt->payload!=NULL) // Libérer l'éventuel payload précédent
	{
		free(pkt->payload);
		pkt->payload=NULL;
		pkt->length=0;
	}
	pkt->payload = (char*) malloc(length);
	if(pkt->payload == NULL)
	{
		return E_NOMEM;
	}
	memcpy(pkt->payload, data, length);
	return PKT_OK;   
}

/* Setter pour CRC2. Les valeurs fournies sont dans l'endianness
 * native de la machine!
 */
pkt_status_code pkt_set_crc2(pkt_t *pkt, const uint32_t crc2)
{
    pkt->crc2=crc2;
    return PKT_OK;
}

/*
 * Decode un varuint (entier non signe de taille variable  dont le premier bit indique la longueur)
 * encode en network byte-order dans le buffer data disposant d'une taille maximale len.
 * @post: place a l'adresse retval la valeur en host byte-order de l'entier de taille variable stocke
 * dans data si aucune erreur ne s'est produite
 * @return:
 *          -1 si data ne contient pas un varuint valide (la taille du varint
 * est trop grande par rapport a la place disponible dans data)
 *
 *          le nombre de bytes utilises si aucune erreur ne s'est produite
 */
ssize_t varuint_decode(const uint8_t *data, const size_t len, uint16_t *retval)
{
	size_t nbBytes = (int) varuint_len(data);
	if(len < nbBytes)
	{
		fprintf(stderr,"La taille du buffer est insuffisante \n");
		return -1;
	}
	else if(nbBytes == 1)
	{
		*retval = 0b01111111 & *data;
		return nbBytes;
	}
	else if(nbBytes == 2)
	{
		uint16_t* tmp = (uint16_t*) data;
		*tmp = ntohs(*tmp); //Convert in network to host byte order
		*retval = 0b0111111111111111 & *tmp; // Mettre le premier bit a O
		return nbBytes;
	}
	else
	{
		fprintf(stderr,"Le nombre de byte est incoherent \n");
		return -1;
	}
}


/*
 * Encode un varuint en network byte-order dans le buffer data disposant d'une
 * taille maximale len.
 * @pre: val < 0x8000 (val peut etre encode en varuint)
 * @return:
 *           -1 si data ne contient pas une taille suffisante pour encoder le varuint
 *
 *           la taille necessaire pour encoder le varuint (1 ou 2 bytes) si aucune erreur ne s'est produite
 */
ssize_t varuint_encode(uint16_t val, uint8_t *data, const size_t len)
{
	if(val >= 0x8000) // Programmation defensive
	{
		fprintf(stderr,"val est superieur a 0X8000 \n");
		return -1;
	}
	uint8_t nbBytes = varuint_predict_len(val);
	if(nbBytes<len)
	{
		fprintf(stderr,"La taille du buffer est insuffisante \n");
		return -1;
	}
	
	if(nbBytes == 1)
	{
		memcpy(data, &val,1);
		return nbBytes;
	}
	else if( nbBytes == 2)
	{
		uint16_t tmp = val + 0x8000; // mettre le premier bit à 1
		tmp = htons(tmp);
		memcpy(data,&tmp,nbBytes); // Convert in network byte-order
		return nbBytes;
	}
	else
	{
		fprintf(stderr,"Le nombre de byte est incoherent: %d \n",nbBytes);
		return -1;
	}
}

/*
 * @pre: data pointe vers un buffer d'au moins 1 byte
 * @return: la taille en bytes du varuint stocke dans data, soit 1 ou 2 bytes.
 */
size_t varuint_len(const uint8_t *data)
{
	if(data == NULL)
	{
		fprintf(stderr,"Le pointeur data est NULL \n");
		return -1;
	}

	if(*(data)>>7)
	{
		return 2;
	}
	return 1;
}

/*
 * @return: la taille en bytes que prendra la valeur val
 * une fois encodee en varuint si val contient une valeur varuint valide (val < 0x8000).
            -1 si val ne contient pas une valeur varuint valide
    Utiliser pour encode 
 */
ssize_t varuint_predict_len(uint16_t val)
{
	if(val>=0x8000) // Programmation defensive
	{
		fprintf(stderr,"Val est superieur ou egal a Ox8000\n");
		return -1;
	}
	if(val>=128)
	{
		return 2;
	}
	else
	{
		return 1;
	}
	return -1;
}

/*
 * Retourne la longueur du header en bytes si le champs pkt->length
 * a une valeur valide pour un champs de type varuint (i.e. pkt->length < 0x8000).
 * Retourne -1 sinon
 * @pre: pkt contient une adresse valide (!= NULL)
 */
ssize_t predict_header_length(const pkt_t *pkt)
{
	if(pkt == NULL) // Programmation defensive
	{
		fprintf(stderr, "Le pointeur pkt est NULL \n");
		return -1;
	}
	ssize_t nbByteLength = varuint_predict_len(pkt->length);
	return 6 + nbByteLength;
}




/* -------------------------------- FONCTION INUTILE ? ----------------------------------------- */
/* return la valeur reelle du champ length c-a-d sans le premier bit l
*/
uint16_t realvalue(uint16_t pkt_length)
{
	return pkt_length & 0b0111111111111111;
}
void affichebin(char a)
{
    int i = 0;
 
    for(i = 7 ; i >=0 ; i--)
    {
        int res = a>>i & 1;
		printf("%d", res);
    }
	printf("\n");
}
/* @return 1 si le ième bit =1
          0 si le ième bit = 0 */
char getBit(const char c, int index)
{
    char buffer = c;
    return ((buffer>>index) & 1);
}



int main()
{

	pkt_t* pkt = pkt_new();
	pkt_set_type(pkt, PTYPE_DATA);
	pkt_set_tr(pkt,0);
	pkt_set_window(pkt,1);

//1. type?
    uint8_t type = pkt_get_type(pkt);
	if (type == PTYPE_DATA)
    {
        type = 1;
    }
    else if (type == PTYPE_ACK)
    {
        type = 2;
    }
    else if (type == PTYPE_NACK)
    {
        type = 3;
    }
    else
    {
        return E_TYPE;
    }
	//2. tr?
	uint8_t tr = pkt_get_tr(pkt);
	
	//3. type?

	uint8_t window = pkt_get_window(pkt);
	uint8_t premierbyte = (type | tr | window);

	affichebin(premierbyte);
	
}

