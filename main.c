#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netpacket/packet.h>
#include <net/if.h>
#include <netinet/if_ether.h>

#define PORT 67
// #define PORT 3000
// #define BUFFER_SIZE 1024
#define BUFFER_SIZE 600
#include <time.h>
#include "map.h"

// TODO: lista klientów
// TODO: obsłużyć release i decline
// TODO: pula adresów

unsigned int generateRandomHex()
{
    srand((unsigned int)time(NULL));
    return rand() & 0xFFFFFFFF;
}

// const char *ip_str = "192.168.1.2";
// const char *serverIp = "192.168.1.1";
// const unsigned char serverIp[4] = {0xC0, 0xA8, 0x01, 0x01};
const unsigned char offered_subnet_base[4] = {0xC0, 0xA8, 0x01, 0x00};
const unsigned char offered_subnet_mask[4] = {0xFF, 0xFF, 0xFF, 0x00};

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

int main()
{
    unsigned char serverIp[4] = {
        offered_subnet_base[0],
        offered_subnet_base[1],
        offered_subnet_base[2],
        0x01,
    };
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    int broadcastEnable = 1;
    int ret = setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));

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
        };
        unsigned int combinedTransactionId = combineBytes(transactionId, 4);
        unsigned char combinedTransactionIdAsCharArray[8];
        intToHex(combinedTransactionId, combinedTransactionIdAsCharArray);

        char requestedIP[4];

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
            unsigned char MAC_octets[6];
            for (int i = 0; i < 6; ++i)
            {
                // sprintf(MAC_octets, "%02x", (unsigned char)buffer[28 + i]);
                MAC_octets[i] = (unsigned char)buffer[28 + i]; // end at 33
                // printf("MAC_octets end at %d\n", 28 + i);
            }
            insertKeyValuePair(&transactionIDsToMACs, combinedTransactionIdAsCharArray, MAC_octets);

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
                0x02,
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
                inet_pton(AF_INET, "192.168.1.255", &(fake_client_addr2.sin_addr)); // broadcast tej sieci
                client_addr = fake_client_addr2;
                client_addr_len = sizeof(client_addr);
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

            unsigned char *targetMAC = getValueByKey(&transactionIDsToMACs, combinedTransactionIdAsCharArray); // adres MAC naszego klienta
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
            int subnetMaskEnd = addOption(responseBuffer, messageTypeEnd, SubnetMask, offered_subnet_mask, 4);
            int serverIdEnd = addOption(responseBuffer, subnetMaskEnd, ServerID, serverIp, 4);
            int finalEnd = serverIdEnd;

            // responseBuffer[243] = End; // koniec odpowiedzi
            // responseBuffer[249] = End; // koniec odpowiedzi
            responseBuffer[finalEnd] = End; // koniec odpowiedzi

            // for (size_t i = 244; i < BUFFER_SIZE; i++)
            // for (size_t i = 250; i < BUFFER_SIZE; i++)
            for (size_t i = finalEnd + 1; i < BUFFER_SIZE; i++)
            {
                responseBuffer[i] = 0;
            }

            if (sendto(sockfd, responseBuffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, client_addr_len) == -1)
            {
                perror("Error sending response");
                exit(EXIT_FAILURE);
            }
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
            inet_pton(AF_INET, requestedIP, &(fake_client_addr2.sin_addr));
            client_addr = fake_client_addr2;
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

            unsigned char *targetMAC = getValueByKey(&transactionIDsToMACs, combinedTransactionIdAsCharArray); // adres MAC naszego klienta
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
            int subnetMaskEnd = addOption(responseBuffer, messageTypeEnd, SubnetMask, offered_subnet_mask, 4);
            int serverIdEnd = addOption(responseBuffer, subnetMaskEnd, ServerID, serverIp, 4);
            int finalEnd = serverIdEnd;

            responseBuffer[finalEnd] = End; // koniec odpowiedzi

            for (size_t i = finalEnd + 1; i < BUFFER_SIZE; i++)
            {
                responseBuffer[i] = 0;
            }

            if (sendto(sockfd, responseBuffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, client_addr_len) == -1)
            {
                perror("Error sending response");
                exit(EXIT_FAILURE);
            }
        }
    }

    close(sockfd);

    return 0;
}
