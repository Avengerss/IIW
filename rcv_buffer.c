#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define WINDOW   7
#define NSEQ 17

struct pkt {
	
	int n_seq;
	int flag;
};

struct pkt *getPkt(){

	struct pkt *packet = malloc(sizeof(struct pkt));
	scanf("%d", &packet->n_seq);
	return packet;
}





int main(void){

	int N = WINDOW;
	int seq_range_len = NSEQ;
	int recv_base = 0;
	int end_wind = N - 1;
	int pos_0 = 0;
	int pos_0_prev = pos_0;
	int recv_base_prev = recv_base;
	int end_wind_prev = end_wind;

	struct pkt recvBuffer[N];


	while(1){

		struct pkt *arr_pkt;
		arr_pkt = getPkt();
		int pos_pkt;
		int n_seq = arr_pkt->n_seq;
		// 1° caso
		if(end_wind > recv_base){
			
			printf("la recv_base %d è minore di end_wind %d\n", recv_base, end_wind); //stampa
			
			if( (n_seq > recv_base) && (n_seq <= end_wind)){
				
				printf("il n_seq non è uguale a recv_base: la finestra non scorre\n"); //stampa

				pos_pkt = (n_seq + pos_0 ) % N;
				recvBuffer[pos_pkt] = *arr_pkt;
				recvBuffer[pos_pkt].flag = 1;
 
				printf("pacchetto inserito in posizione %d\n", pos_pkt);
			}

			if( n_seq == recv_base){

				pos_pkt = (n_seq + pos_0 ) % N;
				recvBuffer[pos_pkt] = *arr_pkt;
				recvBuffer[pos_pkt].flag = 1;

				printf("il n_seq è uguale alla base: la finestra scorre\n");

				int i = pos_pkt;
				printf("la pos_pkt è %d\n", i);
				while(recvBuffer[i].flag == 1){
					recvBuffer[i].flag = 0;

					printf("il pacchetto con n_seq %d è stato tolto dal buffer e consegnato\n", n_seq);

					recv_base_prev = recv_base;
					end_wind_prev = end_wind;
					recv_base= (recv_base +1) % seq_range_len;
					end_wind= (recv_base + N -1) % seq_range_len;

					printf("nuova base %d, nuova fine %d\n", recv_base, end_wind); //stampa

					if(end_wind > recv_base){
						printf("end_wind non ha ricominciato\n");
					}

					if((end_wind < recv_base) && (end_wind_prev > recv_base_prev)){
						printf("end_wind è ricominciato! Ricalcolo posizione del nuovo zero\n");

						pos_0_prev = pos_0;
						pos_0 = pos_0 + (seq_range_len % N);
					}	
					
					i = (i + 1) % N;
					
				}
			}
				
                }
		// 2° caso
		else if(end_wind < recv_base){

			printf("la recv_base %d è maggiore di end_wind %d\n", recv_base, end_wind); //stam			
                        // numero di sequenza tra lo zero e la fine 
			if((n_seq > 0) && (n_seq <= end_wind)){

				printf("il numero di sequenza è tra lo zero e end_wind\n");

				pos_pkt = (n_seq + pos_0) % N;
		                recvBuffer[pos_pkt] = *arr_pkt;
				recvBuffer[pos_pkt].flag = 1;

				printf("pacchetto inserito in posizione %d\n", pos_pkt);
			}

			if((recv_base <= n_seq) && (n_seq < seq_range_len)){   

				printf("il numero di sequenza è tra l'inizio e lo zero");
				
				pos_pkt = (n_seq + pos_0_prev) % N;
				recvBuffer[pos_pkt] = *arr_pkt;
				recvBuffer[pos_pkt].flag = 1;

				printf("pacchetto inserito in posizione %d\n", pos_pkt);

				if( n_seq == recv_base){
					int i = pos_pkt;
					while(recvBuffer[i].flag == 1){
						recvBuffer[i].flag = 0;
						printf("il pacchetto con n_seq %d è stato consegnato\n", n_seq);
						
						recv_base_prev = recv_base;
						end_wind_prev = end_wind;
						recv_base= (recv_base +1) % seq_range_len;
						end_wind= (recv_base + N -1) % seq_range_len;
						printf("nuova base %d, nuova fine %d\n", recv_base, end_wind); //stampa
						if(end_wind > recv_base){
							printf("end_wind non ha ricominciato\n");
						}

					
						if((end_wind < recv_base) && (end_wind_prev > recv_base_prev)){
							printf("end_wind è ricominciato! Ricalcolo posizione del nuovo zero\n");

							pos_0_prev = pos_0;
							pos_0 = pos_0 + (seq_range_len % N);
						}	
						i = (i + 1) % N;
					}
				}
			}
			if(n_seq == 0)	{
				pos_pkt = pos_0;
				recvBuffer[pos_pkt] = *arr_pkt;
				recvBuffer[pos_pkt].flag = 1;
			}

		}				

		
	}	
		return 1;
}
