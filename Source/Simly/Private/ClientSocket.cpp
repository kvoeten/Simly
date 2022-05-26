/*
*   Copyright 2022 Kaz Voeten
*
*	Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
*	The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "ClientSocket.h"
#include "ServerSocket.h"

void ClientSocket::ProcessPacket(UServerSocket* server)
{
	short nPacketID = this->RecvBuff.readUInt16_LE();
	UE_LOG(LogTemp, Log, TEXT("[ClientSocket] Processing Packet: %d."), nPacketID);
	switch (nPacketID)
	{
	case 0xF0:
		// HandleHandshake();
		break;
	case 0x01:
		HandlePong(this->RecvBuff.readInt32_LE());
		break;
	case 0x02:
		HandleForceSensor(server);
		break;
	case 0x03:
		HandleRotator(server);
		break;
	}
}

void ClientSocket::HandleForceSensor(UServerSocket* server)
{
	// Create struct with sensor data
	this->Force.front = this->RecvBuff.readUInt32_LE();
	this->Force.back = this->RecvBuff.readUInt32_LE();
	this->Force.left = this->RecvBuff.readUInt32_LE();
	this->Force.right = this->RecvBuff.readUInt32_LE();

	// Broadcast result on server object
	AsyncTask(ENamedThreads::GameThread, [server, this]()
	{
		server->OnForceSensorData.Broadcast(this->Address, this->Force);
	});
}

void ClientSocket::HandleRotator(UServerSocket* server)
{
	// Create struct with sensor data
	this->Rotation.type = this->RecvBuff.readUInt16_LE();
	this->Rotation.id = this->RecvBuff.readUInt16_LE();
	this->Rotation.rotation = this->RecvBuff.readInt32_LE();

	// Broadcast result on server object
	AsyncTask(ENamedThreads::GameThread, [server, this]()
	{
		server->OnRotationData.Broadcast(this->Address, this->Rotation);
	});
}

void ClientSocket::SendPacket(Buffer OutPacket)
{
	if (OutPacket.getBuffer().empty())
	{
		UE_LOG(LogTemp, Error, TEXT("[ClientSocket] OutPacket buffer was empty."));
		return;
	}

	// Make Packet
	Buffer Packet;
	Packet.writeUInt16_LE(OutPacket.getBuffer().size());
	Packet.writeArray((unsigned char*) OutPacket.getBuffer().data(), OutPacket.getBuffer().size());

	// Send Packet
	int32 BytesSent = 0;
	bool success = this->Socket->Send(Packet.getBuffer().data(), Packet.getBuffer().size(), BytesSent);

	// Release buffer(s)
	Packet.clear();
	OutPacket.clear();

	if (!success)
	{
		UE_LOG(LogTemp, Error, TEXT("[ClientSocket] Unable to send OutPacket!"));
		this->Socket->Close();
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[ClientSocket] Succesfully sent packet of %d Bytes."), (int) BytesSent);
}

void ClientSocket::SendPing()
{
	Buffer OutPacket;
	OutPacket.writeUInt16_LE(0x01);
	OutPacket.writeInt32_LE(this->PingNum ^ this->PingKey);
	this->SendPacket(OutPacket);
}

void ClientSocket::SendRotationRequest(FRotatorSensor request)
{
	Buffer OutPacket;
	OutPacket.writeUInt16_LE(0x02);
	OutPacket.writeUInt16_LE(request.type);
	OutPacket.writeUInt16_LE(request.id);
	OutPacket.writeInt32_LE(request.rotation);
	this->SendPacket(OutPacket);
}

void ClientSocket::HandlePong(uint32 code)
{
	UE_LOG(LogTemp, Log, TEXT("[ClientSocket] Received Pong."));
	if (code == this->PingNum)
	{
		UE_LOG(LogTemp, Log, TEXT("[ClientSocket] Validated Pong."));
		this->PingNum = -1;
	}
	else this->Socket->Close();
}