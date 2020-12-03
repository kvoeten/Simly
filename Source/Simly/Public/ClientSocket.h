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
	UPROPERTY(BlueprintReadOnly, Category = "Sensor Value")
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
	void SendRotationRequest(int steps);
	void SendPing();

private:
	void SendPacket(Buffer OutPacket);
	void HandlePong(uint32 code);
	void HandleForceSensor(UServerSocket* server);
	void HandleRotator(UServerSocket* server);
};