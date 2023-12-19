#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifndef _WIN32
#include <arpa/inet.h>
#else
#include <winsock2.h>
#include <stdint.h>
#endif
#ifndef _WIN32
#include <netpacket/packet.h>
#include <net/if.h>
#include <netinet/if_ether.h>
#endif
#include <pthread.h>

#define PORT 67
// #define PORT 3000
#define WEB_UI_CONFIG_UPDATE_PORT 8080
// #define BUFFER_SIZE 1024
#define BUFFER_SIZE 600
#define MAX_LEASES 512
// #define LEASE_TIME 600
#include <time.h>
#include "map.h"

#include <signal.h>

// TODO: lista klientów
// TODO: obsłużyć release i decline
// TODO: pula adresów
// TODO: zwalnianie dzierżaw

int LEASE_TIME = 600;

unsigned int generateRandomHex()
{
    srand((unsigned int)time(NULL));
    return rand() & 0xFFFFFFFF;
}

// const char *ip_str = "192.168.1.2";
// const char *serverIp = "192.168.1.1";
// const unsigned char serverIp[4] = {0xC0, 0xA8, 0x01, 0x01};
unsigned char offered_subnet_base[4] = {0xC0, 0xA8, 0x01, 0x00};
unsigned char offered_subnet_mask[4] = {0xFF, 0xFF, 0xFF, 0x00};

SimpleMap transactionIDsToMACs;

unsigned int combineBytes(unsigned char *array, size_t size)
{
    unsigned int returnVal = 0;
    for (size_t i = 0; i < size; i++)
    {
        returnVal |= array[i];
        if (!(i == size - 1))
        {
            returnVal <<= 8;
        }
    }
    return returnVal;
}

void intToHex(unsigned int num, char hexString[])
{
    sprintf(hexString, "%X", num);
}

enum DHCP_Options
{
    MessageType = 53,
    End = 255,
    ClientIdentifier = 61,
    ParameterRequestList = 55,
    MaximumDHCPMessageSize = 57,
    HostName = 12,
    RequestedIPAddress = 50,
    VendorClassId = 60,
    SubnetMask = 1,
    ServerID = 54,
    IPAdressLeaseTime = 51,
};

enum DHCP_MessageTypes
{
    Offer = 0x02,
    Discover = 0x01,
    Request = 0x03,
    Ack = 0x05,
};

unsigned char DHCP_MagicCookie[4] = {0x63, 0x82, 0x53, 0x63};

int isValidOption(int opt)
{
    switch (opt)
    {
    case MessageType:
    case End:
    case ClientIdentifier:
    case ParameterRequestList:
    case MaximumDHCPMessageSize:
    case HostName:
    case RequestedIPAddress:
    case VendorClassId:
    case SubnetMask:
    case ServerID:
    case IPAdressLeaseTime:
        return 1;
    default:
        return 0;
    }
}

int addOption(char *buffer, int start, int optionType, char *values, int valuesLen)
{
    buffer[start] = optionType;
    buffer[start + 1] = valuesLen;
    for (size_t i = 0; i < valuesLen; i++)
    {
        buffer[start + 1 + 1 + i] = values[i];
    }
    return start + 1 + 1 + valuesLen;
}

// void handle_client(int client_sock, struct sockaddr_in client_addr) {
// }

typedef struct
{
    unsigned char MAC_addr[6];
    unsigned char IP_addr[4];
    int duration;
    long start;
    int valid;
    // char hostName[512];
    char *hostName;
} DHCP_Lease;

DHCP_Lease clients[MAX_LEASES];
int clientsLen = 0;
pthread_mutex_t leaseMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t configMutex = PTHREAD_MUTEX_INITIALIZER;

void *handle_leases()
{
    while (1)
    {
        // printf("test\n");
        // sleep(5);
        // if ()
        for (size_t i = 0; i < clientsLen; i++)
        {
            pthread_mutex_lock(&leaseMutex);
            DHCP_Lease lease = clients[i];
            if (time(NULL) == lease.start + lease.duration)
            {
                printf("%d: lease expired for %d.%s: %d\n", time(NULL), i, lease.hostName, lease.IP_addr[3]);
                lease.valid = 0;
                clients[i] = lease;

                for (int j = i; j < clientsLen - 1; ++j)
                {
                    clients[j] = clients[j + 1];
                }
                clientsLen--;
            }
            pthread_mutex_unlock(&leaseMutex);
        }
        sleep(1);
    }
    return NULL;
}

// SimpleMap clients;
int ip_range[2] = {
    2,
    254,
};

int chooseIp()
{
    // for (size_t j = 0; j < clients.size; j++)
    // {
    //     // pthread_mutex_lock(&leaseMutex);
    //     for (size_t i = ip_range[0]; i < ip_range[1]; i++)
    //     {
    //         if (((struct DHCP_Lease *)clients.data[i].value)->IP_addr[4] == i)
    //         {
    //             continue;
    //         }
    //         return
    //     }
    //     // pthread_mutex_unlock(&leaseMutex);
    // }
    // int minRange = ip_range[0], maxRange = ip_range[1];
    // for (size_t i = minRange; i <= maxRange; ++i)
    // {
    //     int isOccupied = 0;
    //     for (size_t j = 0; j < clients.size; ++j)
    //     {
    //         if (((struct DHCP_Lease *)clients.data[j].value)->IP_addr[3] == i)
    //         {
    //             isOccupied = 1;
    //             break;
    //         }
    //     }
    //     if (!(isOccupied == 1))
    //     {
    //         return i;
    //     }
    // }
    // return -1;

    int usedOctets[256];
    memset(usedOctets, 0, sizeof(usedOctets));
    int minRange = ip_range[0], maxRange = ip_range[1];
    for (int i = 0; i < clientsLen; i++)
    {
        // void *val = clients[i];
        // DHCP_Lease* lease = ((DHCP_Lease*)(val));
        DHCP_Lease lease = clients[i];
        // unsigned char ip[4];
        // for (int j = 0; j < 4; j++)
        // {
        //     ip[j] = lease->IP_addr[j];
        // }
        // usedOctets[ip[3]]++;
        if (lease.valid == 1)
            usedOctets[lease.IP_addr[3]]++;
    }

    int availableOctet = -1;
    for (int i = minRange; i <= maxRange; i++)
    {
        if (usedOctets[i] == 0)
        {
            availableOctet = i;
            break;
        }
    }

    return availableOctet;
}

