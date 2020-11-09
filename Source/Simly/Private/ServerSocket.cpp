#include "ServerSocket.h"
#include "Async/Async.h"
#include "Buffer.h"
#include "SocketSubsystem.h"
#include "Kismet/KismetSystemLibrary.h"

DEFINE_LOG_CATEGORY(LogServer);

UServerSocket::UServerSocket(const FObjectInitializer& init) : UActorComponent(init)
{
	bShouldAutoListen = true;
	bReceiveDataOnGameThread = true;
	bWantsInitializeComponent = true;
	bAutoActivate = true;
	ListenPort = 3000;
	ListenSocketName = TEXT("ue4-tcp-server");
	bDisconnectOnFailedEmit = true;
	bShouldPing = false;
	PingInterval = 10.0f;
	PingMessage = TEXT("<Ping>");
	BufferMaxSize = 2048;
}

void UServerSocket::StartListenServer(const int32 InListenPort)
{
	FIPv4Address Address;
	FIPv4Address::Parse(TEXT("0.0.0.0"), Address);

	//Create Socket
	FIPv4Endpoint Endpoint(Address, InListenPort);

	ListenSocket = FTcpSocketBuilder(*ListenSocketName)
		//.AsNonBlocking()
		.AsReusable()
		.BoundToEndpoint(Endpoint)
		.WithReceiveBufferSize(BufferMaxSize);

	ListenSocket->SetReceiveBufferSize(BufferMaxSize, BufferMaxSize);
	ListenSocket->SetSendBufferSize(BufferMaxSize, BufferMaxSize);

	ListenSocket->Listen(8);

	OnListenBegin.Broadcast();
	bShouldListen = true;

	UE_LOG(LogTemp, Log, TEXT("[ServerSocket] Listening on port: %d"), (int) InListenPort);
	ServerFinishedFuture = UServerSocket::RunLambdaOnBackGroundThread([&]()
	{
		TArray<uint8> ReceiveBuffer;
		TArray<TSharedPtr<ClientSocket>> ClientsDisconnected;

		UE_LOG(LogTemp, Log, TEXT("[ServerSocket] Worker thread started."));

		while (bShouldListen)
		{
			if (!ListenSocket)
			{
				UE_LOG(LogTemp, Log, TEXT("[ServerSocket] Socket became invalid."));
				bShouldListen = false;
				continue;
			}

			//Do we have clients trying to connect? connect them
			bool bHasPendingConnection;
			ListenSocket->HasPendingConnection(bHasPendingConnection);
			if (bHasPendingConnection)
			{
				UE_LOG(LogTemp, Log, TEXT("[ServerSocket] Processing pending clients."));
				TSharedPtr<FInternetAddr> Addr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
				FSocket* Socket = ListenSocket->Accept(*Addr, TEXT("tcp-client"));

				const FString AddressString = Addr->ToString(true);
				
				TSharedPtr<ClientSocket> Client = MakeShareable(new ClientSocket());
				Client->Address = AddressString;
				Client->Socket = Socket;
				Client->LastPing = FDateTime::Now();
				Client->PingNum = -1;

				Clients.Add(AddressString, Client);	//todo: balance this with remove when clients disconnect
				UE_LOG(LogTemp, Log, TEXT("[ServerSocket] New client connected: %s."), *Client->Address);

				AsyncTask(ENamedThreads::GameThread, [&, AddressString]()
				{
					OnClientConnected.Broadcast(AddressString);
				});
			}

			//Check each endpoint for data
			for (auto ClientPair : Clients)
			{
				TSharedPtr<ClientSocket> Client = ClientPair.Value;

				//Did we disconnect? Note that this almost never changed from connected due to engine bug, instead it will be caught when trying to send data
				ESocketConnectionState ConnectionState = Client->Socket->GetConnectionState();
				if (ConnectionState != ESocketConnectionState::SCS_Connected)
				{
					ClientsDisconnected.Add(Client);
					continue;
				}

				if (Client->DecodeLen == -1 && Client->Socket->HasPendingData(Client->Header))
				{
					int32 Read = 0;
					ReceiveBuffer.SetNumUninitialized(2);
					Client->Socket->Recv(ReceiveBuffer.GetData(), ReceiveBuffer.Num(), Read);
					Client->RecvBuff.writeArray(ReceiveBuffer.GetData(), ReceiveBuffer.Num());
					Client->DecodeLen = Client->RecvBuff.readInt16_LE();
					Client->RecvBuff.clear();
					UE_LOG(LogTemp, Log, TEXT("[ClientSocket] Received Header: %d."), (int)Client->DecodeLen);
				}

				if (Client->DecodeLen != -1 && Client->Socket->HasPendingData(Client->DecodeLen))
				{
					// Read packet data
					int32 Read = 0;
					ReceiveBuffer.SetNumUninitialized(Client->DecodeLen);
					Client->Socket->Recv(ReceiveBuffer.GetData(), ReceiveBuffer.Num(), Read);
					Client->RecvBuff.writeArray(ReceiveBuffer.GetData(), ReceiveBuffer.Num());

					// Handle packet
					Client->ProcessPacket(this);

					// Clear buffer
					Client->DecodeLen = -1;
					Client->RecvBuff.clear();
				}

				if (bShouldPing)
				{
					FDateTime Now = FDateTime::Now();
					float TimeSinceLastPing = (Now - Client->LastPing).GetTotalSeconds();

					if (TimeSinceLastPing > PingInterval)
					{
						UE_LOG(LogTemp, Log, TEXT("[ClientSocket] Running Ping Logic, current num: %d"), (int) Client->PingNum);
						if (Client->PingNum == -1) {
							// Last ping attempt was succesfull, send new key
							UE_LOG(LogTemp, Log, TEXT("[ClientSocket] Sending Ping: %d."), (int)Client->PingKey);
							Client->PingNum = rand();
							Client->SendPing();
						}
						else
						{
							// Previous ping attempt didn't result key in time
							UE_LOG(LogTemp, Log, TEXT("[ClientSocket] Never got ping! Closing socket.%d."));
							Client->Socket->Close();
						}
						Client->LastPing = Now;
					}
				}
			}

			//Handle disconnections
			if (ClientsDisconnected.Num() > 0)
			{
				UE_LOG(LogTemp, Log, TEXT("[ServerSocket] Removing dead cients."));
				for (TSharedPtr<ClientSocket> ClientToRemove : ClientsDisconnected)
				{
					const FString Address = ClientToRemove->Address;
					Clients.Remove(Address);
					AsyncTask(ENamedThreads::GameThread, [this, Address]()
					{
						OnClientDisconnected.Broadcast(Address);
					});
				}
				ClientsDisconnected.Empty();
			}

			//sleep for 100microns
			FPlatformProcess::Sleep(0.0001);
		}//end while

		for (auto ClientPair : Clients)
		{
			ClientPair.Value->Socket->Close();
		}
		Clients.Empty();

		//Server ended
		AsyncTask(ENamedThreads::GameThread, [&]()
		{
			Clients.Empty();
			OnListenEnd.Broadcast();
		});
	});
}

