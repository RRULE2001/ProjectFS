#include <stdio.h>
#include <stdint.h>
#include <windows.h>

#define COMPORT "\\\\.\\COM7"
#define BAUDRATE 115200
#define DEADZONE 10
#define PITCH_MODIFIER 1
#define ROLL_MODIFIER 1
#define YAW_MODIFIER 1

void print_error(const char * context)
{
    DWORD error_code = GetLastError();
    char buffer[256];
    DWORD size = FormatMessageA(
    FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_MAX_WIDTH_MASK,
    NULL, error_code, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
    buffer, sizeof(buffer), NULL);
    if (size == 0) { buffer[0] = 0; }
    fprintf(stderr, "%s: %s\n", context, buffer);
}

// Opens the specified serial port, configures its timeouts, and sets its
// baud rate. Returns a handle on success, or INVALID_HANDLE_VALUE on failure.
HANDLE open_serial_port(const char * device, uint32_t baud_rate)
{
    HANDLE port = CreateFileA(device, GENERIC_READ | GENERIC_WRITE, 0, NULL,
    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (port == INVALID_HANDLE_VALUE)
    {
        print_error(device);
        return INVALID_HANDLE_VALUE;
    }

    // Flush away any bytes previously read or written.
    BOOL success = FlushFileBuffers(port);
    if (!success)
    {
        print_error("Failed to flush serial port");
        CloseHandle(port);
        return INVALID_HANDLE_VALUE;
    }

    // Configure read and write operations to time out after 100 ms.
    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = 0;
    timeouts.ReadTotalTimeoutConstant = 100;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 100;
    timeouts.WriteTotalTimeoutMultiplier = 0;

    success = SetCommTimeouts(port, &timeouts);
    if (!success)
    {
        print_error("Failed to set serial timeouts");
        CloseHandle(port);
        return INVALID_HANDLE_VALUE;
    }

    // Set the baud rate and other options.
    DCB state = {0};
    state.DCBlength = sizeof(DCB);
    state.BaudRate = baud_rate;
    state.ByteSize = 8;
    state.Parity = NOPARITY;
    state.StopBits = ONESTOPBIT;
    success = SetCommState(port, &state);
    if (!success)
    {
        print_error("Failed to set serial settings");
        CloseHandle(port);
        return INVALID_HANDLE_VALUE;
    }

    return port;
}

// Writes bytes to the serial port, returning 0 on success and -1 on failure.
int write_port(HANDLE port, uint8_t * buffer, size_t size)
{
    DWORD written;
    BOOL success = WriteFile(port, buffer, size, &written, NULL);
    if (!success)
    {
        print_error("Failed to write to port");
        return -1;
    }
    if (written != size)
    {
        print_error("Failed to write all bytes to port");
        return -1;
    }
    return 0;
}

// Reads bytes from the serial port.
// Returns after all the desired bytes have been read, or if there is a
// timeout or other error.
// Returns the number of bytes successfully read into the buffer, or -1 if
// there was an error reading.
SSIZE_T read_port(HANDLE port, uint8_t * buffer, size_t size)
{
    DWORD received;
    BOOL success = ReadFile(port, buffer, size, &received, NULL);
    if (!success)
    {
        print_error("Failed to read from port");
        return -1;
    }
    return received;
}

void simulateKeystroke(char key, char key2) {
    INPUT input = {0};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = (WORD)key;
    input.ki.dwFlags = TRUE ? 0 : KEYEVENTF_KEYUP;
    SendInput(1, &input, sizeof(INPUT));
    if(key2 != NULL) {
        input.ki.wVk = (WORD)key2;
        SendInput(1, &input, sizeof(INPUT));
        input.ki.dwFlags = FALSE ? 0 : KEYEVENTF_KEYUP;
        SendInput(1, &input, sizeof(INPUT));
    }
    input.ki.wVk = (WORD)key;
    input.ki.dwFlags = FALSE ? 0 : KEYEVENTF_KEYUP;
    SendInput(1, &input, sizeof(INPUT));
}

void simulateMouseMovement(float pitch, float yaw) {
    // Convert pitch/yaw to delta movements
    int dx = (int)(yaw * 1);
    int dy = (int)(-pitch * 1); // Invert pitch if necessary

    // Create INPUT structure
    INPUT input = {0};
    input.type = INPUT_MOUSE;
    input.mi.dx = dx;
    input.mi.dy = dy;
    input.mi.dwFlags = MOUSEEVENTF_MOVE;

    // Send mouse input
    SendInput(1, &input, sizeof(INPUT));
}

int parseBuffer(const uint8_t *buffer, int *output, int max_output) {
    if (buffer == NULL || output == NULL || max_output <= 0) {
        return 0;
    }

    // Find the 'R' character
    const char *startR = strchr((const char *)buffer, 'R');
    const char *startL = strchr((const char *)buffer, 'L');

    if (startR != NULL) {
        // Copy to a modifiable temporary buffer
        char temp[256];
        strncpy(temp, startR, sizeof(temp) - 1);
        temp[sizeof(temp) - 1] = '\0';

        int count = 0;
        char *token = strtok(temp, ",");
        while (token != NULL && count < max_output) {
            output[count++] = atoi(token);
            token = strtok(NULL, ",");
        }
        return 'R';
    }
    else if (startL != NULL) {
        // Copy to a modifiable temporary buffer
        char temp[256];
        strncpy(temp, startL, sizeof(temp) - 1);
        temp[sizeof(temp) - 1] = '\0';

        int count = 0;
        char *token = strtok(temp, ",");
        while (token != NULL && count < max_output) {
            output[count++] = atoi(token);
            token = strtok(NULL, ",");
        }
        return 'L';
    }
    else {
        return 0; // Nothing  found
    }
}


int main()
{
    printf("Starting ProjectFS...\n");
    // COM ports higher than COM9 need the \\.\ prefix, which is written as
    // "\\\\.\\" in C because we need to escape the backslashes.
    const char * device = COMPORT;

    // Choose the baud rate (bits per second).
    uint32_t baud_rate = BAUDRATE;

    HANDLE port = open_serial_port(device, baud_rate);
    if (port == INVALID_HANDLE_VALUE) { return 1; }

    while(1) {
        uint8_t output = 1;
        write_port(port,&output,1); 
        uint8_t buffer[255] = {0};
        int readData[255]= {0};
        SSIZE_T received = read_port(port, buffer, sizeof(buffer));
        if (received > 16) {
            int controller = parseBuffer(buffer, readData, 8);
            // Website for keystrokes
            // https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
            if(controller == (int)'R') {
                
                if(readData[1] != 0 || readData[2] != 0)
                {
                    //Pitch & Yaw
                    int pitch = readData[1] * PITCH_MODIFIER;
                    int roll = readData[2] * ROLL_MODIFIER;
                    printf("Pitch: %d, Roll %d\n", pitch ,roll); //print received data
                    if(abs(pitch) > DEADZONE || abs(roll) > DEADZONE ) {
                        simulateMouseMovement(pitch, roll);
                    }
                    
                }
                if(readData[3] != 0)
                {
                    //Roll Keystroke
                    
                    int yaw = readData[3] * YAW_MODIFIER;
                    printf("Yaw %d\n", yaw); //print received data
                    if(yaw > DEADZONE) {
                        for(int i=0; i<yaw; i++) {
                            simulateKeystroke('Q', NULL);
                        }
                    }
                    else if (yaw < -DEADZONE) {
                        for(int i=0; i>yaw; i--) {
                            simulateKeystroke('E', NULL);
                        }
                    }
                }
                if(readData[4] > 0)
                {
                    //GPIO2 Keystroke
                    printf("BTN PRESS...\n");
                    simulateKeystroke(VK_LBUTTON, NULL); // Left Click
                }
                //printf("Pitch: %d, Roll %d, Yaw %d\n", readData[0],readData[1],readData[2]); //print received data
                //printf("GPIO2 %d\n", readData[3]); //print received data
                memset(buffer, 0, sizeof(buffer)); // empty buffer
            }
            if (controller == 'L') {
                if(readData[1] != NULL)
                {
                    int yMovement = readData[1];
                    if(yMovement > 0) {
                        for(int i=0; i<yMovement; i++) {
                            simulateKeystroke('W', NULL);
                        }
                    }
                    else if (yMovement < 0) {
                        for(int i=0; i>yMovement; i--) {
                            simulateKeystroke('S', NULL);
                        }
                    }
                }
                if(readData[2] != NULL)
                {
                    int xMovement = readData[2];
                    if(xMovement > 0) {
                        for(int i=0; i<xMovement; i++) {
                            simulateKeystroke('A', NULL);
                        }
                    }
                    else if (xMovement < 0) {
                        for(int i=0; i>xMovement; i--) {
                            simulateKeystroke('D', NULL);
                        }
                    }
                }
                if(readData[3] != NULL)
                {
                    int zMovement = readData[3];
                    if(zMovement > 0) {
                        for(int i=0; i<zMovement; i++) {
                            simulateKeystroke(VK_SPACE, NULL); //spacebar
                        }
                    }
                    else if (zMovement < 0) {
                        for(int i=0; i>zMovement; i--) {
                            simulateKeystroke(VK_CONTROL, NULL); //ctrl
                        }
                    }
                }
                if(readData[3] > 0)
                {
                    //GPIO2 Keystroke
                    simulateKeystroke(VK_RBUTTON, NULL); // Right Click
                    //simulateKeystroke(VK_LMENU, 'N'); // Left Alt + N
                }

            }
        }
    }

    printf("Closing Program...\n");
    CloseHandle(port);
    return 0;
}