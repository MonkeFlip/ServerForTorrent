#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>

#define BUF_SIZE 500

void sendPiece(int sockfd, int index,int begin,unsigned char* block,int length,struct sockaddr_storage peer_addr,socklen_t peer_addr_len);//index - index of file(should be index of piece), begin - offset whithin the piece, block - block of data(binary) from piece, length - length of block(in bytes)

int main(int argc, char *argv[])
{
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int sockfd, s;
    struct sockaddr_storage peer_addr;
    socklen_t peer_addr_len;
    ssize_t nread;
    unsigned char buf[BUF_SIZE];

    /*if (argc != 2) {
         fprintf(stderr, "Usage: %s port\n", argv[0]);
         exit(EXIT_FAILURE);
     }*/

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_DGRAM; /* Datagram socket */
    hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */
    hints.ai_protocol = 0;          /* Any protocol */
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    s = getaddrinfo(NULL, "6881", &hints, &result);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
    }

    /* getaddrinfo() returns a list of address structures.
        Try each address until we successfully bind(2).
        If socket(2) (or bind(2)) fails, we (close the socket
        and) try the next address. */
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sockfd = socket(rp->ai_family, rp->ai_socktype,
                     rp->ai_protocol);
        if (sockfd == -1)
            continue;

        if (bind(sockfd, rp->ai_addr, rp->ai_addrlen) == 0)
            break;                  /* Success */

        close(sockfd);
    }

    if (rp == NULL) {               /* No address succeeded */
        fprintf(stderr, "Could not bind\n");
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(result);           /* No longer needed */

    /* Read datagrams and echo them back to sender */
    FILE* file;
    //std::string filename="Bjarne Stroustrup - Programming Principles And Practice Using C++ 2nd Edition - 2014.pdf";
    unsigned long int position=0;
    int index=0;
    int index_mem=-1;
    int begin=0;
    int begin_buf=0;
    int length=0;
    int readBytesFromFile=0;
    unsigned char* blockBuffer;
    //file= fopen(filename.c_str(),"rb");
    std::string filename;
    std::ifstream fileList;
    //fileList.open("03d9a5ea768746a22140072dc6d5e0332d52efb1.txt");
    int counter=0;
    for (;;) {
        peer_addr_len = sizeof(struct sockaddr_storage);
        nread = recvfrom(sockfd, buf, BUF_SIZE, 0,
                         (struct sockaddr *) &peer_addr, &peer_addr_len);
        if (nread == -1)
        {
            continue;               /* Ignore failed request */
        }
        if(nread==68)
        {
            std::ostringstream os;
            os.fill('0');
            os<<std::hex;
            for(int i=28;i<48;i++)
            {
                os<<std::setw(2)<<(unsigned int)buf[i];
            }
            os<<".txt";
            fileList.open(os.str());
            std::cout<<os.str()<<std::endl;
            std::cout<<std::dec;
            continue;
            //std::cout<<"Received buffer: "<<buf<<std::endl;
        }

        char host[NI_MAXHOST], service[NI_MAXSERV];

        s = getnameinfo((struct sockaddr *) &peer_addr,
                        peer_addr_len, host, NI_MAXHOST,
                        service, NI_MAXSERV, NI_NUMERICSERV);

        index+=buf[5];
        index<<=24;
        index+=buf[6];
        index<<=16;
        index+=buf[7];
        index<<=8;
        index+=buf[8];
        //std::cout<<"Index: "<<index<<std::endl;
        begin_buf=buf[9];
        begin_buf<<=24;
        begin+=begin_buf;
        begin_buf=buf[10];
        begin_buf<<=16;
        begin+=begin_buf;
        begin_buf=buf[11];
        begin_buf<<=8;
        begin+=begin_buf;
        begin+=buf[12];

        length+=buf[13];
        length<<=24;
        length+=buf[14];
        length<<=16;
        length+=buf[15];
        length<<=8;
        length+=buf[16];

        if(index_mem!=index)
        {
            getline(fileList,filename);
        }
        //std::cout<<"filename: "<<filename<<std::endl;
        index_mem=index;
        blockBuffer=(unsigned char*)malloc(length);
        file= fopen(filename.c_str(),"rb");
        fseek(file,begin,SEEK_SET);
        readBytesFromFile=fread(blockBuffer,1,length,file);
        fclose(file);
        //std::cout<<readBytesFromFile<<std::endl;
        counter++;
        //std::cout<<counter<<std::endl;
        sendPiece(sockfd,index,begin,blockBuffer,readBytesFromFile,peer_addr,peer_addr_len);
        begin+=readBytesFromFile;
        index=0;
        begin=0;
        length=0;
        free(blockBuffer);
        /*if (s == 0)
            printf("Received %ld bytes from %s:%s\n",
                   (long) nread, host, service);
        else
            fprintf(stderr, "getnameinfo: %s\n", gai_strerror(s));

        if (sendto(sfd, buf, nread, 0,
                   (struct sockaddr *) &peer_addr,
                   peer_addr_len) != nread)
            fprintf(stderr, "Error sending response\n");*/
    }
    fileList.close();
    //fclose(file);
}

void sendPiece(int sockfd,int index,int begin,unsigned char* block,int length,struct sockaddr_storage peer_addr,socklen_t peer_addr_len)
{
    char* buffer=(char*)malloc(4+1+4+4+length);//len+id+index+begin+length of block
    int len=9+length;
    buffer[0]=len/1000;
    len=len-(len/1000)*1000;
    buffer[1]=len/100;
    len=len-(len/100)*100;
    buffer[2]=len/10;
    len=len-(len/10)*10;
    buffer[3]=len;
    buffer[4]=7;//id
    //index
    buffer[8]=index&0x000000ff;
    buffer[7]=index&0x0000ff00>>8;
    buffer[6]=index&0x00ff0000>>16;
    buffer[5]=index&0xff000000>>24;
    //begin
    buffer[12]=begin&0x000000ff;
    buffer[11]=(begin&0x0000ff00)>>8;
    buffer[10]=(begin&0x00ff0000)>>16;
    buffer[9]=(begin&0xff000000)>>24;
    //block from file
    for(int i=0;i<length;i++)
    {
        buffer[13+i]=block[i];
    }
    int fl=0;
    fl=sendto(sockfd, buffer, 4+1+4+4+length, 0,(struct sockaddr *) &peer_addr,peer_addr_len);
    free(buffer);
}

