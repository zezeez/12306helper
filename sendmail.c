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
#include "config.h"
#include "ordertickethelper.h"
  
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
  
int sendemail(struct user_config *, char *body);  
int open_socket(struct sockaddr *addr);  
  
int setup_mail(struct user_config *uc, struct train_info *t_info)  
{  
    //char email[] = "smtp.163.com";  
    /*char bodyHead[] = "From: \"gxzpljj\"<gxzpljj@163.com>\r\n"  
        "To:  \"gxzpljj\"<gxzpljj@163.com>\r\n"  
        "MIME-Version:1.0\r\n"  
        //"Content-Type:multipart/mixed;boundary=\"---=_NextPart_000_0050_01C\"\r\n"  
        "Content-Type:text/plain;boundary=\"---=_NextPart_000_0050_01C\"\r\n"  
        "Subject:test\r\n"  
        "\r\n"  
        "-----=_NextPart_000_0050_01C\r\n"  
        "Content-Type:text/plain;charset=\"utf8\"\r\n"  
        "\r\n"  
        "hello \r\n"  
        "-----=_NextPart_000_0050_01C\r\n"  
        "\r\n\r\n";
        //"Content-Type:application/octet-stream;name=\"test.png\"\r\n"  
        "Content-Transfer-Encoding:base64\r\n"  
        //"Content-Disposion:attachment;filename=\"test.png\""  
        "\r\n\r\n"; 
         */ 
    char bodyHead[1024];
    snprintf(bodyHead, sizeof(bodyHead), "From: \"%s\"<%s>\r\nTo \"%s\"<%s>\r\nMIME-Version:1.0\r\nContent-Type:text/html;boundary=\"---=_NextPart_000_0050_01C\"\r\nSubject:Tickethelper notification\r\n\r\n"
	    "&nbsp;&nbsp;You recieve this email because you use tickethelper to order ticket at <a href=\"https://kyfw.12306.cn\">12306</a>. Congratulations a ticket is ready for you from %s to %s at %s %s, please pay your order at <a href=\"https://kyfw.12306.cn\">12306</a> as soon as possible or your order will be canceled after 30 minutes.<br />&nbsp;&nbsp;Thanks for using tickethelper.\r\n\r\n", 
	    uc->_mail_username, uc->_mail_username, uc->_mail_username, uc->_mail_username, t_info->from_station_name, t_info->to_station_name, t_info->start_train_date, t_info->start_time);

    if(sendemail(uc, bodyHead) < 0) {
	return -1;
    }

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
 * @param user config  
 * @param body  
 */  
int sendemail(struct user_config *uc, char *body)  
{  
    int sockfd = 0;  
    struct sockaddr_in their_addr;
    char buf[1500]; 
    char rbuf[1500];
    char login[128];
    char pass[128];
    struct hostent *host = NULL;  
  
    if((host = gethostbyname(uc->_mail_server))==NULL)/*取得主机IP地址*/  
    {  
        fprintf(stderr,"Gethostname error, %s\n", strerror(errno));  
        return -1;
    }  
  
    memset(&their_addr, 0, sizeof(their_addr));  
    their_addr.sin_family = AF_INET;  
    their_addr.sin_port = htons(25);  
    their_addr.sin_addr = *((struct in_addr *)host->h_addr);  
  
    sockfd = open_socket((struct sockaddr *)&their_addr);  
    //memset(rbuf, 0, 1500);  
    while(recv(sockfd, rbuf, sizeof(rbuf), 0) == 0)  
    {  
        printf("reconnect..\n");  
        close(sockfd);  
        sleep(2);  
        sockfd = open_socket((struct sockaddr *)&their_addr);  
        //memset(rbuf, 0, 1500);  
    }  
  
    /* EHLO */  
    //memset(buf, 0, 1500);  
    sprintf(buf, "EHLO newer-PC\r\n");  
  
    if(send(sockfd, buf, strlen(buf), 0) < 0) {
	perror("send: ");
	return -1;
    }
    //memset(rbuf, 0, 1500);  
    if(recv(sockfd, rbuf, sizeof(rbuf), 0) < 0) {
	perror("recv: ");
	return -1;
    }
    printf("%s\n", rbuf);  
  
    /*AUTH LOGIN  */  
    //memset(buf, 0, 1500);  
    sprintf(buf, "AUTH LOGIN\r\n");  
    if(send(sockfd, buf, strlen(buf), 0) < 0) {
	perror("send: ");
	return -1;
    }
  
    //memset(rbuf, 0, 1500);  
    if(recv(sockfd, rbuf, sizeof(rbuf), 0) < 0) {
	perror("recv: ");
	return -1;
    }
    printf("%s\n", rbuf);  
  
    /* USER */  
    //memset(buf, 0, 1500);  
  
    //sprintf(buf, "test@163.com");  
    //memset(login, 0, sizeof(login));  
  
    base64_encode((unsigned char *)uc->_mail_username, strlen(uc->_mail_username), login, 128);                   /* base64 */  
  
    sprintf(buf, "%s\r\n", login);  
    if(send(sockfd, buf, strlen(buf), 0) < 0) {
	perror("send: ");
	return -1;
    }
    //memset(rbuf, 0, 1500);  
    if(recv(sockfd, rbuf, sizeof(rbuf), 0) < 0) {
	perror("recv: ");
	return -1;
    }
    printf("%s\n", rbuf);  
  
    /* PASSWORD */  
    //memset(buf, 0, 1500);  
    //sprintf(buf, "test");  
    //memset(pass, 0, 128);  
  
    base64_encode((unsigned char *)uc->_mail_password, strlen(uc->_mail_password), pass, 128);  
    sprintf(buf, "%s\r\n", pass);  
    if(send(sockfd, buf, strlen(buf), 0) < 0) {
	perror("send: ");
	return -1;
    }
  
    //printf("%s, %d\n", buf, __LINE__);  
  
    //memset(rbuf, 0, 1500);  
    if(recv(sockfd, rbuf, sizeof(rbuf), 0) < 0) {
	perror("recv: ");
	return -1;
    }
    printf("%s\n", rbuf);  
    //printf("%s, %d\n", rbuf, __LINE__);  
  
    /* MAIL FROM */  
    snprintf(buf, sizeof(buf), "MAIL FROM: <%s>\r\n", uc->_mail_username);  
    if(send(sockfd, buf, strlen(buf), 0) < 0) {
	perror("send: ");
	return -1;
    }
  
    //memset(rbuf, 0, 1500);  
    if(recv(sockfd, rbuf, sizeof(rbuf), 0) < 0) {
	perror("recv: ");
	return -1;
    }
    printf("%s\n", rbuf);  
  
    /* rcpt to 第一个收件人 */  
    snprintf(buf, sizeof(buf), "RCPT TO:<%s>\r\n", uc->_mail_username);  
    if(send(sockfd, buf, strlen(buf), 0) < 0) {
	perror("send: ");
	return -1;
    }
  
    //memset(rbuf, 0, 1500);  
    if(recv(sockfd, rbuf, sizeof(rbuf), 0) < 0) {
	perror("recv: ");
	return -1;
    }
    printf("%s\n", rbuf);  
  
    /* DATA email connext ready  */  
    sprintf(buf, "DATA\r\n");  
    if(send(sockfd, buf, strlen(buf), 0) < 0) {
	perror("send: ");
	return -1;
    }
  
    //memset(rbuf, 0, 1500);  
    if(recv(sockfd, rbuf, sizeof(rbuf), 0) < 0) {
	perror("recv: ");
	return -1;
    }
    printf("%s\n", rbuf);  
  
    /* send email connext \r\n.\r\n end*/  
    sprintf(buf, "%s\r\n.\r\n", body);  
    if(send(sockfd, buf, strlen(buf), 0) < 0) {
	perror("send: ");
	return -1;
    }
    //memset(rbuf, 0, 1500);  
    if(recv(sockfd, rbuf, sizeof(rbuf), 0) < 0) {
	perror("recv: ");
	return -1;
    }
    printf("%s\n", rbuf);  
  
    /* QUIT */  
    sprintf(buf, "QUIT\r\n");  
    if(send(sockfd, buf, strlen(buf), 0) < 0) {
	perror("send: ");
	return -1;
    }
    //memset(rbuf, 0, 1500);  
  
    if(recv(sockfd, rbuf, sizeof(rbuf), 0) < 0) {
	return -1;
    }
    printf("%s\n", rbuf);  
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
