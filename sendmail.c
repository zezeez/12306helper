#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <sys/types.h>  
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <errno.h>  
#include <unistd.h>  
#include <sys/time.h>  
#include <netdb.h>  
  
  
#define TEST_IMG     ("test.png")  
static char base64_table[64] =  
{  
    'A', 'B', 'C', 'D', 'E', 'F', 'G',  
    'H', 'I', 'J', 'K', 'L', 'M', 'N',  
    'O', 'P', 'Q', 'R', 'S', 'T',  
    'U', 'V', 'W', 'X', 'Y', 'Z',  
    'a', 'b', 'c', 'd', 'e', 'f', 'g',  
    'h', 'i', 'j', 'k', 'l', 'm', 'n',  
    'o', 'p', 'q', 'r', 's', 't',  
    'u', 'v', 'w', 'x', 'y', 'z',  
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',  
    '+', '/'  
};  
  
  
int base64_encode(unsigned char *buf, int nLen, char *pOutBuf, int nBufSize);  
  
void sendemail(char *stmpServer, char *body);  
int open_socket(struct sockaddr *addr);  
  
int main()  
{  
    FILE * fp = NULL;  
    char buf[128] = {0};  
    char outbuf[256] = {0};  
    char *body = NULL;  
    char email[] = "smtp.163.com";  
#if 0  
    char body[] = "From: \"********\"<****@qq.com>\r\n"  
        "To:  \"horst\"<>\r\n"  
        "Subject: Hello\r\n\r\n"  
        "this is a test!\n"  
        "good bye!"  
        "MIME-Version:1.0\r\n"  
        "Content-Type:multipart/mixed;boundary=\"---=_NextPart_000_0050_01C\"\r\n"  
        "Subject:=?gb2312?B?TU1NRdCt0unLtcP308q8/g==?=\r\n";  
#endif  
    char bodyHead[] = "From: \"gxzpljj\"<gxzpljj@163.com>\r\n"  
        "To:  \"gxmgljj\"<gxmgljj@163.com>\r\n"  
        "MIME-Version:1.0\r\n"  
        "Content-Type:multipart/mixed;boundary=\"---=_NextPart_000_0050_01C\"\r\n"  
        "Subject:test\r\n"  
        "\r\n"  
        "-----=_NextPart_000_0050_01C\r\n"  
        "Content-Type:text/plain;charset=\"gb2312\"\r\n"  
        "\r\n"  
        "hello \r\n"  
        "-----=_NextPart_000_0050_01C\r\n"  
        "\r\n\r\n"  
        "Content-Type:application/octet-stream;name=\"test.png\"\r\n"  
        "Content-Transfer-Encoding:base64\r\n"  
        "Content-Disposion:attachment;filename=\"test.png\""  
        "\r\n\r\n";  
          
    char bodyend[] = "\r\n-----=_NextPart_000_0050_01C\r\n";  
  
    body = malloc(8 * 1024 * 1024);  
    if(NULL == body)  
    {  
        perror("malloc(): ");  
        return -1;  
    }  
  
    memset(body, 0x0, 8*1024*1024);  
  
    sprintf(body, bodyHead);  
      
    fp = fopen(TEST_IMG, "rb");  
    if(fp == NULL)  
    {  
        perror("fopen(): ");  
        return -1;  
    }  
  
    memset(buf, 0x0, 128);  
    memset(outbuf, 0x0, 256);  
    int len = 0;  
    char *ptr = NULL;  
    ptr = body + strlen(body);  
    while((len = fread(buf, sizeof(char), 60, fp)) > 0)  
    {  
        base64_encode(buf, len, outbuf, sizeof(outbuf));  
        len = sprintf(outbuf, "%s\r\n", outbuf);  
        printf("len :  %d\n", len);  
        sprintf(ptr, outbuf);  
        memset(buf, 0x0, 128);  
        memset(outbuf, 0x0, 256);  
        ptr += len;  
    }  
  
    sprintf(ptr, "\r\n\r\n");  
    ptr += strlen("\r\n\r\n");  
    sprintf(ptr, bodyend);  
  
    sendemail(email, body);  
  
    fclose(fp);  
    free(body);  
  
    return 0;  
}  
  
