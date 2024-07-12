#include <SimpleSerial.h>
#include <chrono>
#include <thread>
#include <Xinput.h>
#include <algorithm> // Consider removal in favour of own implementation for clamping
#include <cstring> // Aswell. Serves for strlen()
#include <windows.h>
#include <iostream>

#pragma comment(lib, "XInput.lib")	// required for linker

#define XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE  6849
#define XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE 6689
#define XINPUT_GAMEPAD_TRIGGER_THRESHOLD 15

char com_port[] = "\\\\.\\COM3";
DWORD COM_BAUD_RATE = CBR_9600;
SimpleSerial Serial(com_port, COM_BAUD_RATE);

int main()
{
	char newSend = 0;
	XINPUT_STATE gpState;	// Gamepad state
	BYTE oldRT = 0;
	SHORT oldLX = 0;

	int player = -1;	// Gamepad ID

	// Polling all 4 gamepads to see who's alive
	for (int i = 0; i < 4; i++)
	{
		DWORD res = XInputGetState(i, &gpState);	// Getting state
		if (res == ERROR_SUCCESS)			// If alive - print message
		{
			printf("Controller #%d is ON!\n", i + 1);
			player = i;	// Assign last alive gamepad as active
		}
	}
	if (player < 0) // If player==-1 in other words...
	{
		printf("Haven't found any gamepads...\n");
	}
	else
	{

		char send[3] = { 0, 0, '\0' }; //RT, LX, '\0'}; //LX convertido de 16 para 8 bits

		while (true)
		{
			newSend = 0;
			//system("CLS");					// Clear screen

			SHORT LX = gpState.Gamepad.sThumbLX; // Get LX
			SHORT LY = gpState.Gamepad.sThumbLY; // Get LY
			double magnitudeL = sqrt(LX * LX + LY * LY);    // Calculate the radius of current position
			// abs() does not work on double values due to the precision level. To bypass this, we cast sqrt() to a SHORT and then get its abs valu
			if (magnitudeL < XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) // Inside dead zone?
			{
				// Set all to 0
				LX = 0.0;
				LY = 0.0;
			}

			SHORT RX = gpState.Gamepad.sThumbRX; // Get RX
			SHORT RY = gpState.Gamepad.sThumbRY; // Get RY
			double magnitudeR = sqrt(RX * RX + RY * RY);     // Calculate the radius of current position
			if (magnitudeR < XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE) // Inside dead zone?
			{
				// Set all to 0
				RX = 0.0;
				RY = 0.0;
			}

			BYTE RT = gpState.Gamepad.bRightTrigger; // Get RX
			if (RT < XINPUT_GAMEPAD_TRIGGER_THRESHOLD) // Inside dead zone?
			{
				// Set all to 0
				RT = 0.0;
			}

			memset(&gpState, 0, sizeof(XINPUT_STATE)); 	// Reset state
			DWORD res = XInputGetState(0, &gpState);		// Get new state
			printf("\nLX\tLY\tRX\tRY\tLTrig\tRTrig\tButtons\n"); // Print header

			// Thumbstick values are divided by 256 for better consistency
			printf("%d\t%d\t%d\t%d\t%d\t%d\t%d\n",
				LX / 256,
				LY / 256,
				RX / 256,
				RY / 256,
				gpState.Gamepad.bLeftTrigger,
				RT,
				gpState.Gamepad.wButtons);

			if (RT != oldRT) {
				send[0] = std::clamp((int)RT, 1, 255); // Envia RT clampado a no minimo 1, com terminação nula. É necessário a clampagem para 
				oldRT = RT;
				newSend = 1;
			}

			if (LX != oldLX) {
				send[1] = std::clamp((int)(LX * 254 / 65536), -127, 127) + 128; //Envia LX, curva. Convertido para 1 a 255, com valor neutro (zero) em 127.
				oldLX = LX;
				newSend = 1;
			}

			if (Serial.IsConnected() && newSend)
			{
				printf(Serial.WriteSerialPort(send) ? "Sent\n" : "Not sent\n");
				//printf(Serial.WriteSerialPort(std::to_string(RT)) ? "Sent\n" : "Not sent\n");
				printf("SEND: RT: %d \t LX: %d \t end: %d\n", (unsigned char)send[0], (unsigned char)send[1], (unsigned char)send[2]);
			}
			else
			{
				printf("%s", "Not connected\n");
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}
	system("PAUSE");
	return 0;
}
