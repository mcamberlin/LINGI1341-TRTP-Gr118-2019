#include "packet_interface.h"
#include <stdio.h> 
#include <stdlib.h> // pour calloc, malloc, free
#include <string.h>
#include <arpa/inet.h>



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
    pkt_t* p = (pkt_t*) calloc(0,sizeof(pkt_t)); //initialise à zero
    if(p==NULL)
    {
        printf("allocation du nouveau paquet a echoue. \n");
        return NULL;
    }
    
    return p;
}


/* @return 1 si le ième bit =1
          0 si le ième bit = 0 */
char getBit(const char c, int index)
{
    char buffer = c;
    return ((buffer>>index) & 1);
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
	
    int ptr=0; //"pointeur servant à parcourir data"

	printf("Premier byte de data dans decode : ");
	affichebin(*data);
    
    //pkt->type
	int type = (int) (*data)>>6;
    if(type == 1)
    {
        pkt->type = PTYPE_DATA;
		printf("pkt->type = 1\n");
    }
    else
    {
        if(type == 2)
        {
            pkt->type = PTYPE_ACK;
			printf("pkt->type = 2\n");
        }
        else
        {
            if(type == 3)
            {
                pkt->type = PTYPE_NACK;
				printf("pkt->type = 3\n");
            }
            else
            {
                return E_TYPE;
            }
        }
    }
    
    //pkt->tr;
    pkt->tr = (*data<<2)>>7;
	printf("pkt->tr = %d\n", getBit(*data, 5));
	if(type!=PTYPE_DATA && pkt->tr!=0)
	{
		return E_TR;
	}
    
    //pkt->window;
    pkt->window = (*data) & 0b00011111;
	printf("pkt->windom = %d\n", (int) pkt->window);
    
    //A partir d'ici on traite le deuxieme byte de data, ca va chier 
    ptr++;
    
    //pkt->length
	int l = varuint_predict_len((uint16_t) *(data+ptr));
    if(l == 1)
    {
        //length mesure 7 bits et l=0
        pkt->length = ((*(data+ptr))<<1)>>1; //le l disparait
        if((pkt->length)>MAX_PAYLOAD_SIZE) //securite en plus (varuint_predict_len le fait deja normalement)
        {
            return E_LENGTH;
        }
        ptr++;
        //verifie si le paquet a bien la bonne longueuer len
        //si L==0, longueur attedue = 12+longueur du payload
		/*
        if((uint16_t) len!=(pkt->length+15))
        {
			printf("le paquet est inconscient\n");
            return E_UNCONSISTENT;
        }
		*/
		//on traite les erreurs a la fin 
		printf("pkt->length = %d\n", pkt->length);
    }
    else
    {
        //length mesure 15 bits
		// [--------][-------l]
		printf("length dans data : \n");
		affichebin(*(data+ptr));
		affichebin(*(data+ptr+1));
		
        uint16_t tmp = (((uint16_t) *(data+ptr))<<9)>>1 | (uint16_t) *(data+ptr+1); //retire le l
		if(tmp > MAX_PAYLOAD_SIZE)
		{
			printf("Valeur du champ length (=%d) > 512", tmp);
			return E_LENGTH;
		}

		printf("length dans tmp : \n");
		affichebin((char) tmp);
		affichebin(*( (char*) (&tmp) +1));

        pkt->length=ntohs(tmp);
		printf("pkt->length (sur 15 bits) = %d\n", pkt->length);
        
        //verifie si le paquet a bien la bonne longueuer len
        //si L==1, longueur attendue = 16+longueur du payload
        if((uint16_t) len!=(pkt->length)+16)
        {
            return E_UNCONSISTENT;
        }

        ptr=ptr+2;
    }
    
    //pkt->seqnum;
    pkt->seqnum = *(data+ptr);
	printf("pkt->seqnum = %d\n", pkt->seqnum);
    ptr++;
    
    //pkt->timestamp
    pkt->timestamp = ((uint32_t)*(data+ptr+3))<<24 | ((uint32_t)*(data+ptr+2))<<16 | ((uint32_t)*(data+ptr+1))<<8 | (uint32_t)*(data+ptr);
	printf("pkt->timestamp = %d\n", pkt->timestamp);
	
    ptr=ptr+4;
    
    //pkt->crc1
    pkt->crc1= ((uint32_t)*(data+ptr+3))<<24 | ((uint32_t)*(data+ptr+2))<<16 | ((uint32_t)*(data+ptr+1))<<8 | (uint32_t)*(data+ptr);
    pkt->crc1=ntohl(pkt->crc1);
	printf("pkt->crc1 = %d\n", pkt->crc1);
    ptr=ptr+4;
    
    //pkt->paylaod
    if(pkt->tr==0)
    {
        pkt->payload = (char*) malloc(pkt->length);
        if(pkt->payload==NULL)
        {
            return E_NOMEM; //je sais pas quoi mettre d'autre
        } 
        memcpy(pkt->payload, data+ptr, pkt->length);
        ptr = ptr+(pkt->length);
    }
    else
    {
        pkt->payload = NULL;
    }
	printf("pkt->payload = %d\n", *pkt->payload);
	printf("adresse de pkt->paylaod = %p\n", pkt->payload);
    
    //pkt->crc2
    if(pkt->tr==1)
    {
        pkt->crc2 = ((uint32_t)*(data+ptr+3))<<24 | ((uint32_t)*(data+ptr+2))<<16 | ((uint32_t)*(data+ptr+1))<<8 | (uint32_t)*(data+ptr);
    }
    pkt->crc2=ntohl(pkt->crc2);
	printf("pkt->crc2 = %d\n", pkt->crc2);
    
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

	printf("----DEBUT ENCODE----\n");


	printf("Taille maxi: len = %ld\n", *len);
    
    if((uint16_t)*len < (15 + (pkt->length>>15)*1 + realvalue(pkt->length)))
    {
		printf("encode, premier if. len = %ld; longueur totale = %d\n",*len, (15 + (pkt->length>>15)*1 + realvalue(pkt->length)));
        return E_NOMEM;
    }

    int ptr = 0;
    
    //type, tr, window
	if(pkt->type!=PTYPE_DATA && pkt->tr!=0)
	{
		return E_TR;
	}
    memcpy(buf, pkt, 1); //le premier byte de pkt est mis dans buf
    ptr++;

	printf("premier byte de buf dans encode : ");
	affichebin(*(buf));
    
    //l, length
	int l = varuint_predict_len(pkt->length);
    if(l == 1)//length mesure 1 byte
    {
		uint8_t pktlength = (uint8_t) (pkt->length);
        memcpy(buf+ptr, &pktlength, 1);
		printf("second byte de buf dans encode : ");
		affichebin(*(buf+ptr));
        ptr++;
    }
    else
    {
        uint16_t tp = (pkt->length); //mettre en network byte order
		printf("tp = %d\n", tp);
        tp = htons(tp) | (0b10000000)<<8; //met l=1
        memcpy(buf+ptr, &tp, 2);
		printf("les deux bytes de length et l sont : \n");
		affichebin(*(buf+ptr));
		affichebin(*(buf+ptr+1));
        ptr=ptr+2;
    }
	printf("fin length\n");
    
    //seqnum
    memcpy(buf+ptr, &(pkt->seqnum), 1);
	printf("seqnum = ");
	affichebin(*(buf+ptr));
	printf("fin seqnum\n");
    ptr++;
    
    //timestamp
    memcpy(buf+ptr, &(pkt->timestamp), 4);
	printf("fin timestamp\n");
    ptr=ptr+4;
    
    //crc1
    uint32_t t = htonl(pkt->crc1);
    memcpy(buf+ptr, &t, 4);
	printf("fin crc1\n");
    ptr = ptr+4;
    
    //payload
    memcpy(buf+ptr,pkt->payload,pkt->length);
	free(pkt->payload);
    printf("fin payload\n");
    ptr = ptr + pkt->length;
    
    //CRC2
    uint32_t tmp = htonl(pkt->crc2);
	printf("fin crc2\n");
    memcpy(buf+ptr,&tmp,4);  

    
	printf("----FIN ENCODE----\n\n\n");
	
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
	else if(window >=0 && window<=31 )
	{
		fprintf(stderr,"La fenetre n'est pas comprise entre 0 et 31 alors qu'elle est encodee sur 5 bits \n");
		return E_WINDOW;
	}
	else 
	{
		pkt->window=window;
		return PKT_OK;
	}
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
	size_t nbBytes = varuint_predict_len(data);
	if(len < nbBytes)
	{
		fprintf(stderr,"La taille du buffer est insuffisante \n");
		return -1;
	}
	else if(nbBytes == 1)
	{
		*retVal = 0b01111111 & *data;
		return nbBytes;
	}
	else if(nbBytes == 2)
	{
		uint16_t* tmp = (uint16_t*) data;
		tmp = ntohs(*tmp); //Convert in network to host byte order
		*retval = 0b0111111111111111 & tmp; // Mettre le premier bit a O
		return nbBytes;
	}
	else
	{
		fprintf(stderr,"Le nombre de byte est incoherent: %d \n",nbBytes);
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
		fprintf("Val est superieur ou egal a Ox8000\n");
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