int base64_encode(unsigned char* pBase64, int nLen, char* pOutBuf, int nBufSize)  
{  
    int i = 0;  
    int j = 0;  
    int nOutStrLen = 0;  
  
    /* nOutStrLen does not contain null terminator. */  
    nOutStrLen = nLen / 3 * 4 + (0 == (nLen % 3) ? 0 : 4);  
    if ( pOutBuf && nOutStrLen < nBufSize )  
    {  
        char cTmp = 0;  
        for ( i = 0, j = 0; i < nLen; i += 3, j += 4 )  
        {  
            /* the first character: from the first byte. */  
            pOutBuf[j] = base64_table[pBase64[i] >> 2];  
  
            /* the second character: from the first & second byte. */  
            cTmp = (char)((pBase64[i] & 0x3) << 4);  
            if ( i + 1 < nLen )  
            {  
                cTmp |= ((pBase64[i + 1] & 0xf0) >> 4);  
            }  
            pOutBuf[j+1] = base64_table[(int)cTmp];  
  
            /* the third character: from the second & third byte. */  
            cTmp = '=';  
            if ( i + 1 < nLen )  
            {  
                cTmp = (char)((pBase64[i + 1] & 0xf) << 2);  
                if ( i + 2 < nLen )  
                {  
                    cTmp |= (pBase64[i + 2] >> 6);  
                }  
                cTmp = base64_table[(int)cTmp];  
            }  
            pOutBuf[j + 2] = cTmp;  
  
            /* the fourth character: from the third byte. */  
            cTmp = '=';  
            if ( i + 2 < nLen )  
            {  
                cTmp = base64_table[pBase64[i + 2] & 0x3f];  
            }  
            pOutBuf[j + 3] = cTmp;  
        }  
  
        pOutBuf[j] = '\0';  
    }  
  
    return nOutStrLen + 1;  
}  
  
/**  
 * @func  sendemail 
 * @brief send email in blocking-mode 
 * @param smtpServer  
 * @param body  
 */  
