#include "./SerialPort/SerialPort.hpp"
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <process.h>
#include <time.h>

#define MAX_DATA_LENGTH 512
#define MAX_INPUT_LENGTH 256

// Global variables for thread communication
SerialPort* arduino = nullptr;
bool programRunning = true;
bool logToFile = false;
FILE* logFile = nullptr;
char inputBuffer[MAX_INPUT_LENGTH] = {0};
int inputBufferLength = 0;
HANDLE inputMutex;
HANDLE consoleMutex;
DWORD currentBaudRate = CBR_9600; // Store current baud rate

// COM port receive buffer
char comReceiveBuffer[MAX_DATA_LENGTH] = {0};
int comReceiveBufferLength = 0;

// Function to get current timestamp
void getCurrentTimestamp(char* timestamp, size_t size) {
    time_t now;
    struct tm* timeinfo;
    time(&now);
    timeinfo = localtime(&now);
    strftime(timestamp, size, "[%Y-%m-%d %H:%M:%S]", timeinfo);
}

// Console management functions using Windows Console API
void clearCurrentLine() {
    CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    
    if (GetConsoleScreenBufferInfo(hConsole, &consoleInfo)) {
        COORD cursorPos = consoleInfo.dwCursorPosition;
        cursorPos.X = 0;
        SetConsoleCursorPosition(hConsole, cursorPos);
        
        // Fill the line with spaces to clear it
        DWORD charsWritten;
        FillConsoleOutputCharacterA(hConsole, ' ', consoleInfo.dwSize.X, cursorPos, &charsWritten);
        
        // Reset cursor to beginning of line
        SetConsoleCursorPosition(hConsole, cursorPos);
    }
}

void saveCurrentInputAndClear() {
    // Clear current line using Windows Console API
    clearCurrentLine();
}

void restoreInputLine() {
    // Print the input prompt and any text the user has typed
    printf("> %.*s", inputBufferLength, inputBuffer);
    fflush(stdout);
}

// Thread-safe console output with input line management
void safeConsolePrintWithRestore(const char* message) {
    WaitForSingleObject(consoleMutex, INFINITE);
    
    // Clear current line (which contains the input prompt and user text)
    saveCurrentInputAndClear();
    
    // Print the COM port message
    printf("%s", message);
    
    // Restore the input line with user's partial input
    restoreInputLine();
    
    ReleaseMutex(consoleMutex);
}

// Thread-safe console output (for system messages that don't need input restoration)
void safeConsolePrint(const char* message) {
    WaitForSingleObject(consoleMutex, INFINITE);
    printf("%s", message);
    fflush(stdout);
    ReleaseMutex(consoleMutex);
}

// Thread-safe file logging
void logToFileIfEnabled(const char* message) {
    if (logToFile && logFile) {
        fprintf(logFile, "%s", message);
        fflush(logFile);
    }
}

// Function to process and display complete lines from COM port
void processCompleteComLine(char* lineData, int length) {
    char logMessage[MAX_DATA_LENGTH + 64] = {0};
    char timestamp[32] = {0};
    
    // Null-terminate the line
    lineData[length] = '\0';
    
    // Remove trailing carriage return if present
    if (length > 0 && lineData[length - 1] == '\r') {
        lineData[length - 1] = '\0';
        length--;
    }
    
    // Only process if we have actual data
    if (length > 0) {
        // Create timestamped log entry
        getCurrentTimestamp(timestamp, sizeof(timestamp));
        snprintf(logMessage, sizeof(logMessage), "%s RX: %s\n", timestamp, lineData);
        
        // Output to console with sophisticated line management and log file
        safeConsolePrintWithRestore(logMessage);
        logToFileIfEnabled(logMessage);
    }
}