void addLease(DHCP_Lease lease)
{
    // unsigned char *targetMAC = lease.MAC_addr;
    // int offset = clients.size > 0 ? (clients.size + 1) : 1;
    // int targetMAC_asInt = (targetMAC[0] + targetMAC[1] + targetMAC[2] + targetMAC[3] + targetMAC[4] + targetMAC[5]) * offset;
    // void *val = getValueByKey(&clients, targetMAC_asInt);
    // // if (val == NULL)
    // pthread_mutex_lock(&leaseMutex);
    // insertKeyValuePair(&clients, targetMAC_asInt, &lease, 6);
    // pthread_mutex_unlock(&leaseMutex);
    pthread_mutex_lock(&leaseMutex);
    clients[clientsLen] = lease;
    clientsLen++;
    printf("%d: lease added\n", time(NULL));
    pthread_mutex_unlock(&leaseMutex);
}

#ifndef _WIN32
int mySockFd;
#else
SOCKET mySockFd;
#endif
pthread_t leasesThread;
pthread_t webUiConfigUpdateThread;

void loadConfig();

void *webUiConfigUpdateServer()
{
#ifndef _WIN32
    int sockfd;
#else
    SOCKET sockfd;
    WSADATA wsaData;
#endif
    struct sockaddr_in server_addr, client_addr;
#ifndef _WIN32
    socklen_t client_addr_len = sizeof(client_addr);
#else
    int client_addr_len = sizeof(client_addr);
#endif
    char buffer[BUFFER_SIZE];

#ifdef _WIN32
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        perror("Error initializing Winsock");
        exit(EXIT_FAILURE);
    }
#endif

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        perror("Error creating socket");
#ifdef _WIN32
        WSACleanup();
#endif
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(WEB_UI_CONFIG_UPDATE_PORT);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("Error binding socket");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        ssize_t recv_len = recvfrom(sockfd, buffer, 128, 0, (struct sockaddr *)&client_addr, &client_addr_len);
        if (recv_len == -1)
        {
            perror("Error receiving data");
            exit(EXIT_FAILURE);
        }

        buffer[recv_len] = '\0';
        // printf("%s\n", buffer);
        if (strcmp(buffer, "reload_config") == 0)
        {
            printf("got request to reload configuration\n");
            loadConfig();
        }
        if (strcmp(buffer, "send_leases") == 0)
        {
            char response[512];
            int offs = 0;
            for (size_t i = 0; i < clientsLen; i++)
            {
                DHCP_Lease lease = clients[i];
                response[offs] = 'x';
                offs++;
                response[offs] = ',';
                offs++;
                // response[offs] = lease.duration;
                // offs++;
                char durationAsCharArr[24]; // uh how much here?
                int durationAsCharArrLen = sprintf(durationAsCharArr, "%d", lease.duration);
                for (size_t j = 0; j < durationAsCharArrLen; j++)
                {
                    response[offs] = durationAsCharArr[j];
                    offs++;
                }
                // response[offs] = lease.start;
                // offs++;
                response[offs] = ',';
                offs++;
                char longAsCharArr[24]; // uh how much here?
                int longAsCharArrLen = sprintf(longAsCharArr, "%ld", lease.start);
                for (size_t j = 0; j < longAsCharArrLen; j++)
                {
                    response[offs] = longAsCharArr[j];
                    offs++;
                }
                response[offs] = ',';
                offs++;
                for (size_t j = 0; j < 4; j++)
                {
                    response[offs] = lease.IP_addr[j];
                    offs++;
                }
                response[offs] = ',';
                offs++;
                for (size_t j = 0; j < 6; j++)
                {
                    response[offs] = lease.MAC_addr[j];
                    offs++;
                }
                response[offs] = ',';
                offs++;
                for (size_t j = 0; j < strlen(lease.hostName); j++)
                {
                    response[offs] = lease.hostName[j];
                    offs++;
                }
                response[offs] = '\n';
                offs++;
            }

            if (sendto(sockfd, response, 512, 0, (struct sockaddr *)&client_addr, client_addr_len) == -1)
            {
                perror("Error sending response");
                exit(EXIT_FAILURE);
            }
        }
        // const char *response = {};
        // if (sendto(sockfd, response, strlen(response), 0, (struct sockaddr *)&client_addr, client_addr_len) == -1)
        // {
        //     perror("Error sending response");
        //     exit(EXIT_FAILURE);
        // }
    }

    close(sockfd);

    return NULL;
}

static void cleanup()
{
    printf("cleanup\n");
    pthread_join(leasesThread, NULL);
    pthread_join(webUiConfigUpdateThread, NULL);
#ifndef _WIN32
    close(mySockFd);
#else
    closesocket(mySockFd);
    WSACleanup();
#endif
}

