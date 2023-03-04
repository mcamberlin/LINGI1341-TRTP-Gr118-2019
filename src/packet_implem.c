#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <unistd.h> 
#include <errno.h>
#include <arpa/inet.h>
#include <stddef.h>
#include <stdint.h>
#include <zlib.h>

#include "packet_interface.h"
#include "utils.h"

struct __attribute__((__packed__)) pkt {
    uint8_t window:5;   //0b 000x xxxx
    uint8_t tr:1;       //0b 00x0 0000
    uint8_t type:2;     //0b xx00 0000 
    uint16_t length;
    uint8_t seqnum;
    uint32_t timestamp;
    uint32_t crc1;
    char* payload;
    uint32_t crc2;     
};

/* =========================== VARUINT ===================================*/
size_t varuint_len(const uint8_t *data){
	if(data == NULL){
		return -1;
	}

	if(*(data)>>7 == 1){
		return 2;
	}
	return 1;
}

ssize_t varuint_decode(const uint8_t *data, const size_t len, uint16_t *retval)
{
	size_t nbBytes = (size_t) varuint_len(data);
	if(len < nbBytes){
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
		return -1;
	}
}

ssize_t varuint_encode(uint16_t val, uint8_t *data, const size_t len){
	if(val >= 0x8000){
		return -1;
	}
	if(len == 1){
		memcpy(data, &val,len);
		return len;
	}else if(len == 2){
		uint16_t tmp = htons(0b1000000000000000 | val);
		memcpy(data,&tmp,len); // Convert in network byte-order
		return len;
	}
	return len;
}

ssize_t varuint_predict_len(uint16_t val){
	if(val>=0x8000){
		return -1;
	}else if(val>=0x0080){
		return 2;
	}else{
		return 1;
	}
}


/* =========================== NEW DELETE ===================================*/
pkt_t* pkt_new()
{
    pkt_t* pkt = (pkt_t*) calloc(1, sizeof(pkt_t));
    if(pkt == NULL){
        error(-1, "Error allocate memory for new packet");
        return NULL;
    }
    return pkt;
}

void pkt_del(pkt_t *pkt)
{
    if(pkt!=NULL){
		if(pkt->payload!=NULL)
		{
			free(pkt->payload);
			pkt->payload=NULL;
		}
        free(pkt);
        pkt=NULL;
    }
}