// COM port reading thread
unsigned __stdcall comPortThread(void* pArguments) {
    char incomingData[MAX_DATA_LENGTH] = {0};
    
    while (programRunning && arduino && arduino->isConnected()) {
        int bytesRead = arduino->readSerialPort(incomingData, MAX_DATA_LENGTH - 1);
        
        if (bytesRead > 0) {
            // Process each received byte
            for (int i = 0; i < bytesRead; i++) {
                char currentChar = incomingData[i];
                
                // Check for end of line characters
                if (currentChar == '\n') {
                    // Process the complete line
                    processCompleteComLine(comReceiveBuffer, comReceiveBufferLength);
                    
                    // Reset buffer
                    comReceiveBufferLength = 0;
                    memset(comReceiveBuffer, 0, sizeof(comReceiveBuffer));
                } 
                else if (currentChar != '\r') { // Ignore carriage returns, handle them in processing
                    // Add character to buffer if there's space
                    if (comReceiveBufferLength < MAX_DATA_LENGTH - 1) {
                        comReceiveBuffer[comReceiveBufferLength++] = currentChar;
                    } else {
                        // Buffer is full, process it as a complete line
                        processCompleteComLine(comReceiveBuffer, comReceiveBufferLength);
                        
                        // Reset buffer and add the current character
                        comReceiveBufferLength = 0;
                        memset(comReceiveBuffer, 0, sizeof(comReceiveBuffer));
                        comReceiveBuffer[comReceiveBufferLength++] = currentChar;
                    }
                }
            }
        }
        
        Sleep(10); // Small delay to prevent excessive CPU usage
    }
    
    // Process any remaining data in buffer when thread exits
    if (comReceiveBufferLength > 0) {
        processCompleteComLine(comReceiveBuffer, comReceiveBufferLength);
    }
    
    _endthreadex(0);
    return 0;
}

// Input handling thread using Windows Console API
unsigned __stdcall inputThread(void* pArguments) {
    char logMessage[MAX_DATA_LENGTH + 64] = {0};
    char timestamp[32] = {0};
    
    HANDLE hStdin;
    DWORD fdwSaveOldMode, fdwMode, cNumRead;
    INPUT_RECORD irInBuf[128];
    DWORD bufferSize = 0;
    
    safeConsolePrint("Input thread started. Type commands and press Enter to send.\n");
    safeConsolePrint("Special commands: 'quit' to exit, 'log on/off' to toggle file logging\n");
    safeConsolePrint("Press ESC to quit.\n");
    safeConsolePrint("> ");
    
    // Get stdin handle
    hStdin = GetStdHandle(STD_INPUT_HANDLE);
    if (hStdin == INVALID_HANDLE_VALUE) {
        safeConsolePrint("Error: Invalid stdin handle.\n");
        _endthreadex(0);
        return 0;
    }
    
    // Save current console mode
    if (!GetConsoleMode(hStdin, &fdwSaveOldMode)) {
        safeConsolePrint("Error: GetConsoleMode failed.\n");
        _endthreadex(0);
        return 0;
    }
    
    // Set console mode for reading input events
    fdwMode = ENABLE_WINDOW_INPUT | ENABLE_PROCESSED_INPUT;
    if (!SetConsoleMode(hStdin, fdwMode)) {
        safeConsolePrint("Error: SetConsoleMode failed.\n");
        _endthreadex(0);
        return 0;
    }
    
    while (programRunning && !(GetAsyncKeyState(VK_ESCAPE) & 0x0001)) {
        GetNumberOfConsoleInputEvents(hStdin, &bufferSize);
        
        if (bufferSize > 0) {
            if (ReadConsoleInput(hStdin, irInBuf, 128, &cNumRead)) {
                for (DWORD i = 0; i < cNumRead; i++) {
                    if (irInBuf[i].EventType == KEY_EVENT) {
                        KEY_EVENT_RECORD ker = irInBuf[i].Event.KeyEvent;
                        
                        if (ker.bKeyDown && ker.uChar.AsciiChar != 0) {
                            WaitForSingleObject(inputMutex, INFINITE);
                            
                            if (ker.uChar.AsciiChar == 13) { // Enter key
                                inputBuffer[inputBufferLength] = '\0';
                                
                                // Clear the current input line before processing
                                WaitForSingleObject(consoleMutex, INFINITE);
                                clearCurrentLine();
                                ReleaseMutex(consoleMutex);
                                
                                // Handle special commands
                                if (strcmp(inputBuffer, "quit") == 0) {
                                    programRunning = false;
                                    safeConsolePrint("Shutting down...\n");
                                } else if (strcmp(inputBuffer, "log on") == 0) {
                                    if (!logToFile) {
                                        char filename[64];
                                        time_t now = time(NULL);
                                        struct tm* t = localtime(&now);
                                        strftime(filename, sizeof(filename), "comlog_%Y%m%d_%H%M%S.txt", t);
                                        
                                        logFile = fopen(filename, "w");
                                        if (logFile) {
                                            logToFile = true;
                                            char msg[128];
                                            snprintf(msg, sizeof(msg), "Logging enabled. Writing to: %s\n", filename);
                                            safeConsolePrint(msg);
                                        } else {
                                            safeConsolePrint("Error: Could not create log file.\n");
                                        }
                                    } else {
                                        safeConsolePrint("Logging is already enabled.\n");
                                    }
                                } else if (strcmp(inputBuffer, "log off") == 0) {
                                    if (logToFile) {
                                        logToFile = false;
                                        if (logFile) {
                                            fclose(logFile);
                                            logFile = nullptr;
                                        }
                                        safeConsolePrint("Logging disabled.\n");
                                    } else {
                                        safeConsolePrint("Logging is already disabled.\n");
                                    }
                                } else if (strcmp(inputBuffer, "baud") == 0) {
                                    char baudMsg[128];
                                    snprintf(baudMsg, sizeof(baudMsg), "Current baud rate: %lu\n", currentBaudRate);
                                    safeConsolePrint(baudMsg);
                                } else if (inputBufferLength > 0) {
                                    // Send command to COM port
                                    if (arduino && arduino->isConnected()) {
                                        // Add newline to command
                                        char command[MAX_INPUT_LENGTH + 2];
                                        snprintf(command, sizeof(command), "%s\n", inputBuffer);
                                        
                                        if (arduino->writeSerialPort(command, strlen(command))) {
                                            // Log sent command
                                            getCurrentTimestamp(timestamp, sizeof(timestamp));
                                            snprintf(logMessage, sizeof(logMessage), "%s TX: %s\n", timestamp, inputBuffer);
                                            safeConsolePrint(logMessage);
                                            logToFileIfEnabled(logMessage);
                                        } else {
                                            safeConsolePrint("Error: Failed to send command.\n");
                                        }
                                    } else {
                                        safeConsolePrint("Error: COM port not connected.\n");
                                    }
                                }
                                
                                // Clear input buffer and show new prompt
                                inputBufferLength = 0;
                                memset(inputBuffer, 0, sizeof(inputBuffer));
                                safeConsolePrint("> ");
                                
                            } else if (ker.uChar.AsciiChar == 8) { // Backspace
                                if (inputBufferLength > 0) {
                                    inputBufferLength--;
                                    inputBuffer[inputBufferLength] = '\0';
                                    WaitForSingleObject(consoleMutex, INFINITE);
                                    printf("\b \b");
                                    fflush(stdout);
                                    ReleaseMutex(consoleMutex);
                                }
                            } else if (ker.uChar.AsciiChar >= 32 && ker.uChar.AsciiChar <= 126) { // Printable characters
                                if (inputBufferLength < MAX_INPUT_LENGTH - 1) {
                                    inputBuffer[inputBufferLength++] = ker.uChar.AsciiChar;
                                    WaitForSingleObject(consoleMutex, INFINITE);
                                    printf("%c", ker.uChar.AsciiChar);
                                    fflush(stdout);
                                    ReleaseMutex(consoleMutex);
                                }
                            }
                            
                            ReleaseMutex(inputMutex);
                        }
                    }
                }
            }
        }
        
        Sleep(30);
    }
    
    // Restore console mode
    SetConsoleMode(hStdin, fdwSaveOldMode);
    
    if (!programRunning) {
        safeConsolePrint("Input thread: ESC pressed or quit command received.\n");
    }
    
    _endthreadex(0);
    return 0;
}