void handle_signal(int signum)
{
    printf("Received signal %d\n", signum);
    exit(EXIT_SUCCESS);
}

int occurrencesOfTargetInStr(const char *str, char target)
{
    int i, count;
    for (i = 0, count = 0; str[i]; i++)
        count += (str[i] == '.');
    return count;
}

char *slice(const char *str, int start, int end)
{
    int length = strlen(str);
    start = (start < 0) ? length + start : start;
    end = (end < 0) ? length + end : end;

    if (start < 0 || start >= length || end < start || end > length)
    {
        return NULL;
    }
    int sliceLength = end - start;

    char *slicedStr = (char *)malloc((sliceLength + 1) * sizeof(char));
    strncpy(slicedStr, str + start, sliceLength);
    slicedStr[sliceLength] = '\0';
    return slicedStr;
}

int indexOf(const char *str, char target, int startIndex)
{
    int index = -1;
    for (int i = startIndex; str[i] != '\0'; ++i)
    {
        if (str[i] == target)
        {
            index = i;
            break;
        }
    }
    return index;
}

int str2int(int *out, char *s, int base)
{
    char *end;
    long l = strtol(s, &end, base);
    if (*end != '\0')
        return 3;
    *out = l;
    return 0;
}
unsigned char serverIp[4];

void loadConfig()
{
    FILE *filePtr;

    if (filePtr = fopen("config.conf", "r"))
    {
        char line[256];
        while (fgets(line, 256, filePtr))
        {
            // printf("%s\n", line);
            // char **splittedString = malloc(128 * sizeof(char *));
            // size_t splittedCount;
            // splitString(line, "=", splittedString, &splittedCount);
            char *optValS_1 = slice(line, indexOf(line, '=', 0) + 1, strlen(line));
            char *optVal = slice(optValS_1, 1, strlen(optValS_1) - 2);
            // sliced2[strlen(sliced2) - 2] = '\0';
            char *optKey = slice(line, 0, indexOf(line, '=', 0));
            printf("%s = %s\n", optKey, optVal);
            // printf("%s:%s\n", splittedString[0], splittedString[1]);

            if (strcmp(optKey, "ipRange") == 0)
            {
                char *optVal_RangeS_1 = slice(optVal, 1, indexOf(optVal, ',', 0));
                // printf("%s\n", optVal_RangeS_1);
                int optVal_RangeS_2_offset = 0;
                // printf("%d\n", optVal[indexOf(optVal, ',') + 1]);
                if (optVal[indexOf(optVal, ',', 0) + 1] == ' ')
                    optVal_RangeS_2_offset++;
                char *optVal_RangeS_2 = slice(optVal, indexOf(optVal, ',', 0) + 1 + optVal_RangeS_2_offset, strlen(optVal) - 1);
                // printf("%s\n", optVal_RangeS_2);
                int out_1;
                str2int(&out_1, optVal_RangeS_1, 10);
                int out_2;
                str2int(&out_2, optVal_RangeS_2, 10);
                pthread_mutex_lock(&configMutex);
                ip_range[0] = out_1 + 0;
                ip_range[1] = out_2 + 0;
                pthread_mutex_unlock(&configMutex);

                free(optVal_RangeS_1);
                free(optVal_RangeS_2);
            }

            if (strcmp(optKey, "subnetMask") == 0)
            {
                int firstOctetIndex = indexOf(optVal, '.', 0);
                int secondOctetIndex = indexOf(optVal, '.', firstOctetIndex + 1);
                int thirdOctetIndex = indexOf(optVal, '.', secondOctetIndex + 1);
                int fourthOctetIndex = strlen(optVal);
                char *firstOctet = slice(optVal, 0, firstOctetIndex);
                char *secondOctet = slice(optVal, firstOctetIndex + 1, secondOctetIndex);
                char *thirdOctet = slice(optVal, secondOctetIndex + 1, thirdOctetIndex);
                char *fourthOctet = slice(optVal, thirdOctetIndex + 1, fourthOctetIndex);
                // printf("%s : %s : %s : %s\n", firstOctet, secondOctet, thirdOctet, fourthOctet);
                int octet1;
                str2int(&octet1, firstOctet, 10);
                int octet2;
                str2int(&octet2, secondOctet, 10);
                int octet3;
                str2int(&octet3, thirdOctet, 10);
                int octet4;
                str2int(&octet4, fourthOctet, 10);

                pthread_mutex_lock(&configMutex);
                offered_subnet_mask[0] = octet1 + 0;
                offered_subnet_mask[1] = octet2 + 0;
                offered_subnet_mask[2] = octet3 + 0;
                offered_subnet_mask[3] = octet4 + 0;
                pthread_mutex_unlock(&configMutex);

                free(firstOctet);
                free(secondOctet);
                free(thirdOctet);
                free(fourthOctet);
            }

            if (strcmp(optKey, "subnetBase") == 0)
            {
                int firstOctetIndex = indexOf(optVal, '.', 0);
                int secondOctetIndex = indexOf(optVal, '.', firstOctetIndex + 1);
                int thirdOctetIndex = indexOf(optVal, '.', secondOctetIndex + 1);
                int fourthOctetIndex = strlen(optVal);
                char *firstOctet = slice(optVal, 0, firstOctetIndex);
                char *secondOctet = slice(optVal, firstOctetIndex + 1, secondOctetIndex);
                char *thirdOctet = slice(optVal, secondOctetIndex + 1, thirdOctetIndex);
                char *fourthOctet = slice(optVal, thirdOctetIndex + 1, fourthOctetIndex);
                // printf("%s : %s : %s : %s\n", firstOctet, secondOctet, thirdOctet, fourthOctet);
                int octet1;
                str2int(&octet1, firstOctet, 10);
                int octet2;
                str2int(&octet2, secondOctet, 10);
                int octet3;
                str2int(&octet3, thirdOctet, 10);
                int octet4;
                str2int(&octet4, fourthOctet, 10);

                pthread_mutex_lock(&configMutex);
                offered_subnet_base[0] = octet1 + 0;
                offered_subnet_base[1] = octet2 + 0;
                offered_subnet_base[2] = octet3 + 0;
                offered_subnet_base[3] = octet4 + 0;
                pthread_mutex_unlock(&configMutex);

                free(firstOctet);
                free(secondOctet);
                free(thirdOctet);
                free(fourthOctet);
            }

            if (strcmp(optKey, "myIp") == 0)
            {
                int firstOctetIndex = indexOf(optVal, '.', 0);
                int secondOctetIndex = indexOf(optVal, '.', firstOctetIndex + 1);
                int thirdOctetIndex = indexOf(optVal, '.', secondOctetIndex + 1);
                int fourthOctetIndex = strlen(optVal);
                char *firstOctet = slice(optVal, 0, firstOctetIndex);
                char *secondOctet = slice(optVal, firstOctetIndex + 1, secondOctetIndex);
                char *thirdOctet = slice(optVal, secondOctetIndex + 1, thirdOctetIndex);
                char *fourthOctet = slice(optVal, thirdOctetIndex + 1, fourthOctetIndex);
                // printf("%s : %s : %s : %s\n", firstOctet, secondOctet, thirdOctet, fourthOctet);
                int octet1;
                str2int(&octet1, firstOctet, 10);
                int octet2;
                str2int(&octet2, secondOctet, 10);
                int octet3;
                str2int(&octet3, thirdOctet, 10);
                int octet4;
                str2int(&octet4, fourthOctet, 10);

                pthread_mutex_lock(&configMutex);
                serverIp[0] = octet1 + 0;
                serverIp[1] = octet2 + 0;
                serverIp[2] = octet3 + 0;
                serverIp[3] = octet4 + 0;
                pthread_mutex_unlock(&configMutex);

                free(firstOctet);
                free(secondOctet);
                free(thirdOctet);
                free(fourthOctet);
            }

            if (strcmp(optKey, "leaseTime") == 0)
            {
                int out_1;
                str2int(&out_1, optVal, 10);
                pthread_mutex_lock(&configMutex);
                LEASE_TIME = out_1;
                pthread_mutex_unlock(&configMutex);
            }

            free(optValS_1);
            free(optVal);
            free(optKey);
        }
        fclose(filePtr);
    }
}

