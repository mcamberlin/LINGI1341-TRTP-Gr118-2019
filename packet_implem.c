#include "packet_interface.h"
#include <stdio.h> 
#include <stdlib.h> // pour calloc, malloc, free
#include <string.h>
#include <arpa/inet.h>


/* Extra #includes */
/* Your code will be inserted here */


struct __attribute__((__packed__)) pkt {
    ptypes_t type:2;
    uint8_t tr:1;
    uint8_t window:5;
    uint8_t l:1;
    uint16_t length;
    uint8_t seqnum;
    uint32_t timestamp;
    uint32_t crc1;
    char* payload ; // utiliser un pointeur est plus intéressant au point de vue des performances et de la consommation de la mémoire. L'espace memoire est alloue grace a un malloc (paylaod=mallo(...)) (pas oublier free(...))
    uint32_t crc2;
};


int affichebin(char a)
{
    int i = 0;
    int* octet = malloc(8*sizeof(int));
 
    for(i = 0 ; i < 8 ; i++)
    {
        octet[i] = a % 2;
        a /= 2;
    }
	for(int i = 7; i>=0; i--)
	{
		printf("%d", octet[i]);
	}
	int res = *octet;
	free(octet);
	printf("\n");
    return res;
}


/* @return 1 si le ième bit =1
          0 si le ième bit = 0 */
char getBit(const char c, int index)
{
    char buffer = c;
    return ((buffer>>index) & 1);
}


/* Alloue et initialise une struct pkt
 * @return: NULL en cas d'erreur */
pkt_t* pkt_new()
{
    pkt_t* p = malloc(sizeof(pkt_t)); //initialise à zero
    if(p==NULL)
    {
        printf("allocation du nouveau paquet a echoue. \n");
        return NULL;
    }
    
    return p;
}

/* Libere le pointeur vers la struct pkt, ainsi que toutes les
 * ressources associees
 */
void pkt_del(pkt_t *pkt)
{
    free(pkt);
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
	printf("debut decode\n");
    int ptr=0; //"pointeur servant à parcourir data"

	affichebin(*data);
    

	printf("valeur pkt->type = %d\n", (*data)>>6);
    //pkt->type
    if(*data == 1)
    {
        pkt->type = PTYPE_DATA;
    }
    else
    {
        if(*data == 2)
        {
            pkt->type = PTYPE_ACK;
        }
        else
        {
            if(*data == 3)
            {
                pkt->type = PTYPE_NACK;
            }
            else
            {
                return E_TYPE;
            }
        }
    }
    
    //pkt->tr;
    pkt->tr = getBit(*data, 2);
    //possible erreur?????
    
    //pkt->window;
    pkt->window = (((*data)<<3)>>3);
    
    //A partir d'ici on traite le deuxieme byte de data
    ptr++;
    
    //pkt->l
    pkt->l = *(data+ptr)>>7;
    
    //pkt->length
    if(pkt->l == 0)
    {
        //length mesure 7 bits
        pkt->length = (*(data+ptr)<<1)>>1;
        if((pkt->length)>MAX_PAYLOAD_SIZE)
        {
            return E_LENGTH;
        }
        //pkt->length=htonl(pkt->length);
        ptr++;
        //verifie si le paquet a bien la bonne longueuer len
        //si L==0, longueur attedue = 12+longueur du payload
        if((uint16_t) len!=(pkt->length+12))
        {
            return E_UNCONSISTENT;
        }
    }
    else
    {
        //length mesure 15 bits
        uint16_t tmp = ((uint16_t) *(data+ptr)<<8) | (uint16_t) *(data+ptr+1); //concatenation
        pkt->length = tmp & 0b0111111111111111; //enlever le premier bit qui vaut 1
        pkt->length=ntohs(pkt->length);
        
        //verifie si le paquet a bien la bonne longueuer len
        //si L==1, longueur attendue = 16+longueur du payload
        if((uint16_t) len!=(pkt->length+16))
        {
            return E_UNCONSISTENT;
        }
        ptr=ptr+2;
    }
    
    //pkt->seqnum;
    pkt->seqnum = *(data+ptr);
    ptr++;
    
    //pkt->timestamp
    pkt->timestamp = ((uint32_t)*(data+ptr))<<24 | ((uint32_t)*(data+ptr+1))<<16 | ((uint32_t)*(data+ptr+2))<<8 | (uint32_t)*(data+ptr+3);
    ptr=ptr+4;
    
    //pkt->crc1
    pkt->crc1= ((uint32_t)*(data+ptr))<<24 | ((uint32_t)*(data+ptr+1))<<16 | ((uint32_t)*(data+ptr+2))<<8 | (uint32_t)*(data+ptr+3);
    pkt->crc1=ntohl(pkt->crc1);
    ptr=ptr+4;
    
    //pkt->paylaod
    if(pkt->tr==0)
    {
        pkt->payload = (char*) malloc(pkt->length);
        if(pkt->payload==NULL)
        {
            return E_NOMEM; //je sais pas quoi mettre d'autre
        } 
        //void *memcpy(void *dest, const void *src, size_t n);
        // The memcpy() function copies n bytes from memory area src to memory area dest.
        memcpy(pkt->payload, data+ptr, pkt->length);
        ptr = ptr+(pkt->length);
    }
    else
    {
        pkt->payload = NULL;
    }
    
    //pkt->crc2
    if(pkt->tr==1)
    {
        pkt->crc2 = ((uint32_t)*(data+ptr))<<24 | ((uint32_t)*(data+ptr+1))<<16 | ((uint32_t)*(data+ptr+2))<<8 | (uint32_t)*(data+ptr+3);
    }
    pkt->crc2=ntohl(pkt->crc2);
    
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

	printf("debut encode\n");
    /*
    if((uint16_t)*len < (15 + pkt->l*1 + pkt->length))
    {
		printf("encode, premier if\n");
        return E_NOMEM;
    }
*/
    int ptr = 0;
    
    //type, tr, window
    memcpy(buf, pkt, 1); //le premier byte de pkt est mis dans buf
    ptr++;

	printf("premier byte de pkt : ");
    
    //l, length
    if(pkt->l == 0)
    {
		printf("pkt->l=0\n");
        memcpy(buf+ptr, pkt+ptr, 1);
        ptr++;
    }
    else
    {
        
        uint16_t tp = 0b1000000000000000 | pkt->length; //ajouter le premier bit qui vaut 1
        tp = htons(tp);
        memcpy(buf+ptr, &tp, 2);
        ptr=ptr+2;
    }
    
    //seqnum
    memcpy(buf+ptr, &pkt->seqnum, 1);
    ptr++;
    
    //timestamp
    memcpy(buf+ptr, &pkt->timestamp, 4);
    ptr=ptr+4;
    
    //crc1
    uint32_t t = htonl(pkt->crc1);
    memcpy(buf+ptr, &t, 4);
    ptr = ptr+4;
    
    //payload
    memcpy(buf+ptr,pkt->payload,pkt->length);
    ptr = ptr + pkt->length;
    
    //CRC2
    uint32_t tmp = htonl(pkt->crc2);
    memcpy(buf+ptr,&tmp,4);    
    
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
    pkt->type=type;
    return PKT_OK;
}

