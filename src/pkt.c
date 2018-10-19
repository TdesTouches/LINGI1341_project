#include "pkt.h"

/* Extra #includes */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <zlib.h>
#include <netinet/in.h>

#include "utils.h"

struct __attribute__((__packed__)) pkt {
	struct header{
		uint8_t window 	:5; // [0-31]
		uint8_t tr 		:1; // [0-1]
		ptypes_t ptype 	:2;
		uint8_t seqnum; // [0-255]
		uint16_t length; // [0-512] - network byte order
		uint32_t timestamp; //
		uint32_t crc1; //
	} header;
	char *payload;
	uint32_t crc2; //
};

uint32_t pkt_gen_crc2(const pkt_t *pkt){
	fprintf(stderr, "Generate crc2\n");
	uLong crc = crc32(0L, Z_NULL, 0);
	crc = crc32(crc, (Bytef*) pkt_get_payload(pkt), pkt_get_length(pkt));
	return htonl((uint32_t) crc);
}

uint32_t pkt_gen_crc1(const pkt_t *pkt){
	fprintf(stderr, "Generate crc1\n");
	uLong crc = crc32(0L, Z_NULL, 0);
	uint8_t tr = pkt_get_tr(pkt);
	pkt_set_tr((pkt_t*) pkt, 0);
	crc = crc32(crc, (Bytef*) pkt, sizeof(pkt->header) - sizeof(uint32_t));
	pkt_set_tr((pkt_t*) pkt, tr);
	return htonl((uint32_t) crc);
}

/* Extra code */
/* Your code will be inserted here */

pkt_t* pkt_new(){
	fprintf(stderr, "Creating new pkt\n");
	pkt_t *pkt = (pkt_t*) calloc(1, sizeof(pkt_t));
	return pkt;
}

void pkt_del(pkt_t *pkt){
	fprintf(stderr, "deleting pkt\n");
	if(pkt->payload != NULL){
		free(pkt->payload);
	}
    free(pkt);
}

pkt_status_code pkt_decode(const char *data, const size_t len, pkt_t *pkt){
	fprintf(stderr, "Decoding packet\n");
	uint32_t counter = sizeof(pkt->header);
	// cpy header
	memcpy(pkt, data, sizeof(pkt->header));

	fprintf(stderr, "pkt get length : %d\n", pkt_get_length(pkt));

	if(len < sizeof(pkt_t)+pkt_get_length(pkt)-sizeof(char*)){
		return E_NOMEM;
	}

	// cpy payload
	if(!pkt_get_tr(pkt) && pkt_get_length(pkt) > 0){
		pkt_set_payload(pkt, data+counter, pkt_get_length(pkt));
		counter += pkt_get_length(pkt);
		memcpy(&pkt->crc2, data+counter, sizeof(uint32_t));
		counter += sizeof(uint32_t);
	}

	// check crc1
	if(pkt_get_crc1(pkt) != pkt_gen_crc1(pkt)){
		return E_CRC;
	}
	// check crc2
	if(!pkt_get_tr(pkt) && pkt_get_crc2(pkt) != pkt_gen_crc2(pkt)){
		return E_CRC;
	}

	return PKT_OK;
}

pkt_status_code pkt_encode(const pkt_t* pkt, char *buf, size_t *len){
	fprintf(stderr, "Encoding packet\n");
	if(*len < pkt_get_length(pkt) + sizeof(pkt_t) - sizeof(pkt->payload)){
	 	return E_NOMEM;
	}
	
	size_t counter = sizeof(pkt->header) - sizeof(uint32_t);
	// Encode header without crc1
	memcpy(buf, pkt, counter);

	// encode crc1
	uint32_t crc = pkt_gen_crc1(pkt);
	memcpy(buf+counter, &crc, sizeof(uint32_t));
	counter += sizeof(uint32_t);

	// encode payload
	if(!pkt_get_tr(pkt)){
		memcpy(buf+counter, pkt->payload, pkt_get_length(pkt));
		counter += pkt_get_length(pkt);
	}

	// encode crc2
	if(!pkt_get_tr(pkt)){
		crc = pkt_gen_crc2(pkt);
		memcpy(buf+counter, &crc, sizeof(uint32_t));
		counter += sizeof(uint32_t);
	}

	*len = counter;
	return PKT_OK;
}

ptypes_t pkt_get_type  (const pkt_t* pkt){
	fprintf(stderr, "get type\n");
	return pkt->header.ptype;
}

uint8_t  pkt_get_tr(const pkt_t* pkt){
	fprintf(stderr, "get tr\n");
	return pkt->header.tr;
}

uint8_t  pkt_get_window(const pkt_t* pkt){
	fprintf(stderr, "get window\n");
	return pkt->header.window;
}

uint8_t  pkt_get_seqnum(const pkt_t* pkt){
	fprintf(stderr, "get seqnum\n");
	return pkt->header.seqnum;
}

uint16_t pkt_get_length(const pkt_t* pkt){
	fprintf(stderr, "get length\n");
	return ntohs(pkt->header.length);
}

uint32_t pkt_get_timestamp   (const pkt_t* pkt){
	fprintf(stderr, "get timestamp\n");
	return pkt->header.timestamp;
}

uint32_t pkt_get_crc1   (const pkt_t* pkt){
	fprintf(stderr, "get crc1\n");
	return pkt->header.crc1;
}

uint32_t pkt_get_crc2   (const pkt_t* pkt){
	fprintf(stderr, "get crc2\n");
	return pkt->crc2;
}

const char* pkt_get_payload(const pkt_t* pkt){
	fprintf(stderr, "get payload\n");
	return pkt->payload;
}


