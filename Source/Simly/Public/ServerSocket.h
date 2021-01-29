/*
*   MIT License
*
*	Copyright (c) 2018 Jan Kaniewski (getnamo)
*
*	Permission is hereby granted, free of charge, to any person obtaining a copy
*	of this software and associated documentation files (the "Software"), to deal
*	in the Software without restriction, including without limitation the rights
*	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*	copies of the Software, and to permit persons to whom the Software is
*	furnished to do so, subject to the following conditions:
*
*	The above copyright notice and this permission notice shall be included in all
*	copies or substantial portions of the Software.
*
*	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
*	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
*	SOFTWARE.
*/

#pragma once

#include "Components/ActorComponent.h"
#include "Networking.h"
#include "IPAddress.h"
#include "ClientSocket.h"

#include "ServerSocket.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogServer, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FTCPEventSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTCPMessageSignature, const TArray<uint8>&, Bytes);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTCPClientSignature, const FString&, Client);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FForceSensorSignature, FString, Client, FForceSensor, Data);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFRotatorSensorSignature, FString, Client, FRotatorSensor, Data);

UCLASS(ClassGroup = "Networking", meta = (BlueprintSpawnableComponent))
class SIMLY_API UServerSocket : public UActorComponent
{
	GENERATED_UCLASS_BODY()

public:
	//Async events

	UPROPERTY(BlueprintAssignable, Category = "Sensor Events")
	FForceSensorSignature OnForceSensorData;

	UPROPERTY(BlueprintAssignable, Category = "Sensor Events")
	FFRotatorSensorSignature OnRotationData;

	/** On message received on the receiving socket. */
	UPROPERTY(BlueprintAssignable, Category = "TCP Events")
	FTCPMessageSignature OnReceivedBytes;

	/** Callback when we start listening on the TCP receive socket*/
	UPROPERTY(BlueprintAssignable, Category = "TCP Events")
	FTCPEventSignature OnListenBegin;

	/** Called after receiving socket has been closed. */
	UPROPERTY(BlueprintAssignable, Category = "TCP Events")
	FTCPEventSignature OnListenEnd;

	/** Callback when we start listening on the TCP receive socket*/
	UPROPERTY(BlueprintAssignable, Category = "TCP Events")
	FTCPClientSignature OnClientConnected;

	//This will only get called if an emit fails due to how FSocket works. Use custom disconnect logic if possible.
	UPROPERTY(BlueprintAssignable, Category = "TCP Events")
	FTCPClientSignature OnClientDisconnected;

	/** Default connection port e.g. 3001*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TCP Connection Properties")
	int32 ListenPort;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TCP Connection Properties")
	FString ListenSocketName;

	/** in bytes */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TCP Connection Properties")
	int32 BufferMaxSize;

	/** If true will auto-listen on begin play to port specified for receiving TCP messages. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TCP Connection Properties")
	bool bShouldAutoListen;

	/** Whether we should process our data on the game thread or the TCP thread. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TCP Connection Properties")
	bool bReceiveDataOnGameThread;

	/** With current FSocket architecture, this is about the only way to catch a disconnection*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TCP Connection Properties")
	bool bDisconnectOnFailedEmit;

	/** Convenience ping send utility used to determine if connection disconnected. This is a custom system and not a normal ping*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TCP Connection Properties")
	bool bShouldPing;

	/** How often we should ping */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TCP Connection Properties")
	float PingInterval;

	/** What the default ping message should be*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TCP Connection Properties")
	FString PingMessage;

	UPROPERTY(BlueprintReadOnly, Category = "TCP Connection Properties")
	bool bIsConnected;

	/** 
	* Start listening at given port for TCP messages. Will auto-listen on begin play by default
	*/
	UFUNCTION(BlueprintCallable, Category = "TCP Functions")
	void StartListenServer(const int32 InListenPort = 3001);

	/**
	* Close the receiving socket. This is usually automatically done on end play.
	*/
	UFUNCTION(BlueprintCallable, Category = "TCP Functions")
	void StopListenServer();

	/** 
	* Disconnects client on the next tick
	* @param ClientAddress	Client Address and port, obtained from connection event or 'All' for multicast
	*/
	UFUNCTION(BlueprintCallable, Category = "TCP Functions")
	void DisconnectClient(FString ClientAddress = TEXT("All"), bool bDisconnectNextTick = false);

	/**
	* Send rotation request to specified client.
	*/
	UFUNCTION(BlueprintCallable, Category = "TCP Functions")
	void SendRotationRequest(FString client, FRotatorSensor request);

	virtual void InitializeComponent() override;
	virtual void UninitializeComponent() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
protected:
	TMap<FString, TSharedPtr<ClientSocket>> Clients;
	FSocket* ListenSocket;
	FThreadSafeBool bShouldListen;
	TFuture<void> ServerFinishedFuture;

	FString SocketDescription;
	TSharedPtr<FInternetAddr> RemoteAdress;

private:
	static TFuture<void> RunLambdaOnBackGroundThread(TFunction< void()> InFunction)
	{
		return Async(EAsyncExecution::Thread, InFunction);
	}
};