int main()
{
    // unsigned char serverIp[4] = {
    //     offered_subnet_base[0],
    //     offered_subnet_base[1],
    //     offered_subnet_base[2],
    //     0x01,
    // };
    serverIp[0] = offered_subnet_base[0];
    serverIp[1] = offered_subnet_base[1];
    serverIp[2] = offered_subnet_base[2];
    serverIp[3] = 0x01;
#ifndef _WIN32
    int sockfd;
#else
    SOCKET sockfd;
    WSADATA wsaData;
#endif
    struct sockaddr_in server_addr, client_addr;
#ifndef _WIN32
    socklen_t client_addr_len = sizeof(client_addr);
#else
    int client_addr_len = sizeof(client_addr);
#endif
    char buffer[BUFFER_SIZE];

#ifdef _WIN32
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        perror("Error initializing Winsock");
        exit(EXIT_FAILURE);
    }
#endif

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        perror("Error creating socket");
#ifdef _WIN32
        WSACleanup();
#endif
        exit(EXIT_FAILURE);
    }
    mySockFd = sockfd;
    atexit(cleanup);
    // signal(SIGINT, exit);
    // signal(SIGTERM, exit);
    if (signal(SIGINT, handle_signal) == SIG_ERR)
    {
        perror("Unable to set up signal handler");
        return EXIT_FAILURE;
    }

    loadConfig();

#ifndef _WIN32
    int broadcastEnable = 1;
    int ret = setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));