pkt_status_code pkt_set_tr(pkt_t *pkt, const uint8_t tr)
{
    pkt->tr=tr;
    return PKT_OK;
}

pkt_status_code pkt_set_window(pkt_t *pkt, const uint8_t window)
{
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
    pkt->length=length;
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

pkt_status_code pkt_set_crc2(pkt_t *pkt, const uint32_t crc2)
{
    pkt->crc2=crc2;
    return PKT_OK;
}

pkt_status_code pkt_set_payload(pkt_t *pkt,
                                const char *data,
                                const uint16_t length)
{
    pkt->payload = (char*) malloc(length);
    memcpy(pkt->payload, data, length);
    return PKT_OK;
}


ssize_t varuint_decode(const uint8_t *data, const size_t len, uint16_t *retval)
{   
    //puisque en netword byte order : le bit qui donne la longueur est sur le deuxieme byte
    int bit = *(data+1)>>7;
    if(bit==1)//longueur de 2 by
    {
        if(len<2)
        {
            return -1;
        }
        *retval = ( ((uint16_t) *(data)<<8) | (uint16_t) *(data+1) ) & 0b0111111111111111;
        *retval = htonl(*retval);
        return 2;
    }
    *retval = (uint16_t) *data;
    return 1;
}

ssize_t varuint_encode(uint16_t val, uint8_t *data, const size_t len)
{
    
    int bit = val>>15;
    if(bit==0 && len>=1)
    {
        *data = (uint8_t) val;
        return 1;
    }
    else if(len>=2)
    {
        memcpy(data, &val, 2);
        return 2;
    }
    return -1;
}

size_t varuint_len(const uint8_t *data)
{
    if(*data>>7==0)
    {
        return 1;
    }
    return 2;
}


ssize_t varuint_predict_len(uint16_t val)
{
    char* ptr = (char*) &val;
    if(*ptr>>7==0)
    {
        return 1;
    } 
    return 2;
}


ssize_t predict_header_length(const pkt_t *pkt)
{
    if(pkt->l==0)
    {
        return 1;
    }
    return 2;
}




int main()
{
	pkt_t* pkt = calloc(1,sizeof(pkt_t));
	if(pkt==NULL)
	{
		printf("plantage malloc");
		return -1;
	}

	pkt->type = 1;
	printf("pkt->type = %d\n", pkt->type);
	pkt->tr = 0;
	pkt->window = 2;
	pkt->l=0;
	pkt->length = 0;
	pkt->seqnum = 1;
	pkt->timestamp=8;
	pkt->crc1 = 7;
	pkt->payload = NULL;
	pkt->crc2=4;

	char* buf = (char*) malloc(sizeof(pkt_t));
	if(buf==NULL)
	{
		printf("plantage malloc\n");
		return -1;
	}

	size_t len = sizeof(buf);
	pkt_status_code p = pkt_encode(pkt, buf, &len);
	if(p!=PKT_OK)
	{
		printf("plantage pkt_encode, valeur de retour : %d\n", p);
		return -1;
	}

	pkt_t* pktRecu = malloc(sizeof(pkt_t));
	if(pkt==NULL)
	{
		printf("plantage malloc\n");
		return -1;
	}

	pkt_status_code m = pkt_decode(buf, sizeof(buf), pktRecu);
	if(m!=PKT_OK)
	{
		printf("plantage pkt_decode, valeur de retour : %d\n", m);
		return -1;
	}

	return 0;

}


