
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h>

#include <unistd.h> 
#include <stdio.h> 
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <errno.h>

#define SERVPORT 5193 


struct packet {
   
   int size;
   short nseq;
   unsigned char msgType;
   unsigned char errorType;
   char data[];
   };

short randNseq(short maxNum){
   
	short random;
        random = rand() % (maxNum +1);
	return(random);
}


short get_connect(int sockfd, struct sockaddr_in *listenAddr, struct sockaddr_in *dedAddr){

	struct packet synPkt;
        struct packet synAckPkt;
        socklen_t len = sizeof(*dedAddr);
	struct timeval timeout;
	int tent = 0;
	timeout.tv_sec = 2;
	timeout.tv_usec = 0;
	int result;

	synPkt.msgType = 5;
	int nSeq = randNseq(65535);
	synPkt.nseq = nSeq;
        
	if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout)) <0){
		perror("options failed\n");
		exit(EXIT_FAILURE);	
	}

        while(tent < 4){

           if(sendto(sockfd, &synPkt, sizeof(synPkt), 0,(struct sockaddr*) listenAddr, sizeof(*listenAddr)) <0){
        perror("errore in invio\n");
        exit(EXIT_FAILURE);
       } 
	   errno = 0;
	   if (((result = recvfrom(sockfd, &synAckPkt, sizeof(synAckPkt), 0, (struct sockaddr *)dedAddr, &len))) < 0) {
                  perror("errore in recvfrom");
                  exit(EXIT_FAILURE);
            }
           else if(errno == EAGAIN || errno == EWOULDBLOCK){
               printf("timeout scaduto! Rispedisco\n");
               tent++;
           }
           else
               break;
           
  
        }
        if(tent == 4){
            return -1;
	}
        if((synAckPkt.msgType == 6)){
            return synAckPkt.nseq;
	}
        return -1;
}

int main(int argc, char *argv[]){
    
    int sockfd;
    struct sockaddr_in servaddr, dedaddr;
    short nSeq;
    
    
    if(argc < 2){
       perror("argv[1] usage : ip\n");
       exit(EXIT_FAILURE);
    }

    

    if((sockfd = socket(AF_INET, SOCK_DGRAM,0))<0){
        perror("errore in socket\n");
        exit(EXIT_FAILURE);
    }
    
    memset((void *)&servaddr, 0, sizeof(servaddr)); 
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERVPORT);
    if(inet_pton(AF_INET, argv[1], &servaddr.sin_addr)<=0){
        perror("errore\n");
        exit(EXIT_FAILURE);
    }
    
    
    if((nSeq = get_connect(sockfd, &servaddr, &dedaddr)) <0){
       perror("connessione non stabilita\n");
       exit(EXIT_FAILURE);
    }
    
    printf("il connect è andato a buon fine, in numero di sequenza è %d\n", nSeq);
    exit(EXIT_SUCCESS);
}