#endif

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("Error binding socket");
        exit(EXIT_FAILURE);
    }

    initializeMap(&transactionIDsToMACs);
    // initializeMap(&clients);

    pthread_create(&leasesThread, NULL, handle_leases, NULL);
    pthread_create(&webUiConfigUpdateThread, NULL, webUiConfigUpdateServer, NULL);
    printf("running and awaiting connection\nCtrl-C to stop\nconfig update trigger set at %d\n", WEB_UI_CONFIG_UPDATE_PORT);

    while (1)
    {
        ssize_t recv_len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, &client_addr_len);
        if (recv_len == -1)
        {
            perror("Error receiving data");
            exit(EXIT_FAILURE);
        }

        buffer[recv_len] = '\0'; // dodajemy nulla
        printf("Received from %s:%d: %s\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), buffer);

        int replyWithOffer = 0;
        int replyWithAck = 0;

        unsigned char transactionId[4] = {
            buffer[4],
            buffer[5],
            buffer[6],
            buffer[7],
            // '\0',
        };
        unsigned char combinedTransactionIdAsCharArray[4] = {
            buffer[4],
            buffer[5],
            buffer[6],
            buffer[7],
            // '\0',
        };
        // unsigned int combinedTransactionId = combineBytes(transactionId, 4);
        // unsigned char combinedTransactionIdAsCharArray[8];
        // intToHex(combinedTransactionId, combinedTransactionIdAsCharArray);
        // unsigned char *combinedTransactionIdAsCharArray = transactionId;

        char requestedIP[4];
        char requestedHostname[512];

        if (buffer[0] == 0x01)
        {
            char htype = buffer[1];
            char hlen = buffer[2];
            char hops = buffer[3];

            unsigned char elapsed[2] = {
                buffer[8],
                buffer[9],
            };

            unsigned char bootp[2] = {
                buffer[10],
                buffer[11],
            };

            unsigned char clientIP[4] = {
                buffer[12],
                buffer[13],
                buffer[14],
                buffer[15],
            };

            unsigned char yourIP[4] = {
                buffer[16],
                buffer[17],
                buffer[18],
                buffer[19],
            };

            unsigned char nextServer[4] = {
                buffer[20],
                buffer[21],
                buffer[22],
                buffer[23],
            };

            unsigned char relayAgent[4] = {
                buffer[24],
                buffer[25],
                buffer[26],
                buffer[27],
            };

            // unsigned int MAC_octet1 = buffer[28];
            // unsigned int MAC_octet2 = buffer[29];
            // unsigned int MAC_octet3 = buffer[30];
            // unsigned int MAC_octet4 = buffer[31];
            // unsigned int MAC_octet5 = buffer[32];
            // unsigned int MAC_octet6 = buffer[33];

            /*
            int MAC_octets[6] = {
                buffer[28],
                buffer[29],
                buffer[30],
                buffer[31],
                buffer[32],
                buffer[33],
            };

            unsigned int combinedTransactionId = combineBytes(transactionId, 4);
            // insertKeyValuePair(&transactionIDsToMACs, )
            unsigned char combinedTransactionIdAsCharArray[8];
            intToHex(combinedTransactionId, combinedTransactionIdAsCharArray);
            insertKeyValuePair(&transactionIDsToMACs, combinedTransactionIdAsCharArray, MAC_octets);
            */

            // int offset = 0;
            unsigned char *MAC_octets = malloc(6 * sizeof(char));
            for (int i = 0; i < 6; ++i)
            {
                // sprintf(MAC_octets, "%02x", (unsigned char)buffer[28 + i]);
                MAC_octets[i] = (unsigned char)buffer[28 + i]; // end at 33
                // printf("MAC_octets end at %d\n", 28 + i);
            }
            int combinedTransactionIdAsInt = combinedTransactionIdAsCharArray[0] + combinedTransactionIdAsCharArray[1] + combinedTransactionIdAsCharArray[2] + combinedTransactionIdAsCharArray[3];
            insertKeyValuePair(&transactionIDsToMACs, combinedTransactionIdAsInt, (unsigned char *)MAC_octets, 4);

            unsigned char MAC_octets_padding[10];
            for (int i = 0; i < 10; ++i)
            {
                MAC_octets_padding[i] = (unsigned char)buffer[34 + i];
                // printf("MAC_octets_padding end at %d\n", 34 + i);
            }

            unsigned char serverHostName[64];
            for (int i = 0; i < 64; ++i)
            {
                serverHostName[i] = (unsigned char)buffer[44 + i];
                // printf("serverHostName end at %d\n", 44 + i);
            }

            unsigned char bootFilename[128];
            for (int i = 0; i < 128; ++i)
            {
                bootFilename[i] = (unsigned char)buffer[108 + i];
                // printf("bootFilename end at %d\n", 108 + i);
            }

            // unsigned char magicCookie[4] = {
            //     buffer[236],
            //     buffer[237],
            //     buffer[238],
            //     buffer[239],
            // };
            // should be DHCP_MagicCookie

            // printf("%s\n", magicCookie);

            // unsigned char messageType[3] = {
            //     buffer[240],
            //     buffer[241],
            //     buffer[242],
            // };
            // printf("test\n");

            int isDiscovery = 0;
            int isRequest = 0;

            int optionsStart = 240;
            for (size_t i = 0; i < recv_len; i++)
            {
                int toSkip = 0;
                unsigned char current = buffer[optionsStart + i];
                // printf("test\n");
                if (isValidOption(current) == 0)
                    continue;
                if (current == 0xFF)
                {
                    break;
                }
                unsigned char currentLen = buffer[optionsStart + i + 1];
                toSkip++;
                toSkip += currentLen;
                for (size_t j = 0; j < currentLen; j++)
                {
                    unsigned char currentSetting = buffer[optionsStart + i + 1 + 1 + j];
                    if (current == MessageType)
                    {
                        printf("%d\n", currentSetting);
                        if (currentSetting == Request)
                        {
                            isRequest = 1;
                            continue;
                        }
                        if (currentSetting == Discover)
                        {
                            isDiscovery = 1;
                            continue;
                        }
                    }
                    if (current == RequestedIPAddress)
                    {
                        requestedIP[j] = currentSetting;
                    }
                    if (current == HostName)
                    {
                        requestedHostname[j] = currentSetting;
                        if (j == currentLen - 1)
                            requestedHostname[j + 1] = '\0';
                    }
                }
                i += toSkip;
            }

            if (isDiscovery == 1)
                replyWithOffer = 1;
            if (isRequest)
                replyWithAck = 1;
        }

        // char responseBuffer_[BUFFER_SIZE];
        // if (sendto(sockfd, responseBuffer_, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, client_addr_len) == -1)
        // {
        //     perror("Error sending response");
        //     exit(EXIT_FAILURE);
        // }
        printf("sigma\n");
        // continue;
        if (replyWithOffer == 1 && replyWithAck == 0)
        {
            // struct sockaddr_in fake_client_addr;
            // inet_pton(AF_INET, ip_str, &(fake_client_addr.sin_addr));
            // // Extracting the four octets
            // uint32_t proposedIP = fake_client_addr.sin_addr.s_addr;
            char proposedIP[4] = {
                offered_subnet_base[0],
                offered_subnet_base[1],
                offered_subnet_base[2],
                // 0x02,
                chooseIp(),
            };

            char responseBuffer[BUFFER_SIZE]; // uwaga niepotrzebnie duże użycie pamięci tutaj
            responseBuffer[0] = 0x02;         // bootreply OP
            responseBuffer[1] = 0x01;         // ethernet HTYPE
            responseBuffer[2] = 0x06;         // długość 6 HLEN
            responseBuffer[3] = 0x00;         // HOPS nie wiem co to właściwie

            // unsigned int transactionId = generateRandomHex(); // 4 bajty
            // responseBuffer[4] = (transactionId >> 24) & 0xFF; // 1 bajt
            // responseBuffer[5] = (transactionId >> 16) & 0xFF; // 2 bajt
            // responseBuffer[6] = (transactionId >> 8) & 0xFF;  // 3 bajt
            // responseBuffer[7] = transactionId & 0xFF;         // 4 bajt
            responseBuffer[4] = transactionId[0];
            responseBuffer[5] = transactionId[1];
            responseBuffer[6] = transactionId[2];
            responseBuffer[7] = transactionId[3];

            responseBuffer[8] = 0x00; // sekundy które upłynęły
            responseBuffer[9] = 0x00; // nie jest to ważne a zajmuje 2 bajty :skull:

            responseBuffer[10] = 0x00; // flagi bootp
            responseBuffer[11] = 0x00; // nie jest to ważne a zajmuje 2 bajty :skull:

            responseBuffer[12] = (client_addr.sin_addr.s_addr) & 0xFF;      // adres ip klienta cokolwiek to znaczy
            responseBuffer[13] = (client_addr.sin_addr.s_addr >> 8) & 0xFF; // zazwyczaj są tu zera
            responseBuffer[14] = (client_addr.sin_addr.s_addr >> 16) & 0xFF;
            responseBuffer[15] = (client_addr.sin_addr.s_addr >> 24) & 0xFF;

            // responseBuffer[16] = (proposedIP) & 0xFF; // adres ip proponowany przez serwer dhcp
            // responseBuffer[17] = (proposedIP >> 8) & 0xFF;
            // responseBuffer[18] = (proposedIP >> 16) & 0xFF;
            // responseBuffer[19] = (proposedIP >> 24) & 0xFF;
            responseBuffer[16] = proposedIP[0];
            responseBuffer[17] = proposedIP[1];
            responseBuffer[18] = proposedIP[2];
            responseBuffer[19] = proposedIP[3];

            if (strcmp(inet_ntoa(client_addr.sin_addr), "127.0.0.1"))
            {
                struct sockaddr_in fake_client_addr2;
                fake_client_addr2.sin_family = AF_INET;
                fake_client_addr2.sin_port = client_addr.sin_port;
                // fake_client_addr2.sin_addr.s_addr = INADDR_BROADCAST;
                // char formatted[32];
                // int len = sprintf(formatted, "%d.%d.%d.%d", (int)offered_subnet_base[0], (int)offered_subnet_base[1], (int)offered_subnet_base[2], (int)0xFF) + 1;
                unsigned char broadcast[4] = {
                    offered_subnet_base[0],
                    offered_subnet_base[1],
                    offered_subnet_base[2],
                    0xFF,
                };
                // char *cutFormatted = malloc(len * sizeof(int));
                // for (size_t i = 0; i < len; i++)
                // {
                //     cutFormatted[i] = formatted[i];
                // }
                // inet_pton(AF_INET, "192.168.1.255", &(fake_client_addr2.sin_addr)); // broadcast tej sieci
                // inet_pton(AF_INET, cutFormatted, &(fake_client_addr2.sin_addr)); // broadcast tej sieci
                struct in_addr addr;
                addr.s_addr = *((uint32_t *)broadcast);
                fake_client_addr2.sin_addr = addr;
                client_addr = fake_client_addr2;
                client_addr_len = sizeof(client_addr);
                // free(cutFormatted);
                // client_addr = INADDR_BROADCAST;
            }

            responseBuffer[20] = 0x00; // następny serwer
            responseBuffer[21] = 0x00;
            responseBuffer[22] = 0x00;
            responseBuffer[23] = 0x00;

            responseBuffer[24] = 0x00; // agent przekładni ???
            responseBuffer[25] = 0x00;
            responseBuffer[26] = 0x00;
            responseBuffer[27] = 0x00;

            int combinedTransactionIdAsInt = combinedTransactionIdAsCharArray[0] + combinedTransactionIdAsCharArray[1] + combinedTransactionIdAsCharArray[2] + combinedTransactionIdAsCharArray[3];
            unsigned char *targetMAC = (unsigned char *)getValueByKey(&transactionIDsToMACs, combinedTransactionIdAsInt); // adres MAC naszego klienta
            responseBuffer[28] = targetMAC[0];
            responseBuffer[29] = targetMAC[1];
            responseBuffer[30] = targetMAC[2];
            responseBuffer[31] = targetMAC[3];
            responseBuffer[32] = targetMAC[4];
            responseBuffer[33] = targetMAC[5];

            for (size_t i = 0; i < 10; i++)
            {
                responseBuffer[34 + i] = 0; // padding adresu MAC
            }

            for (size_t i = 0; i < 64; i++)
            {
                responseBuffer[44 + i] = 0; // hostname serwera
            }

            for (size_t i = 0; i < 128; i++)
            {
                responseBuffer[108 + i] = 0; // "boot filename" cokolwiek to jest
            }

            // responseBuffer[236] = 0x63;
            // responseBuffer[237] = 0x82;
            // responseBuffer[238] = 0x53;
            // responseBuffer[239] = 0x63;
            for (size_t i = 0; i < 4; i++)
            {
                responseBuffer[236 + i] = DHCP_MagicCookie[i]; // magiczne ciastko
            }

            // responseBuffer[240] = MessageType;
            // responseBuffer[241] = 0x01;
            // responseBuffer[242] = Offer;
            int messageTypeEnd = addOption(responseBuffer, 240, MessageType, (char[]){Offer}, 1); // dodajemy opcje oferty
            // addOption(responseBuffer, 243, SubnetMask, offered_subnet_mask, 4);
            char offeredSubnetMaskCopy[4] = {
                offered_subnet_mask[0],
                offered_subnet_mask[1],
                offered_subnet_mask[2],
                offered_subnet_mask[3],
            };
            int subnetMaskEnd = addOption(responseBuffer, messageTypeEnd, SubnetMask, offeredSubnetMaskCopy, 4);
            int serverIdEnd = addOption(responseBuffer, subnetMaskEnd, ServerID, serverIp, 4);
            // char leaseTime[4] = {0x01, 0xE1, 0x33, 0x80};
            char leaseTime[4] = {
                (LEASE_TIME >> 24) & 0xFF,
                (LEASE_TIME >> 16) & 0xFF,
                (LEASE_TIME >> 8) & 0xFF,
                LEASE_TIME & 0xFF,
            };
            int leaseTimeEnd = addOption(responseBuffer, serverIdEnd, IPAdressLeaseTime, leaseTime, 4);
            int finalEnd = leaseTimeEnd;

            // responseBuffer[243] = End; // koniec odpowiedzi
            // responseBuffer[249] = End; // koniec odpowiedzi
            responseBuffer[finalEnd] = End; // koniec odpowiedzi

            // for (size_t i = 244; i < BUFFER_SIZE; i++)
            // for (size_t i = 250; i < BUFFER_SIZE; i++)
            for (size_t i = finalEnd + 1; i < BUFFER_SIZE; i++)
            {
                responseBuffer[i] = 0;
            }
            printf("sending %d\n", responseBuffer[242]);
            if (sendto(sockfd, responseBuffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, client_addr_len) == -1)
            {
                perror("Error sending response");
                exit(EXIT_FAILURE);
            }
            continue;
        }
        if (replyWithAck == 1 && replyWithOffer == 0)
        {
            char responseBuffer[BUFFER_SIZE]; // uwaga niepotrzebnie duże użycie pamięci tutaj
            responseBuffer[0] = 0x02;         // bootreply OP
            responseBuffer[1] = 0x01;         // ethernet HTYPE
            responseBuffer[2] = 0x06;         // długość 6 HLEN
            responseBuffer[3] = 0x00;         // HOPS nie wiem co to właściwie

            // unsigned int transactionId = generateRandomHex(); // 4 bajty
            // responseBuffer[4] = (transactionId >> 24) & 0xFF; // 1 bajt
            // responseBuffer[5] = (transactionId >> 16) & 0xFF; // 2 bajt
            // responseBuffer[6] = (transactionId >> 8) & 0xFF;  // 3 bajt
            // responseBuffer[7] = transactionId & 0xFF;         // 4 bajt
            responseBuffer[4] = transactionId[0];
            responseBuffer[5] = transactionId[1];
            responseBuffer[6] = transactionId[2];
            responseBuffer[7] = transactionId[3];

            responseBuffer[8] = 0x00; // sekundy które upłynęły
            responseBuffer[9] = 0x00; // nie jest to ważne a zajmuje 2 bajty :skull:

            responseBuffer[10] = 0x00; // flagi bootp
            responseBuffer[11] = 0x00; // nie jest to ważne a zajmuje 2 bajty :skull:

            responseBuffer[12] = (client_addr.sin_addr.s_addr) & 0xFF;      // adres ip klienta cokolwiek to znaczy
            responseBuffer[13] = (client_addr.sin_addr.s_addr >> 8) & 0xFF; // zazwyczaj są tu zera
            responseBuffer[14] = (client_addr.sin_addr.s_addr >> 16) & 0xFF;
            responseBuffer[15] = (client_addr.sin_addr.s_addr >> 24) & 0xFF;

            responseBuffer[16] = requestedIP[0];
            responseBuffer[17] = requestedIP[1];
            responseBuffer[18] = requestedIP[2];
            responseBuffer[19] = requestedIP[3];

            struct sockaddr_in fake_client_addr2;
            fake_client_addr2.sin_family = AF_INET;
            fake_client_addr2.sin_port = client_addr.sin_port;
            // fake_client_addr2.sin_addr.s_addr = INADDR_BROADCAST;
            // inet_pton(AF_INET, requestedIP, &(fake_client_addr2.sin_addr));
            struct in_addr addr;
            unsigned char broadcast[4] = {
                offered_subnet_base[0],
                offered_subnet_base[1],
                offered_subnet_base[2],
                0xFF,
            };
            addr.s_addr = *((uint32_t *)broadcast);
            fake_client_addr2.sin_addr = addr;
            client_addr = fake_client_addr2;
            printf("%s\n", inet_ntoa(client_addr.sin_addr));
            // client_addr = fake_client_addr2;
            client_addr_len = sizeof(client_addr);
            // client_addr = INADDR_BROADCAST;

            responseBuffer[20] = 0x00; // następny serwer
            responseBuffer[21] = 0x00;
            responseBuffer[22] = 0x00;
            responseBuffer[23] = 0x00;

            responseBuffer[24] = 0x00; // agent przekładni ???
            responseBuffer[25] = 0x00;
            responseBuffer[26] = 0x00;
            responseBuffer[27] = 0x00;

            int combinedTransactionIdAsInt = combinedTransactionIdAsCharArray[0] + combinedTransactionIdAsCharArray[1] + combinedTransactionIdAsCharArray[2] + combinedTransactionIdAsCharArray[3];
            unsigned char *targetMAC = (unsigned char *)getValueByKey(&transactionIDsToMACs, combinedTransactionIdAsInt); // adres MAC naszego klienta
            responseBuffer[28] = targetMAC[0];
            responseBuffer[29] = targetMAC[1];
            responseBuffer[30] = targetMAC[2];
            responseBuffer[31] = targetMAC[3];
            responseBuffer[32] = targetMAC[4];
            responseBuffer[33] = targetMAC[5];

            for (size_t i = 0; i < 10; i++)
            {
                responseBuffer[34 + i] = 0; // padding adresu MAC
            }

            for (size_t i = 0; i < 64; i++)
            {
                responseBuffer[44 + i] = 0; // hostname serwera
            }

            for (size_t i = 0; i < 128; i++)
            {
                responseBuffer[108 + i] = 0; // "boot filename" cokolwiek to jest
            }

            for (size_t i = 0; i < 4; i++)
            {
                responseBuffer[236 + i] = DHCP_MagicCookie[i]; // magiczne ciastko
            }

            // addOption(responseBuffer, 240, MessageType, (char[]){Ack}, 1); // dodajemy opcje Ack

            // responseBuffer[243] = End; // koniec

            // for (size_t i = 244; i < BUFFER_SIZE; i++)
            // {
            //     responseBuffer[i] = 0;
            // }

            // if (sendto(sockfd, responseBuffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, client_addr_len) == -1)
            // {
            //     perror("Error sending response");
            //     exit(EXIT_FAILURE);
            // }
            int messageTypeEnd = addOption(responseBuffer, 240, MessageType, (char[]){Ack}, 1); // dodajemy opcje ack
            char offeredSubnetMaskCopy[4] = {
                offered_subnet_mask[0],
                offered_subnet_mask[1],
                offered_subnet_mask[2],
                offered_subnet_mask[3],
            };
            int subnetMaskEnd = addOption(responseBuffer, messageTypeEnd, SubnetMask, offeredSubnetMaskCopy, 4);
            int serverIdEnd = addOption(responseBuffer, subnetMaskEnd, ServerID, serverIp, 4);
            // char leaseTime[4] = {0x01, 0xE1, 0x33, 0x80};
            char leaseTime[4] = {
                (LEASE_TIME >> 24) & 0xFF,
                (LEASE_TIME >> 16) & 0xFF,
                (LEASE_TIME >> 8) & 0xFF,
                LEASE_TIME & 0xFF,
            };
            int leaseTimeEnd = addOption(responseBuffer, serverIdEnd, IPAdressLeaseTime, leaseTime, 4);
            int finalEnd = leaseTimeEnd;

            responseBuffer[finalEnd] = End; // koniec odpowiedzi

            for (size_t i = finalEnd + 1; i < BUFFER_SIZE; i++)
            {
                responseBuffer[i] = 0;
            }
            printf("sending %d\n", responseBuffer[242]);
            if (sendto(sockfd, responseBuffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, client_addr_len) == -1)
            {
                perror("Error sending response");
                exit(EXIT_FAILURE);
            }

            removeByKey(&transactionIDsToMACs, combinedTransactionIdAsInt);
            // memset(0, transactionIDsToMACs.data, MAX_KEYS);
            initializeMap(&transactionIDsToMACs);

            DHCP_Lease lease;
            // lease.IP_addr = requestedIP;
            // lease.MAC_addr = targetMAC;
            for (size_t i = 0; i < 4; i++)
            {
                lease.IP_addr[i] = requestedIP[i];
            }
            for (size_t i = 0; i < 6; i++)
            {
                lease.MAC_addr[i] = targetMAC[i];
            }
            lease.start = time(NULL);
            lease.duration = LEASE_TIME;
            lease.valid = 1;
            lease.hostName = requestedHostname;
            // targetMAC[6] = clients.size;

            // pthread_mutex_lock(&leaseMutex);
            // int offset = clients.size > 0 ? (clients.size + 1) : 1;
            // int targetMAC_asInt = (targetMAC[0] + targetMAC[1] + targetMAC[2] + targetMAC[3] + targetMAC[4] + targetMAC[5]) * offset;
            // void *val = getValueByKey(&clients, targetMAC_asInt);
            // // if (val == NULL)
            // insertKeyValuePair(&clients, targetMAC_asInt, &lease, 6);
            // // else
            // // ((struct DHCP_Lease *)val)->start = time(NULL);
            // pthread_mutex_unlock(&leaseMutex);
            addLease(lease);
            continue;
        }
        printf("removing leftover transaction id\n");
        int combinedTransactionIdAsInt = combinedTransactionIdAsCharArray[0] + combinedTransactionIdAsCharArray[1] + combinedTransactionIdAsCharArray[2] + combinedTransactionIdAsCharArray[3];
        removeByKey(&transactionIDsToMACs, combinedTransactionIdAsInt);
    }
    // pthread_join(leasesThread, NULL);
    // close(sockfd);
    cleanup();

    return 0;
}
