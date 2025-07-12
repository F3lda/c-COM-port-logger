#include "./SerialPort/SerialPort.hpp" //https://github.com/manashmndl/SerialPort
#include <windows.h>
#include <stdio.h>
#include <string.h>

#define MAX_DATA_LENGTH 255



#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <windows.h>

void ErrorExit(LPSTR);
void KeyEventProc(KEY_EVENT_RECORD ker);

// Global variables are here for example, avoid that.
DWORD fdwSaveOldMode;
HANDLE hStdin;

void printToCoordinates(int x, int y, char* text)
{
    printf("\033[%d;%dH%s", y, x, text);
}

void goToUpperLine()
{
    CONSOLE_SCREEN_BUFFER_INFO coninfo;
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfo(hConsole, &coninfo);
    coninfo.dwCursorPosition.Y -= 1;
    coninfo.dwCursorPosition.X = 0;
    SetConsoleCursorPosition(hConsole, coninfo.dwCursorPosition);
    printf("                                                \r> ");
}


int stdinBufferLength = 0;
char stdinBuffer[MAX_DATA_LENGTH] = {0};


int ReadStdinLoop(int (newCharCallback)(char, char[]))
{
	//printf("\033[H\033[J");
    int i = 0, j = 0;
    char* s = (char*)"*";

    DWORD fdwMode, cNumRead;
	#define BUFFER_SIZE 128
    INPUT_RECORD irInBuf[BUFFER_SIZE];
    DWORD bufferSize = 0;

    hStdin = GetStdHandle(STD_INPUT_HANDLE);

    // Just a check to ensure everything is fine at this state
    if (hStdin==INVALID_HANDLE_VALUE){
        printf("Invalid handle value.\n");
        exit(EXIT_FAILURE);
    }

    // Just a check to ensure everything is fine at this state
    if (! GetConsoleMode(hStdin, &fdwSaveOldMode) )
        ErrorExit((char*)"GetConsoleMode");

    // Those constants are documented on Microsoft doc
    // ENABLE_PROCESSED_INPUT allows you to use CTRL+C
    // (so it's not catched by ReadConsoleInput here)
    fdwMode = ENABLE_WINDOW_INPUT | ENABLE_PROCESSED_INPUT;
    if (! SetConsoleMode(hStdin, fdwMode) )
        ErrorExit((char*)"SetConsoleMode");


    while (!(GetAsyncKeyState(VK_ESCAPE) & 0x0001)) {
        // The goal of this program is to print a line of stars
        //printToCoordinates(i, 5, s);
        i++;


        GetNumberOfConsoleInputEvents(hStdin, &bufferSize);

        // ReadConsoleInput block if the buffer is empty
        if (bufferSize > 0) {
            if (! ReadConsoleInput(
                    hStdin,      // input buffer handle
                    irInBuf,     // buffer to read into
                    BUFFER_SIZE, // size of read buffer
                    &cNumRead) ) // number of records read
                ErrorExit((char*)"ReadConsoleInput");

            // This code is not rock solid, you should iterate over
            // irInBuf to get what you want, the last event may not contain what you expect
            // Once again you'll find an event constant list on Microsoft documentation
            for (j = 0; j < cNumRead; j++) {
				KEY_EVENT_RECORD ker = irInBuf[j].Event.KeyEvent;
				
				if(irInBuf[j].EventType == KEY_EVENT) {
					if (ker.uChar.AsciiChar != 0) {// char input
						if (ker.bKeyDown) {// key pressed
							if (ker.uChar.AsciiChar == 13) {// ENTER
								printf("\n");
								printf("TYPED: %s\n", stdinBuffer);
								stdinBuffer[0] = '\0';
								stdinBufferLength = 0;
							} else if (ker.uChar.AsciiChar == 8) {// BACKSPACE
								stdinBuffer[--stdinBufferLength] = '\0';
								printf("\b \b");
							} else {
								
								if (stdinBufferLength+1 < MAX_DATA_LENGTH) { // CHAR
									stdinBuffer[stdinBufferLength] = ker.uChar.AsciiChar;
									stdinBuffer[++stdinBufferLength] = '\0';
									printf("%c", ker.uChar.AsciiChar);
									
									newCharCallback(ker.uChar.AsciiChar, stdinBuffer);
								}
							}
						}
					} else {
						
						/*
						printf("Key event: \"%c\" [%u] [%d] (%u) (%u) (%u) (%u)", ker.uChar.AsciiChar, ker.uChar.UnicodeChar, ker.uChar.AsciiChar, ker.dwControlKeyState, ker.wVirtualKeyCode, ker.wVirtualScanCode, ker.wRepeatCount);

						if(ker.bKeyDown)
							printf("key pressed\n");
						else printf("key released\n");
						*/
						
					}
					
				}
				
				
				
                //KeyEventProc(irInBuf[j].Event.KeyEvent);
                //Sleep(2000);
            }
        }

        Sleep(30);
    }
    // Setting the console back to normal
    SetConsoleMode(hStdin, fdwSaveOldMode);
    CloseHandle(hStdin);

    printf("\nFIN\n");

    return 0;
}



