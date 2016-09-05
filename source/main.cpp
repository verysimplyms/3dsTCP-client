#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <3ds.h>
#include <ctime>
#include "hbkb.h"

#include <malloc.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <fcntl.h>

//Needed to remove compiler errors -- makefile was also changed
#define _GLIBCXX_USE_CXX11_ABI 0/1
#define SOC_ALIGN       0x1000
#define SOC_BUFFERSIZE  0x100000

void textPrint(std::string givenString, const char* InputCChar, std::vector<std::string> passedStringVector) {

    //30 total lines (from 0 to 29)


    printf("\x1b[1;0H%s %d", givenString.c_str(), strlen(InputCChar));
    printf("\x1b[2;0HInput :");
    printf("\x1b[3;0H%s", InputCChar);

    if(passedStringVector.size() <= 0) {
        passedStringVector[0] = " ";
    }

    for(int i = 0; i < passedStringVector.size(); i++) {

        printf("\x1b[%d;0H%s", (i+5), passedStringVector[i].c_str());
        if(i >= 23) {
            i = 0;
        }
    }
}

int main()
{
	gfxInitDefault();
	//gfxSet3D(true); //Uncomment if using stereoscopic 3D
	consoleInit(GFX_TOP, NULL); //Change this line to consoleInit(GFX_BOTTOM, NULL) if using the bottom screen.
	//unsigned long int t = static_cast<unsigned long int> (std::time(NULL));

	int sock, bytes_recieved, ret, numberOfChatMessages;
    std::vector<std::string> chatMessages;

	char send_data[256], recv_data[256], recv_data_holder[256];
	send_data[0] = recv_data[0] = recv_data_holder[0] = 0;

	struct hostent *host;
	struct sockaddr_in server_addr;
	static u32 *SOC_buffer = NULL;
    numberOfChatMessages = 0;
    // allocate buffer for SOC service
	SOC_buffer = (u32*)memalign(SOC_ALIGN, SOC_BUFFERSIZE);

	if(SOC_buffer == NULL) {
        gfxExit();
        return 0;
	}

	// Now intialise soc:u service
	if ((ret = socInit(SOC_buffer, SOC_BUFFERSIZE)) != 0) {
        gfxExit();
        return 0;
	}

    sock = socket(AF_INET, SOCK_STREAM, 0);

	if(sock == -1) {
        gfxExit();
        return 0;
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(8080);
	server_addr.sin_addr.s_addr = inet_addr("192.168.1.156");
	//bzero(&(server_addr.sin_zero), 8);

	if(connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        gfxExit();
        return 0;
	}

	if(fcntl(sock, F_SETFL, fcntl(sock, F_GETFL, 0) | O_NONBLOCK) == -1) {
        gfxExit();
        return 0;
	}

    strcpy(send_data, "!platform_3ds");
    send(sock, send_data, (strlen(send_data) + 1), 0);

	HB_Keyboard sHBKB;

	// Main loop
	while (aptMainLoop())
	{
		gspWaitForVBlank();
		hidScanInput();
		u32 kDown = hidKeysDown();

		//Your code goes here
        ret = recv(sock, recv_data, sizeof(recv_data), 0);
/*
        if(strcmp(recv_data, recv_data_holder) == 0 && strlen(recv_data) != 0) {
            memset(&recv_data[0], 0, sizeof(recv_data));
            memset(&recv_data_holder[0], 0, sizeof(recv_data_holder));
        } else
*/
         if(strlen(recv_data) > 0){
            consoleClear();
//            strcpy(recv_data_holder, recv_data);
            numberOfChatMessages++;

            if(numberOfChatMessages > 23) {
                chatMessages.clear();
                numberOfChatMessages = 0;
                chatMessages.push_back(recv_data);
                memset(&recv_data[0], 0, sizeof(recv_data));


            } else {
                chatMessages.push_back(recv_data);
                memset(&recv_data[0], 0, sizeof(recv_data));
                numberOfChatMessages++;
            }
        }

		// Touch Stuff
		touchPosition touch;

		//Read the touch screen coordinates
		hidTouchRead(&touch);

		// Call Keyboard with Touch Position
		u8 KBState = sHBKB.HBKB_CallKeyboard(touch);

		std::string InputHBKB = sHBKB.HBKB_CheckKeyboardInput(); // Check Input
		const char* InputCChar = InputHBKB.c_str();

		//Print input

		if (KBState == 1) {
			sHBKB.HBKB_Clean(); // Clean Input
			consoleClear();
			if (strlen(InputCChar) <= 100 && strlen(InputCChar) > 0) {
				textPrint("Keyboard Return = Enter Key Pressed", InputCChar, chatMessages);
				strcpy(send_data, InputCChar);
				send(sock, send_data, (strlen(send_data) + 1), 0);
			}
			else {
                textPrint("Data not sent due to string length", InputCChar, chatMessages);
			}
		}
		else if (KBState == 2) {
            consoleClear();
			if (strlen(InputCChar) <= 100) {
                textPrint("Keyboard Return = Generic Key Pressed", InputCChar, chatMessages);
			} else {
				textPrint("String is too long", InputCChar, chatMessages);
			}
		}
		else if (KBState == 3) {
			sHBKB.HBKB_Clean(); // Clean Input
			textPrint("Keyboard Return = Cancel Button Pressed           ", InputCChar, chatMessages);
		}
		else if (KBState == 4) {
            if (strlen(InputCChar) > 100) {
                textPrint("String is too long (Max is 100 chars)        ", InputCChar, chatMessages);
            } else {
                textPrint("Keyboard Return = No Button Pressed          ", InputCChar, chatMessages);
            }
		}
		else if (KBState == 0)
			printf("\x1b[2;0HKeyboard Return = UNKNOWN (HBKB Error).");
		else
			printf("\x1b[2;0HKeyboard Return = UNKNOWN (main.cpp Error).");

		if (kDown & KEY_START)
			break; //Break in order to return to hbmenu

		// Flush and swap frame-buffers
		gfxFlushBuffers();
		gfxSwapBuffers();
	}

	gfxExit();
	return 0;
}