void sendemail(char *smtpServer, char *body)  
{  
    int sockfd = 0;  
    struct sockaddr_in their_addr = {0};  
    char buf[1500] = {0};  
    char rbuf[1500] = {0};  
    char login[128] = {0};  
    char pass[128] = {0};  
    struct hostent *host = NULL;  
  
    if((host = gethostbyname(smtpServer))==NULL)/*取得主机IP地址*/  
    {  
        fprintf(stderr,"Gethostname error, %s\n", strerror(errno));  
        exit(1);  
    }  
  
  
    memset(&their_addr, 0, sizeof(their_addr));  
    their_addr.sin_family = AF_INET;  
    their_addr.sin_port = htons(25);  
    their_addr.sin_addr = *((struct in_addr *)host->h_addr);  
  
    sockfd = open_socket((struct sockaddr *)&their_addr);  
  
    memset(rbuf, 0, 1500);  
  
    while(recv(sockfd, rbuf, 1500, 0) == 0)  
    {  
        printf("reconnect..\n");  
  
        close(sockfd);  
        sleep(2);  
        sockfd = open_socket((struct sockaddr *)&their_addr);  
  
        memset(rbuf, 0, 1500);  
    }  
  
    printf("%s\n", rbuf);  
  
    /* EHLO */  
  
    memset(buf, 0, 1500);  
    sprintf(buf, "EHLO abcdefg-PC\r\n");  
  
    send(sockfd, buf, strlen(buf), 0);  
    memset(rbuf, 0, 1500);  
  
    recv(sockfd, rbuf, 1500, 0);  
  
    printf("%s\n", rbuf);  
  
    /*AUTH LOGIN  */  
    memset(buf, 0, 1500);  
    sprintf(buf, "AUTH LOGIN\r\n");  
    send(sockfd, buf, strlen(buf), 0);  
  
    printf("%s\n", buf);  
  
    memset(rbuf, 0, 1500);  
  
    recv(sockfd, rbuf, 1500, 0);  
  
    printf("%s\n", rbuf);  
  
    /* USER */  
    memset(buf, 0, 1500);  
  
    sprintf(buf, "gxzpljj@163.com");  
    memset(login, 0, 128);  
  
    base64_encode(buf, strlen(buf), login, 128);                   /* base64 */  
  
    sprintf(buf, "%s\r\n", login);  
    send(sockfd, buf, strlen(buf), 0);  
    printf("%s\n", buf);  
  
    memset(rbuf, 0, 1500);  
  
    recv(sockfd, rbuf, 1500, 0);  
    printf("%s\n", rbuf);  
  
    /* PASSWORD */  
    memset(buf, 0, 1500);  
    sprintf(buf, "19940723gx");  
    memset(pass, 0, 128);  
  
    base64_encode(buf, strlen(buf), pass, 128);  
    memset(buf, 0, 1500);  
    sprintf(buf, "%s\r\n", pass);  
    send(sockfd, buf, strlen(buf), 0);  
  
    printf("%s, %d\n", buf, __LINE__);  
  
    memset(rbuf, 0, 1500);  
    recv(sockfd, rbuf, 1500, 0);  
    printf("%s, %d\n", rbuf, __LINE__);  
  
    /* MAIL FROM */  
  
    memset(buf, 0, 1500);  
    sprintf(buf, "MAIL FROM: <gxzpljj@163.com>\r\n");  
    send(sockfd, buf, strlen(buf), 0);  
  
    memset(rbuf, 0, 1500);  
    recv(sockfd, rbuf, 1500, 0);  
  
    printf("%s\n", rbuf);  
  
    /* rcpt to 第一个收件人 */  
    sprintf(buf, "RCPT TO:<gxmgljj@163.com>\r\n");  
    send(sockfd, buf, strlen(buf), 0);  
  
    memset(rbuf, 0, 1500);  
    recv(sockfd, rbuf, 1500, 0);  
  
    printf("%s\n", rbuf);  
  
    /* DATA email connext ready  */  
  
    sprintf(buf, "DATA\r\n");  
    send(sockfd, buf, strlen(buf), 0);  
  
    memset(rbuf, 0, 1500);  
    recv(sockfd, rbuf, 1500, 0);  
    printf("%s\n", rbuf);  
  
    /* send email connext \r\n.\r\n end*/  
  
    sprintf(buf, "%s\r\n.\r\n", body);  
  
    send(sockfd, buf, strlen(buf), 0);  
  
    memset(rbuf, 0, 1500);  
  
    recv(sockfd, rbuf, 1500, 0);  
    printf("%s\n", rbuf);  
  
    /* QUIT */  
  
    sprintf(buf, "QUIT\r\n");  
    send(sockfd, buf, strlen(buf), 0);  
    memset(rbuf, 0, 1500);  
  
    recv(sockfd, rbuf, 1500, 0);  
    printf("%s\n", rbuf);  
  
    return ;  
  
}  
  
int open_socket(struct sockaddr *addr)  
{  
    int sockfd = 0;  
    sockfd = socket(PF_INET, SOCK_STREAM, 0);  
  
    if(sockfd < 0)  
    {  
        fprintf(stderr, "Open sockfd(TCP ) error!\n");  
        exit(-1);  
    }  
  
    if(connect(sockfd, addr, sizeof(struct sockaddr)) < 0)  
    {  
        close(sockfd);  
  
        fprintf(stderr, "Connect sockfd(TCP ) error!\n");  
        exit(-1);  
    }  
  
    return sockfd;  
  
}  
