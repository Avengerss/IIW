//
// Created by alessandro on 12/05/18.
//

//accept_packet è stata pensata in comune per sender e receiver, l'unica differenza sta nel possibile diverso tipo di buffer di ricezione e invio
//il valore di ritorno permette al sender e receiver di ricevere la posizione in cui è stato posizionato il pacchetto, per aggiornare la finestra, e  di inviare l'ack in caso la usi il receiver,
//move_and_write si differenzia per sender e receiver, perché il sender deve solo modificare gli indici, il receiver deve anche scrivere su file ad ogni movimento. Allora le ho separate

#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <arpa/inet.h>

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
    int type;
    char data[BUFSIZE];
};

//sender
//for(ogni pacchetto da mandare ){
//   ...send_pkt();
//  ...
//  pos = accept_packet(n_seq, &data, snd_buf);
//  move_window(pos, n_seq, &data, snd_buff);
//ecc.

//receiver
//pos = accept_packet(n_seq, &data, rcv_buff);
//if(pos > 0 || pos == -2)
//  send_ack(n_seq, ecc.);
//if(pos > 0)
//  move_and_write(fd, data_len, pos, n_seq, &data, rcv_buff);



//funzione usata dal receiver per inserire il pacchetto nel buffer di ricezione
void insert_pkt_and_flag(struct pkt *packet, struct pkt *buf, int pos) {
    buf[pos] = *packet;
    buf[pos].flag = 1;
}

//funzione usata dal sender per "inserire gli ack" in base a come vogliamo implementarli
//void insert_pkt_and_flag(...){}

ssize_t write_pkt(int fd, struct pkt pkt, size_t n)
{

    char *data = pkt.data;
    size_t nleft;
    ssize_t nwritten;
    char* ptr;

    ptr = data;
    nleft = n;
    while(nleft > 0){
        if((nwritten = write(fd, ptr, nleft)) <=0){
            if(( nwritten <0) && errno == EINTR) nwritten=0;
            else return(-1);
        }
        nleft -= nwritten;
        ptr += nwritten;
    }
    return (nleft);
}

//funzione usata dal sender per traslare la finestra e segnare l'arrivo dell'ack nel buffer
//il buffer dipende da come lo vogliamo fare, quindi la struttura dipende
void move_windoow(int pos, int seq,struct buf_data *data, struct sendbuff *buff ){

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
        while(buff[i].flag == 1){
            buff[i].flag =0;


            printf("il pacchetto con n_seq %d è stato consegnato\n", seq);

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

}

//funzione usata dal receiver che oltre a far scorrere la finestra, scrive i dati sul file
void move_and_write(int filedescr, int size_to_write, int pos, int seq, struct buf_data *data, struct pkt *buff){

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
        while(buff[i].flag == 1){
            buff[i].flag = 0;
            write_on_file(filedescr, buff[i], size_to_write);


            printf("il pacchetto con n_seq %d è stato consegnato\n", seq);

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

//il controllo di campi del pacchetto alla ricerca di eventuali errori non fa parte di questa funzione
//dovendo funzionare sia per il send buffer che per il receive, la funzione torna un int che è la posizione del pacchetto nel buffer, oppure -2 se è una ritrasmissione o -1 se è fuori sequenza
// la posizione tornata verrà usata per lo scorrimento della finestra e eventuale scrittura del campo dati
//nel caso il ricevente che deve mandare gli ack di riscontro deve discernere il -1 dal -2
//il mittent che la usa per ricevere gli ack non interessa il valore di ritorno
//mittente e destinatario avranno diverse implementazioni di insert_pkt_and_flag
int accept_packet(int seq, struct buf_data *data, void *buff){

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
            //insert_pkt_and_flag(buff);

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

            //insert_pkt_and_flag(buff);

            printf("pacchetto inserito in posizione %d\n", pos_pkt);
            result = pos_pkt;

        }

        if((base <= seq) && (seq < seq_range_len)){

            //printf("il numero di sequenza è tra l'inizio e lo zero");

            pos_pkt = (seq + pos_0_prev) % buf_len;

            printf("pacchetto inserito in posizione %d\n", pos_pkt);

            //insert_pkt_and_flag(buff);

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