int stdinNewChar(char ch, char str[])
{
	
	printf("{%c}",ch);
	return 0;
}



int main()
{
    
	ReadStdinLoop(stdinNewChar);
	
	
	return 0;
}

void ErrorExit (LPSTR lpszMessage)
{
    fprintf(stderr, "%s\n", lpszMessage);

    // Restore input mode on exit.

    SetConsoleMode(hStdin, fdwSaveOldMode);

    ExitProcess(0);
}

void KeyEventProc(KEY_EVENT_RECORD ker)
{
	//ker.uChar.AsciiChar != 0
    //printf("Key event: \"%c\" [%u] [%d] (%u) (%u) (%u) (%u)", ker.uChar.AsciiChar, ker.uChar.UnicodeChar, ker.uChar.AsciiChar, ker.dwControlKeyState, ker.wVirtualKeyCode, ker.wVirtualScanCode, ker.wRepeatCount);

    //if(ker.bKeyDown)
    //    printf("key pressed\n");
    //else printf("key released\n");
	
	
	if (ker.uChar.AsciiChar != 0) {// char input
		if (ker.bKeyDown) {// key pressed
			if (ker.uChar.AsciiChar == 13) {// ENTER
				printf("\n");
				printf("TYPED: %s\n", stdinBuffer);
				stdinBuffer[0] = '\0';
				stdinBufferLength = 0;
			} else if (ker.uChar.AsciiChar == 8) {// BACKSPACE
				stdinBuffer[--stdinBufferLength] = '\0';
				printf("\b \b");
			} else {
				
				if (stdinBufferLength+1 < MAX_DATA_LENGTH) { // CHAR
					stdinBuffer[stdinBufferLength] = ker.uChar.AsciiChar;
					stdinBuffer[++stdinBufferLength] = '\0';
					printf("%c", ker.uChar.AsciiChar);
				}
			}
		}
	} else {
		
		/*
		printf("Key event: \"%c\" [%u] [%d] (%u) (%u) (%u) (%u)", ker.uChar.AsciiChar, ker.uChar.UnicodeChar, ker.uChar.AsciiChar, ker.dwControlKeyState, ker.wVirtualKeyCode, ker.wVirtualScanCode, ker.wRepeatCount);

		if(ker.bKeyDown)
		    printf("key pressed\n");
		else printf("key released\n");
		*/
		
	}
	
	
	
	/*
	typedef struct _KEY_EVENT_RECORD {
  BOOL  bKeyDown;
  WORD  wRepeatCount;
  WORD  wVirtualKeyCode;
  WORD  wVirtualScanCode;
  union {
    WCHAR UnicodeChar;
    CHAR  AsciiChar;
  } uChar;
  DWORD dwControlKeyState;
} KEY_EVENT_RECORD;
*/
}



/*
// Get line from stream using fgets() function
int fgetls(char *line_buffer, int line_buffer_size, FILE *stream) // stream_buffer_size = line_buffer_size
{
    if (line_buffer == NULL) return -2;
    if (line_buffer_size < 2) return -3;
    if (stream == NULL) return -4;
    if (fgets(line_buffer, line_buffer_size, stream) == NULL) {
        if (feof(stream)) {
            return EOF; // End Of File
        }
        return 1; // unknown error
    }
    size_t len = strlen(line_buffer);
    if(line_buffer[len-1] == '\n') {
        line_buffer[len-1] = '\0';
        return '\n'; // Newline char read/reached
    }
    return 0; // stream is not empty
}



// crt_begthrdex.cpp
// compile with: /MT
#include <windows.h>
#include <stdio.h>
#include <process.h>



    unsigned threadID;






unsigned End = 0;
unsigned __stdcall SecondThreadFunc( void* pArguments )
{
    printf( "In second thread...\n" );
	
	char line_buffer[255];
	
    while (1) {
		printf("\n >");
		if (fgetls(line_buffer, 255, stdin) == '\n') {
			printf("TYPED: [%s]\n", line_buffer);
		} else {
			printf("End of line reached!\n");
		}
		Sleep(1000);
	}
	
	End = 1;
	
	
    _endthreadex( 0 );
    return 0;
}


unsigned __stdcall ThreadFunc( void* pArguments )
{
    printf( "In second thread...\n" );

    while (!(GetAsyncKeyState(VK_ESCAPE) & 0x0001)) {
		printf("TEST\n");
		Sleep(1000);
	}
	
	PostThreadMessage(threadID, WM_QUIT, (WPARAM) NULL, (LPARAM) NULL);

    _endthreadex( 0 );
    return 0;
}

int main()
{
    HANDLE hThread;
    HANDLE hThread2;
    unsigned threadID2;

    printf( "Creating second thread...\n" );

    // Create the second thread.
    hThread = (HANDLE)_beginthreadex( NULL, 0, &SecondThreadFunc, NULL, 0, &threadID );
    hThread2 = (HANDLE)_beginthreadex( NULL, 0, &ThreadFunc, NULL, 0, &threadID2 );

    // Wait until second thread terminates. If you comment out the line
    // below, Counter will not be correct because the thread has not
    // terminated, and Counter most likely has not been incremented to
    // 1000000 yet.
    WaitForSingleObject( hThread, INFINITE );
    WaitForSingleObject( hThread2, INFINITE );
    // Destroy the thread object.
    CloseHandle( hThread );
    CloseHandle( hThread2);
}

*/