pkt_status_code pkt_set_type(pkt_t *pkt, const ptypes_t type){
	fprintf(stderr, "set type\n");
	if(type != PTYPE_DATA && type != PTYPE_ACK && type != PTYPE_NACK){
		fprintf(stderr, "bad type\n");
		return E_TYPE;
	}
	pkt->header.ptype = type;
	return PKT_OK;
}

pkt_status_code pkt_set_tr(pkt_t *pkt, const uint8_t tr){
	fprintf(stderr, "set tr\n");
	if(tr != 0 && tr != 1){
		fprintf(stderr, "bad tr\n");
		return E_TR;
	}
	pkt->header.tr = tr;
	return PKT_OK;
}

pkt_status_code pkt_set_window(pkt_t *pkt, const uint8_t window){
	fprintf(stderr, "set window\n");
	if(window > MAX_WINDOW_SIZE){
		fprintf(stderr, "bad window\n");
		return E_WINDOW;
	}
	pkt->header.window = window;
	return PKT_OK;
}

pkt_status_code pkt_set_seqnum(pkt_t *pkt, const uint8_t seqnum){
	fprintf(stderr, "set seqnum\n");
	pkt->header.seqnum = seqnum;
	return PKT_OK;
}

pkt_status_code pkt_set_length(pkt_t *pkt, const uint16_t length){
	fprintf(stderr, "set length\n");
	if(length > MAX_PAYLOAD_SIZE){
		fprintf(stderr, "bad length\n");
		return E_LENGTH;
	}
	pkt->header.length = htons(length);
	return PKT_OK;
}

pkt_status_code pkt_set_timestamp(pkt_t *pkt, const uint32_t timestamp){
	fprintf(stderr, "set timestamp\n");
	pkt->header.timestamp = timestamp;
	return PKT_OK;
}

pkt_status_code pkt_set_crc1(pkt_t *pkt, const uint32_t crc1){
	fprintf(stderr, "set crc1\n");
	pkt->header.crc1 = crc1;
	return PKT_OK;
}

pkt_status_code pkt_set_crc2(pkt_t *pkt, const uint32_t crc2){
	fprintf(stderr, "set crc2\n");
	pkt->crc2 = crc2;
	return PKT_OK;
}

pkt_status_code pkt_set_payload(pkt_t *pkt,
							    const char *data,
								const uint16_t length){
	fprintf(stderr, "set payload\n");
	if(data==NULL){
		pkt_set_length(pkt, 0);
		pkt->payload = NULL;
		fprintf(stderr, "no data \n");
		return PKT_OK;
	}

	pkt_status_code status = pkt_set_length(pkt, length);
	if(status != PKT_OK){
		fprintf(stderr, "length problem\n");
		return status;
	}
	if(pkt->payload == NULL){
		pkt->payload = (char*) malloc(length);
	}else{
		pkt->payload = (char*) realloc((void*) pkt->payload, length);
	}

	if(pkt->payload==NULL){
		fprintf(stderr, "no mem\n");
		return E_NOMEM;
	}
	memcpy(pkt->payload, data, length);
	return PKT_OK;
}

pkt_status_code pkt_update_timestamp(pkt_t* pkt){
	uint32_t timestamp = get_time();
	pkt_status_code status = pkt_set_timestamp(pkt, timestamp);
	if(status != PKT_OK){
		return status;
	}

	status = pkt_set_crc1(pkt, pkt_get_crc1(pkt));
	return status;
}

int pkt_compare_timestamp(pkt_t* pkt1, pkt_t* pkt2){
	uint32_t ts1 = pkt_get_timestamp(pkt1);
	uint32_t ts2 = pkt_get_timestamp(pkt2);
	return ts1==ts2;
}

int pkt_timestamp_outdated(pkt_t* pkt, uint32_t RTT){
	uint32_t ts = pkt_get_timestamp(pkt);
	uint32_t ct = get_time();
	return ct - ts > RTT;
}


pkt_status_code pkt_create(	pkt_t* pkt,
							uint8_t seqnum,
							uint8_t window,
							ptypes_t type){
	if(type != PTYPE_ACK || type != PTYPE_NACK){
		fprintf(stderr, "Bad use of pkt_create function (only ack or nack)\n");
		exit(-1);
	}
	pkt_set_type(pkt, type);
	pkt_set_window(pkt, window);
	pkt_set_tr(pkt, 0);
	pkt_set_payload(pkt, NULL, 0);
	pkt_set_seqnum(pkt, seqnum);
	pkt_set_timestamp(pkt, get_time());
	pkt_set_crc1(pkt, pkt_gen_crc1(pkt));
	pkt_set_crc2(pkt, pkt_gen_crc2(pkt));
	return PKT_OK;
}

const char* pkt_get_error(pkt_status_code status){
	switch(status){
		case(PKT_OK):
			return "PKT_OK";
			break;
		case(E_TYPE):
			return "E_TYPE";
			break;
		case(E_TR):
			return "E_TR";
			break;
		case(E_LENGTH):
			return "E_LENGTH";
			break;
		case(E_CRC):
			return "E_CRC";
			break;
		case(E_WINDOW):
			return "E_WINDOW";
			break;
		case(E_SEQNUM):
			return "E_SEQNUM";
			break;
		case(E_NOMEM):
			return "E_NOMEM";
			break;
		case(E_NOHEADER):
			return "E_NOHEADER";
			break;
		case(E_UNCONSISTENT):
			return "E_UNCONSISTENT";
			break;
		default:
			return "Unknown status code";
			break;
	}
}


void pkt_check_error(const char* msg, pkt_status_code status){
	if(status != PKT_OK){
		fprintf(stderr, "%s : %s\n", msg, pkt_get_error(status));
	}
}