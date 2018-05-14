#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <arpa/inet.h>

#define BUFFER 12
#define WINDOW   7
#define NSEQ 17
#define SERV_PORT 2345

struct buf_data {
    
    int buf_len;
    int N;
    int pos_0;
    int pos_0_prev;
    int base;
    int next_seq_num;
    int end_wind;
    int base_prev;
    int end_wind_prev;
    int seq_range_len;

};

struct pkt {

    int n_seq;
    int flag;
};

int isReceiver(int n){

    if(n == 2)
	return 1;
    else
	return 0;
}

void write_on_file(int seq){

	printf("il pacchetto con n_seq %d è stato consegnato\n", seq);
}

void insert_pkt_and_flag(struct pkt *packet, struct pkt *buf, int pos){
	buf[pos] = *packet;
        buf[pos].flag = 1;
}

int is_flagged(struct pkt *buf, int pos){

	if(buf[pos].flag == 1)
		return 1;
	else
		return 0;
}

void unflag(struct pkt *buf, int pos){
	
	buf[pos].flag = 0;
}

void set_data(struct buf_data *data,int bf_ln, int N,int sq_rng_ln){
    
    data-> buf_len = bf_ln;
    data->N = N;
    data->seq_range_len = sq_rng_ln;
    data->pos_0 = 0;
    data->pos_0_prev = data->pos_0;
    data->base = 0;
    data->next_seq_num = 0;
    data->end_wind = (data-> base + N - 1) % sq_rng_ln;
    data->base_prev = data->base;
    data->end_wind_prev= data->end_wind;

}

struct pkt getPkt(int *sockdesc, struct sockaddr_in *cliaddr){

    struct pkt packet;
    ssize_t result;
    socklen_t len = sizeof(*cliaddr);
    if (((result = recvfrom(*sockdesc, &packet, sizeof(packet), 0,(struct sockaddr*) cliaddr, &len))) < 0) {
        perror("errore in recvfrom");
        exit(EXIT_FAILURE);
    }
    return packet;
}

void send_ack( int *sockdesc, int nseq, struct sockaddr_in *addr){

    struct pkt packet;
    packet.n_seq = nseq;

    if(sendto(*sockdesc, &packet, sizeof(packet), 0,(struct sockaddr*) addr, sizeof(*addr)) <0){
        perror("errore in invio\n");
        exit(EXIT_FAILURE);
    }
}


int accept_packet(int seq, struct buf_data *data, void *buff, struct pkt *packet){

    int buf_len = data-> buf_len;
    int N = data -> N;
    int pos_0 = data -> pos_0;
    int pos_0_prev = data -> pos_0_prev;
    int base = data -> base;
    int end_wind = data -> end_wind;
    int seq_range_len = data -> seq_range_len;

    int pos_pkt;
    int result;

    if( end_wind > base){

        if((base <= seq) && (seq <= end_wind)){

            pos_pkt = (seq + pos_0) % buf_len;
            insert_pkt_and_flag(packet, buff, pos_pkt);

            printf("pacchetto inserito in posizione %d\n", pos_pkt);
            result = pos_pkt;

        }
    }

    else if(end_wind < base){

        printf("la recv_base %d è maggiore di end_wind %d\n", base, end_wind); //stam
        // numero di sequenza tra lo zero e la fine
        if((seq >= 0) && (seq <= end_wind)){

            //printf("il numero di sequenza è tra lo zero e end_wind\n");

            pos_pkt = (seq + pos_0) % buf_len;

            insert_pkt_and_flag(packet, buff, pos_pkt);

            printf("pacchetto inserito in posizione %d\n", pos_pkt);
            result = pos_pkt;

        }

        if((base <= seq) && (seq < seq_range_len)){

            //printf("il numero di sequenza è tra l'inizio e lo zero");

            pos_pkt = (seq + pos_0_prev) % buf_len;

            printf("pacchetto inserito in posizione %d\n", pos_pkt);

            insert_pkt_and_flag(packet, buff, pos_pkt);

            result = pos_pkt;
        }

    }

    else if( (seq >= base - N) && (seq < base) ){

        result = -2;
    }

    else
        result = -1;


    return result;

}


void move_and_write(int pos, int seq, struct buf_data *data, void *buff, char *filepath, int sndorrcv){

    int buf_len = data-> buf_len;
    int N = data -> N;
    int pos_0 = data -> pos_0;
    int pos_0_prev = data -> pos_0_prev;
    int base = data -> base;
    int end_wind = data -> end_wind;
    int base_prev = data -> base_prev;
    int end_wind_prev = data -> end_wind_prev;
    int seq_range_len = data -> seq_range_len;

    int pos_pkt = pos;
    

    if( seq == base){
        int i = pos_pkt;
        while(is_flagged(buff, i)){
            unflag(buff, i);
	    

            if(isReceiver(sndorrcv)){

                //write_on_file(buff[i], filepath);
		write_on_file(seq);
            }

            //printf("il pacchetto con n_seq %d è stato consegnato\n", seq);

            base_prev = base;
            end_wind_prev = end_wind;
            base= (base +1) % seq_range_len;
            end_wind= (base + N -1) % seq_range_len;
            printf("nuova base %d, nuova fine %d\n", base, end_wind); //stampa
            if(end_wind > base){
                //printf("end_wind non ha ricominciato\n");
            }


            if((end_wind < base) && (end_wind_prev > base_prev)){
                //printf("end_wind è ricominciato! Ricalcolo posizione del nuovo zero\n");

                pos_0_prev = pos_0;
                pos_0 = pos_0 + (seq_range_len % buf_len);
            }
            i = (i + 1) % buf_len;
        }
    }

    data -> pos_0 = pos_0;
    data -> pos_0_prev = pos_0_prev;
    data -> base = base;
    data -> end_wind = end_wind;
    data -> base_prev = base_prev;
    data -> end_wind_prev = end_wind_prev;
}

int main(void){

    struct buf_data data;
    set_data(&data, BUFFER, WINDOW, NSEQ);

    struct pkt recvBuffer[BUFFER];
    struct sockaddr_in addr;
    struct sockaddr_in cliaddr;
    int sd;
    if((sd = socket(AF_INET, SOCK_DGRAM,0)) < 0){

        perror("errore creazione socket\n");
        exit(EXIT_FAILURE);
    }

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY); /* il server accetta pacchetti su una qualunque delle sue interfacce di rete */
    addr.sin_port = htons(SERV_PORT); /* numero di porta del server */

    if (bind(sd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("errore in bind");
        exit(1);
    }

    while(1){

        //struct pkt *arr_pkt = malloc(sizeof(struct pkt));
        //*arr_pkt = getPkt(&sd, &cliaddr);
	struct pkt arr_pkt;
	arr_pkt = getPkt(&sd, &cliaddr);
        int pos_pkt;
        int n_seq = arr_pkt.n_seq;

        printf("ricevuto pacchetto con numero sequenza %d\n", n_seq);
    
        pos_pkt = accept_packet(n_seq, &data,(struct pkt*) recvBuffer, &arr_pkt);
	printf("la posizione tornata è %d\n", pos_pkt);
        if(pos_pkt >= 0 || pos_pkt == -2){
            send_ack(&sd, n_seq, &cliaddr);
	}

	if(pos_pkt >= 0){
	    move_and_write(pos_pkt, n_seq, &data, (struct pkt*) recvBuffer, "path", 2);
	}
    }
}
	    