void UServerSocket::StopListenServer()
{
	if (ListenSocket)
	{
		bShouldListen = false;
		ServerFinishedFuture.Get();

		ListenSocket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ListenSocket);
		ListenSocket = nullptr;

		OnListenEnd.Broadcast();
	}
}

void UServerSocket::DisconnectClient(FString ClientAddress /*= TEXT("All")*/, bool bDisconnectNextTick/*=false*/)
{
	TFunction<void()> DisconnectFunction = [this, ClientAddress]
	{
		bool bDisconnectAll = ClientAddress == TEXT("All");

		if (!bDisconnectAll)
		{
			TSharedPtr<ClientSocket> Client = Clients[ClientAddress];

			if (Client.IsValid())
			{
				Client->Socket->Close();
				Clients.Remove(Client->Address);
				OnClientDisconnected.Broadcast(ClientAddress);
			}
		}
		else
		{
			for (auto ClientPair : Clients)
			{
				TSharedPtr<ClientSocket> Client = ClientPair.Value;
				Client->Socket->Close();
				Clients.Remove(Client->Address);
				OnClientDisconnected.Broadcast(ClientAddress);
			}
		}
	};

	if (bDisconnectNextTick)
	{
		//disconnect on next tick
		AsyncTask(ENamedThreads::GameThread, DisconnectFunction);
	}
	else
	{
		DisconnectFunction();
	}
}

void UServerSocket::InitializeComponent()
{
	Super::InitializeComponent();
}

void UServerSocket::UninitializeComponent()
{
	Super::UninitializeComponent();
}

void UServerSocket::BeginPlay()
{
	Super::BeginPlay();

	if (bShouldAutoListen)
	{
		StartListenServer(ListenPort);
	}
}

void UServerSocket::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopListenServer();

	Super::EndPlay(EndPlayReason);
}