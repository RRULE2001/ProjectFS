#include <stdio.h>
#include <stdint.h>
#include <windows.h>

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

void SimulateKeystroke(char key, char key2) {
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

int parse_comma_separated_ints(const uint8_t *buffer, int *output, int max_output) {
    if (buffer == NULL || output == NULL || max_output <= 0) {
        return 0;
    }

    // Find the 'R' character
    const char *start = strchr((const char *)buffer, 'R');
    if (start == NULL) {
        return 0; // 'R' not found
    }

    // Move past 'R'
    start++; 

    // Copy to a modifiable temporary buffer
    char temp[256];
    strncpy(temp, start, sizeof(temp) - 1);
    temp[sizeof(temp) - 1] = '\0';

    int count = 0;
    char *token = strtok(temp, ",");
    while (token != NULL && count < max_output) {
        output[count++] = atoi(token);
        token = strtok(NULL, ",");
    }

    return count;
}


int main()
{
    printf("Program Starting...\n");
    // COM ports higher than COM9 need the \\.\ prefix, which is written as
    // "\\\\.\\" in C because we need to escape the backslashes.
    const char * device = "\\\\.\\COM7";

    // Choose the baud rate (bits per second).
    uint32_t baud_rate = 115200;

    HANDLE port = open_serial_port(device, baud_rate);
    if (port == INVALID_HANDLE_VALUE) { return 1; }

    while(1) {
        uint8_t output = 1;
        write_port(port,&output,1); 
        uint8_t buffer[255] = {0};
        int readData[255]= {0};
        SSIZE_T received = read_port(port, buffer, sizeof(buffer));
        if (received > 6) {
            parse_comma_separated_ints(buffer, readData, 8);
            // Website for keystrokes
            // https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
            if(readData[0] != 0)
            {
                //Pitch Keystroke
                int pitch = readData[0];
            }
            if(readData[1] != 0)
            {
                //Roll Keystroke
                int roll = readData[1];
            }
            if(readData[2] != 0)
            {
                //Yaw Keystroke
                int yaw = readData[2];
            }
            if(readData[3] > 0)
            {
                //GPIO2 Keystroke
                SimulateKeystroke('U', NULL);
                SimulateKeystroke(0x10, 'A'); // Shift + a
            }
            //printf("Pitch: %d, Roll %d, Yaw %d\n", readData[0],readData[1],readData[2]); //print received data
            //printf("GPIO2 %d\n", readData[3]); //print received data
            memset(buffer, 0, sizeof(buffer)); // empty buffer
        }
    }

    printf("Program Closing...\n");
    CloseHandle(port);
    return 0;
}