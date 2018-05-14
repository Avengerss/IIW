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
#define FILE_LEN 2450
#define DIM_PKT 64

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

void insert_pkt_and_flag(struct pkt *packet, char *buf, int pos){

	buf[pos] = 1;
	packet->flag =1;
}

int is_flagged(char *buff, int i){

	if(buff[i] == 1)
		return 1;
	else
		return 0;
}	

void unflag(char *buff, int pos){

	buff[pos]=0;
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

int try_send(int sockdesc, struct pkt *packet, struct sockaddr_in *sk, struct buf_data *data){
// funzione che spedisce solo se c'è spazio nella finestra di spedizione

    struct buf_data *d = data;
    int base = d->base;
    int end_wind = d->end_wind;
    int next_seq_num = d->next_seq_num;
    int seq_range_len = d->seq_range_len;

    if((end_wind > base) && (next_seq_num <= end_wind)) {  //se il nseq del prossimo pacchetto è dentro la finestra
    //invio
        if(sendto(sockdesc, packet, sizeof(*packet), 0,(struct sockaddr*) sk, sizeof(*sk)) <0){
            perror("errore in invio\n");
            return 1;
        }
        data -> next_seq_num = (data->next_seq_num +1) % seq_range_len; //incremento nseq del prossimo pacchetto da inv
        return 0;
    }

    if((end_wind < base) && (((base <= next_seq_num) && (next_seq_num <= (seq_range_len -1))) ||( (0<= next_seq_num) && (next_seq_num <= end_wind))) ){
    //il nseq del prox pacc è tra la base e l'ultimo nseq oppure tra il primo e la fine della finestra
        if(sendto(sockdesc, packet, sizeof(*packet), 0,(struct sockaddr*) sk, sizeof(*sk)) <0){
            perror("errore in invio\n");
            return 1;
        }
        data -> next_seq_num = (data->next_seq_num +1) % seq_range_len;
        return 0;
    }
    return 1;
}

int recv_ack(int *sockdesc, char *recvbuf, struct buf_data *data){
//funzione che prova in maniera non bloccante a ricevere un ack
    errno = 0;
    struct pkt packet;
    ssize_t result;
    if (((result = recvfrom(*sockdesc, &packet, sizeof(packet), 0, NULL, NULL))) < 0) {
        //perror("errore in recvfrom (forse per non bloccaggio)\n");
    }
    if(errno == EAGAIN || errno == EWOULDBLOCK){    //se non c'è nessun ack ritorno 0
        printf("non ho ricevuto ack\n");
        return 0;
    }

    int seq_num = packet.n_seq;    //se ho ricevuto l'ack aggiorno la finestra
    printf("ho ricevuto ack con numero sequenza %d\n", seq_num);
    int pos = accept_packet(seq_num, data, recvbuf, &packet);
    if(pos >= 0){
	move_and_write(pos, seq_num, data, recvbuf, NULL, 1);
    }
    return 1;
}

void continue_recv_ack(int *sockdesc, char *recvbuf, struct buf_data *data){
   
   
   while(data->base < data->next_seq_num){

       recv_ack(sockdesc, recvbuf, data);
   }

}

int main (void){

    //file ipotetico da mandare con la sua lunghezza
    int fileLen = FILE_LEN;
    int remain_fileLen =fileLen;

    struct sockaddr_in servaddr;    //indirizzo client a cui spedire (ottenuto nell'handshake)

    struct buf_data data;      //struttura con tutti gli indici di gestione buffer di invio
    
    int dim_pkt = DIM_PKT;
    char *send_buf = malloc(DIM_PKT*sizeof(char));    //buffer d'invio con tanti byte quanti pacchetti

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERV_PORT);
    if(inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr)<=0){     //indirizzo loopback
        perror("errore\n");
        exit(EXIT_FAILURE);
    }

    set_data(&data, BUFFER, WINDOW, NSEQ );

    int pkt_num = fileLen / dim_pkt;    //calcolo del numero di pacchetti da inviare data la dimensione del file e pkt
    int last_pkt = fileLen - (pkt_num*dim_pkt);
    if(last_pkt != 0)
        pkt_num ++;
    printf("pacchetti da inviare : %d\n", pkt_num);
    int sd;
    if((sd = socket(AF_INET, SOCK_DGRAM,0)) < 0){

        perror("errore creazione socket\n");
        exit(EXIT_FAILURE);
    }

    struct timeval read_timeout;
    read_timeout.tv_sec = 0;
    read_timeout.tv_usec = 10;
    setsockopt(sd , SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout);   //socket resa non bloccante in recv


    for(int i = 0; i< pkt_num; i++){    //ciclo che termina quando tutti i pacchetti sono stati inviati

        struct pkt packet;
        //packet->data = read_file();
        int n_seq = i % data.seq_range_len;     //numeri di sequenza assegnati progressivamente
        packet.n_seq = n_seq;
        //timer
        int j=1;
        while(j!=0){    //un dato pacchetto si prova a inviarlo finché non c'è spazio nella finestra
            j = try_send(sd, &packet, &servaddr, &data);
            int k =1;
            while(k != 0){     //dopo aver tentato l'invio, si ricevono gli ack finché ce ne sono
                k = recv_ack(&sd, send_buf, &data );
		
            }

        }


    }
    printf("ho finito di spedire! Ora ricevo: il nextseqnum è %d, l'edwind è %d \n",data.next_seq_num, data.end_wind);
    continue_recv_ack(&sd, send_buf, &data);
    printf("tutti i pacchetti sono stati riscontrati!");
    exit(EXIT_SUCCESS);
}
