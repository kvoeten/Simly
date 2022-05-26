/*
*   Copyright 2022 Kaz Voeten
*
*	Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
*	The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include "CoreMinimal.h"
#include "Networking.h"
#include "Buffer.h"

#include "ClientSocket.generated.h"

// Forward declare server to fire events on it later
class UServerSocket;

USTRUCT(BlueprintType)
struct FForceSensor
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY(BlueprintReadOnly, Category = "Sensor Value")
		int64 front = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Sensor Value")
		int64 back = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Sensor Value")
		int64 left = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Sensor Value")
		int64 right = 0;
};

USTRUCT(BlueprintType)
struct FRotatorSensor
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY(BlueprintReadWrite, Category = "Sensor Value")
		int type = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Sensor Value")
		int id = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Sensor Value")
		int rotation = 0;
};

class ClientSocket 
{
public:
	// Socket
	FSocket* Socket;
	Buffer RecvBuff;

	UPROPERTY(BlueprintReadOnly, Category = "TCP Connection Properties")
	FString Address;

	// Decoding
	uint32 DecodeLen = -1;
	uint32 Header = 2;

	// Data
	FDateTime LastPing;
	int32 PingNum = -1;
	int32 PingKey = 0x20101010;

	// Sensors
	FForceSensor Force;
	FRotatorSensor Rotation;

	long UID = -1;

	bool operator==(const ClientSocket& Other)
	{
		return Address == Other.Address;
	}

	// Packet handling
	void ProcessPacket(UServerSocket* server);
	void SendRotationRequest(FRotatorSensor request);
	void SendPing();

private:
	void SendPacket(Buffer OutPacket);
	void HandlePong(uint32 code);
	void HandleForceSensor(UServerSocket* server);
	void HandleRotator(UServerSocket* server);
};