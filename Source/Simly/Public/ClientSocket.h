/*
*   This file is part of the Simly project by Kaz Voeten at the Eindhoven University of Technology.
*   Copyright (C) 2020 Eindhoven University of Technology
*
*   This program is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program.  If not, see <http://www.gnu.org/licenses/>.
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