/*
void goToUpperLine();

int main()
{
    SetConsoleTitle("Arduino serial communication");
    printf("|------------------------------|\n");
    printf("|    ARDUINO MYSTERY DEVICE    |\n");
    printf("|------------------------------|\n\n> ");
	
	
	system("(for /f \"tokens=1,* delims==\" %a in ('wmic path win32_pnpentity get caption /format:list ^| find \"(COM\"') do @echo %b)");
	system("pause");

    char incomingData[MAX_DATA_LENGTH] = "ahoj";
    //Check arduino code
    char handshakeMessage[] = "ARDUINO_IMD\n";
    //Control signals for turning on and turning off the led
    char ledON[] = "ON\n";
    char ledOFF[] = "OFF\n";
    //Arduino SerialPort object
    SerialPort *arduino;

    int portNumber = 1;
    char portName[16];
    do{
        sprintf(portName, "\\\\.\\COM%d", portNumber++);
        arduino = new SerialPort(portName);
        if(arduino->isConnected()){
            printf("Connection established at port %s\n> Waiting for identification...\n", portName);
            arduino->writeSerialPort(handshakeMessage, MAX_DATA_LENGTH);
            int handshakeTimeout = 300;
            char *pos;
            if((pos = strchr(handshakeMessage, '\n')) != NULL) *pos = '\0';
            while(arduino->isConnected() && handshakeTimeout > 0){
                arduino->readSerialPort(incomingData, MAX_DATA_LENGTH);
                if((pos = strchr(incomingData, '\n')) != NULL) *pos = '\0';
                if(strcmp(handshakeMessage,incomingData) == 0){
                    handshakeTimeout = 0;
                    printf("> OK\n");
                }
                handshakeTimeout--;
                Sleep(10);
            }
            if(handshakeTimeout == 0){
                arduino->Disconnect();
                printf("> Device at port %s isn't INSTAGRAM MYSTERY DEVICE!\n", portName);
            }
        }
        if(arduino->getError() == -1) goToUpperLine();
        if(portNumber > 20){printf("Device not found!\n"); system("pause"); return -1;}
    }while(arduino->getError() || !arduino->isConnected());

    printf("> Commands: 0 - OFF, 1 - ON\n> ------------------------------\n> ");

    while(arduino->isConnected() && !(GetAsyncKeyState(VK_ESCAPE) & 0x8000)){
        int readBytes = arduino->readSerialPort(incomingData, MAX_DATA_LENGTH);
        char *pos;
        if((pos = strchr(incomingData, '\n')) != NULL) *pos = '\0';
        if(readBytes > 0) printf("%s\n> ", incomingData);

        if(GetAsyncKeyState(VK_ESCAPE) & 0x8000){
            arduino->Disconnect();
        } else if(GetAsyncKeyState(VK_NUMPAD0) & 0x8000){
            printf("LED OFF\n> ");
            arduino->writeSerialPort(ledOFF, MAX_DATA_LENGTH);
            Sleep(750);
        } else if(GetAsyncKeyState(VK_NUMPAD1) & 0x8000){
            printf("LED ON\n> ");
            arduino->writeSerialPort(ledON, MAX_DATA_LENGTH);
            Sleep(750);
        }
        Sleep(150);
    }

    delete arduino;
    return 0;
}

void goToUpperLine()
{
    CONSOLE_SCREEN_BUFFER_INFO coninfo;
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfo(hConsole, &coninfo);
    coninfo.dwCursorPosition.Y -= 1;
    coninfo.dwCursorPosition.X = 0;
    SetConsoleCursorPosition(hConsole, coninfo.dwCursorPosition);
    printf("                                                \r> ");
}
*/