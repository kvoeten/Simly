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