// Function to list COM devices using Windows command
void listComDevices() {
    printf("Listing COM devices...\n");
    printf("========================================\n");
    
    // Execute the Windows command to list COM devices
    int result = system("(for /f \"tokens=1,* delims==\" %a in ('wmic path win32_pnpentity get caption /format:list ^| find \"(COM\"') do @echo %b)");
    
    if (result != 0) {
        printf("Warning: Could not execute device listing command.\n");
    }
    
    printf("========================================\n\n");
}

// Function to scan for available COM ports
void scanComPorts() {
    printf("Scanning for available COM ports...\n");
    printf("Available COM ports:\n");
    
    for (int i = 1; i <= 20; i++) {
        char portName[16];
        snprintf(portName, sizeof(portName), "\\\\.\\COM%d", i);
        
        HANDLE testHandle = CreateFileA(portName,
            GENERIC_READ | GENERIC_WRITE,
            0, NULL, OPEN_EXISTING, 0, NULL);
            
        if (testHandle != INVALID_HANDLE_VALUE) {
            printf("  COM%d - Available\n", i);
            CloseHandle(testHandle);
        } else if (GetLastError() == ERROR_ACCESS_DENIED) {
            printf("  COM%d - In use\n", i);
        }
    }
    printf("\n");
}

// Function to display available baud rates
void displayBaudRates() {
    printf("Available baud rates:\n");
    printf("  1. 9600 (default)\n");
    printf("  2. 19200\n");
    printf("  3. 38400\n");
    printf("  4. 57600\n");
    printf("  5. 115200\n");
    printf("  6. 230400\n");
    printf("  7. 460800\n");
    printf("  8. 921600\n");
    printf("  9. Custom\n");
}

