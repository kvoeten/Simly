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