/* ========================== ENCODE DECODE ==================================*/
pkt_status_code pkt_decode(const char *data, const size_t len, pkt_t *pkt)
{
    if(len <7){
		return E_NOHEADER;
	} else if(len <11){
		return E_UNCONSISTENT; 
	}
	
    int ptr=0; //pointeur servant à parcourir data byte par byte
   
    // Byte 1
    memcpy(pkt, data, 1);
    if(pkt_get_type(pkt)==0){
        return E_TYPE;
    } else if(pkt_get_type(pkt)==PTYPE_DATA && pkt_get_tr(pkt)!=0){
        return E_TR;
    }
    ptr = ptr +1;

    // Byte 2
	uint8_t varLength [2];
	memcpy(varLength,data+ptr, 2);
	uint16_t leng;
	int l = (int) varuint_decode(varLength, 2, &leng); 
	if(l == -1) {
		return E_LENGTH;
	}
	pkt_set_length(pkt,leng);
	ptr = ptr + l;
	
	// Seqnum? sur 1 octet
	uint8_t seqnum = *(data+ptr);
	pkt_set_seqnum(pkt,seqnum);
    ptr = ptr + 1;
    
    // Timestamp? encode sur 4 octets
	uint32_t timestamp;
	memcpy( (void*) &timestamp, data+ptr,4);
	pkt_set_timestamp(pkt,timestamp);
    ptr = ptr + 4;
   
    // Crc1? encode sur 4 octets
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

pkt_status_code pkt_encode(const pkt_t* pkt, char *buf, size_t *len)
{
	//Verifier si la taille de buf est suffisante
	int tailleCrc2 = 0;
	if(pkt_get_tr(pkt) == 0 && pkt_get_length(pkt) != 0){
		tailleCrc2 = 4;
	}
	
	if((int) *len< (predict_header_length(pkt) + 4 + pkt_get_length(pkt) +  tailleCrc2)) //verifier espace suffisant dans le buf
	{
		return E_NOMEM;
	}

	//a ignorer
	if(pkt_get_type(pkt)!=PTYPE_DATA && pkt_get_tr(pkt)!=0)
	{
		return E_TR;
	}
	
	*len = 0;

	uint8_t type = pkt_get_type(pkt);
	if (type == PTYPE_DATA)
	{
		type = 1;
	}else if (type == PTYPE_ACK){
		type = 2;
	}else if (type == PTYPE_NACK){
		type = 3;
	}else{
		return E_TYPE;
	}

    //1. 2. 3. encode le premier byte (=type, tr , window)
	memcpy(buf, pkt, 1);


	*len = *len +1;

	//4. length?
	uint8_t varLength [2]; //varuint16 representant length
	ssize_t predictLen = varuint_predict_len(pkt_get_length(pkt));
	ssize_t l = varuint_encode(pkt_get_length(pkt),(uint8_t*) varLength, predictLen);  
	//ssize_t varuint_encode(uint16_t val, uint8_t *data, const size_t len)
	if(l==-1)
    {
        return E_LENGTH;
    }
	if(l == 1) //cas ou la longueur est sur 1 byte
	{
		memcpy(buf + (*len), varLength,1);
		
		*len = *len +1;
	}
	if(l ==2) //cas ou la longueur est sur 2 bytes
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
	char copie[*len];
	memcpy(copie, buf, *len);
	copie[0] = copie[0] & 0b11011111;
	
	
 	uint32_t crc1=crc32(0,(const Bytef *) &copie, *len);
	crc1=htonl(crc1);
	memcpy(buf + (*len), &crc1,4);
	
	*len = *len +4;

	//8. payload?
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

/* ================================ GETTERS ==================================*/
ptypes_t pkt_get_type  (const pkt_t* pkt){
    return pkt->type;
}

uint8_t  pkt_get_tr(const pkt_t* pkt){
    return pkt->tr;
}

uint8_t  pkt_get_window(const pkt_t* pkt){
    return pkt->window;
}

uint8_t  pkt_get_seqnum(const pkt_t* pkt){
    return pkt->seqnum;
}

uint16_t pkt_get_length(const pkt_t* pkt){
    return pkt->length;
}

uint32_t pkt_get_timestamp   (const pkt_t* pkt){
    return pkt->timestamp;
}

uint32_t pkt_get_crc1   (const pkt_t* pkt){
    return pkt->crc1;
}

uint32_t pkt_get_crc2   (const pkt_t* pkt){
    return pkt->crc2;
}

const char* pkt_get_payload(const pkt_t* pkt){
    return pkt->payload;
}

/* ================================ SETTERS ==================================*/
pkt_status_code pkt_set_type(pkt_t *pkt, const ptypes_t type){
	if(type==PTYPE_DATA)	{
		pkt->type=1;
		return PKT_OK;
	}else if(type==PTYPE_ACK){
		pkt->type=2;
		return PKT_OK;
    }else if(type==PTYPE_NACK){
		pkt->type=3;
		return PKT_OK;
	}else{
		return E_TYPE;
	}
}

pkt_status_code pkt_set_tr(pkt_t *pkt, const uint8_t tr){
	if(pkt == NULL){
		return E_TR;
	}else if(pkt->type != 1){	
		return E_TR;
	}else{
		pkt->tr=tr;
		return PKT_OK;
	}	
}

pkt_status_code pkt_set_window(pkt_t *pkt, const uint8_t window){
	if(pkt == NULL){
		return E_WINDOW;
    }else if(window >31 ){
		return E_WINDOW;
	}
	pkt->window=window;
	return PKT_OK;
}

pkt_status_code pkt_set_seqnum(pkt_t *pkt, const uint8_t seqnum){
    pkt->seqnum=seqnum;
    return PKT_OK;
}

pkt_status_code pkt_set_length(pkt_t *pkt, const uint16_t length){
	if((length)>MAX_PAYLOAD_SIZE){
		return E_LENGTH;
	}
	pkt->length = length & 0b0111111111111111;
	return PKT_OK;
}

pkt_status_code pkt_set_timestamp(pkt_t *pkt, const uint32_t timestamp){
    pkt->timestamp=timestamp;
    return PKT_OK;
}

pkt_status_code pkt_set_crc1(pkt_t *pkt, const uint32_t crc1){
    pkt->crc1=crc1;
    return PKT_OK;
}

pkt_status_code pkt_set_payload(pkt_t *pkt, const char *data, const uint16_t length){
	if(length>MAX_PAYLOAD_SIZE){
		return E_NOMEM;
	}else if((data==NULL)||(length == 0)) // s'il n'y a pas de payload a setter
	{
        return PKT_OK;
    } else if (pkt->payload!=NULL){
		free(pkt->payload);
		pkt->payload=NULL;
		pkt->length=0;
	}
	pkt->payload = (char*) calloc(1,length);
	if(pkt->payload == NULL){
		return E_NOMEM;
	}
	memcpy(pkt->payload, data, length);
    pkt->length = length;
	return PKT_OK;   
}

pkt_status_code pkt_set_crc2(pkt_t *pkt, const uint32_t crc2){
    pkt->crc2=crc2;
    return PKT_OK;
}

/* ==============================================*/


ssize_t predict_header_length(const pkt_t *pkt){
    if(pkt == NULL){
		return -1;
	}
	ssize_t nbByteLength = varuint_predict_len(pkt->length);
	return 6 + nbByteLength;
}
