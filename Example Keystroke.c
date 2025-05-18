#include <windows.h>

void SimulateKeystroke(WORD key, BOOL keydown) {
    INPUT input = {0};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = key;
    input.ki.dwFlags = keydown ? 0 : KEYEVENTF_KEYUP;
    SendInput(1, &input, sizeof(INPUT));
}

// Example usage: Press and release the 'A' key
int main() {
    SimulateKeystroke(0x41, TRUE);  // Press 'A'
    SimulateKeystroke(0x41, FALSE); // Release 'A'
    return 0;
}