// Function to get baud rate from user selection
DWORD getBaudRateFromUser() {
    int choice;
    DWORD customBaud;
    
    displayBaudRates();
    printf("Select baud rate (1-9): ");
    
    if (scanf("%d", &choice) != 1) {
        printf("Invalid input! Using default 9600.\n");
        while (getchar() != '\n'); // Clear input buffer
        return CBR_9600;
    }
    
    // Clear input buffer
    while (getchar() != '\n');
    
    switch (choice) {
        case 1: return CBR_9600;
        case 2: return CBR_19200;
        case 3: return CBR_38400;
        case 4: return CBR_57600;
        case 5: return CBR_115200;
        case 6: return 230400;
        case 7: return 460800;
        case 8: return 921600;
        case 9:
            printf("Enter custom baud rate: ");
            if (scanf("%lu", &customBaud) == 1 && customBaud > 0) {
                while (getchar() != '\n'); // Clear input buffer
                printf("Using custom baud rate: %lu\n", customBaud);
                return customBaud;
            } else {
                printf("Invalid baud rate! Using default 9600.\n");
                while (getchar() != '\n'); // Clear input buffer
                return CBR_9600;
            }
        default:
            printf("Invalid choice! Using default 9600.\n");
            return CBR_9600;
    }
}

// Function to connect to COM port
SerialPort* connectToComPort() {
    int portNumber;
    char portName[16];
    DWORD baudRate;
    
    printf("Enter COM port number (1-20): ");
    if (scanf("%d", &portNumber) != 1 || portNumber < 1 || portNumber > 20) {
        printf("Invalid port number!\n");
        while (getchar() != '\n'); // Clear input buffer
        return nullptr;
    }
    
    // Clear input buffer
    while (getchar() != '\n');
    
    // Get baud rate selection
    baudRate = getBaudRateFromUser();
    
    snprintf(portName, sizeof(portName), "\\\\.\\COM%d", portNumber);
    printf("Attempting to connect to %s at %lu baud...\n", portName, baudRate);
    
    SerialPort* serial = new SerialPort(portName, baudRate);
    
    if (serial->isConnected()) {
        currentBaudRate = baudRate; // Store the baud rate globally
        printf("Successfully connected to %s at %lu baud\n", portName, baudRate);
        return serial;
    } else {
        printf("Failed to connect to %s at %lu baud (Error: %d)\n", portName, baudRate, serial->getError());
        delete serial;
        return nullptr;
    }
}

int main() {
    printf("========================================\n");
    printf("       COM Port Logger with CLI        \n");
    printf("========================================\n\n");
    
    // Initialize mutexes
    inputMutex = CreateMutex(NULL, FALSE, NULL);
    consoleMutex = CreateMutex(NULL, FALSE, NULL);
    
    if (!inputMutex || !consoleMutex) {
        printf("Failed to create mutexes!\n");
        return -1;
    }
    
    // List COM devices using Windows command
    listComDevices();
    
    // Scan for available ports
    //scanComPorts();
    
    // Connect to COM port
    arduino = connectToComPort();
    if (!arduino) {
        printf("Press any key to exit...\n");
        getchar();
        return -1;
    }
    
    printf("\nStarting COM port logger...\n");
    printf("Commands:\n");
    printf("  'quit' - Exit program\n");
    printf("  'log on' - Enable logging to file\n");
    printf("  'log off' - Disable logging\n");
    printf("  'baud' - Display current baud rate\n");
    printf("  Any other text - Send to COM port\n");
    printf("========================================\n\n");
    
    // Start threads
    HANDLE hComThread = (HANDLE)_beginthreadex(NULL, 0, &comPortThread, NULL, 0, NULL);
    HANDLE hInputThread = (HANDLE)_beginthreadex(NULL, 0, &inputThread, NULL, 0, NULL);
    
    if (!hComThread || !hInputThread) {
        printf("Failed to create threads!\n");
        if (arduino) {
            delete arduino;
        }
        return -1;
    }
    
    // Wait for threads to complete
    HANDLE threads[] = {hComThread, hInputThread};
    WaitForMultipleObjects(2, threads, TRUE, INFINITE);
    
    // Cleanup
    CloseHandle(hComThread);
    CloseHandle(hInputThread);
    
    if (arduino) {
        delete arduino;
    }
    
    if (logFile) {
        fclose(logFile);
    }
    
    CloseHandle(inputMutex);
    CloseHandle(consoleMutex);
    
    printf("Program terminated.\n");
    return 0;
}
