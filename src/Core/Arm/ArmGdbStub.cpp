#include "Common/Log.hpp"
#include "Common/Types.hpp"
#include "ArmGdbStub.hpp"

#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <cstring>
#include <cstdio>

#include <vector>

namespace Arm {

namespace GdbProtocol {
	enum {
		Begin = '$',
		Ack = '+',
		Repeat = '-',
		Checksum = '#'
	};
}

const int InvalidSocket = -1;

static uint8_t HexToByte(const char* hex) {
	unsigned int c = 0;
	sscanf(hex,"%x", &c);
	return (uint8_t)c;
}

static void ByteToHex(uint8_t c, char* hex) {
	sprintf(hex, "%02X", (unsigned int)c);
}

struct CommandBuffer {

	void WriteBinaryHex(const void* data, size_t size) {
		const uint8_t* b = static_cast<const uint8_t*>(data);
		char hex[2];
		for (size_t i = 0; i < size; i++) {
			ByteToHex(b[i], hex);
			buffer.push_back(hex[0]);
			buffer.push_back(hex[1]);
		}
	}

	void Flush() {
		buffer.clear();
	}
	std::vector<uint8_t> buffer;
};

static void SendResponse(const char* response, uint8_t* commandBuffer, size_t bufferSize) {
	size_t commandSize = 0;
	commandBuffer[0] = GdbProtocol::Begin;

}

GdbStub::GdbStub() :
	socketHandle(InvalidSocket),
	port(0),
	status(Status::Uninitialized)
{
}

GdbStub::~GdbStub() {

}

bool GdbStub::Init(uint16_t port) {
	int listenerSocketHandle;
	int result;
	int reuse = 1;
	struct sockaddr_in serverAddress = {};
	socklen_t addressLength = sizeof(serverAddress);

	this->port = port;

	LOG_INFO(ArmGdbStub, "Starting up GDB server on port %u", (unsigned int)port);

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
	serverAddress.sin_addr.s_addr = INADDR_ANY;
	
	listenerSocketHandle = socket(PF_INET, SOCK_STREAM, 0);
	socketHandle = InvalidSocket;

	if (listenerSocketHandle == InvalidSocket) {
		LOG_ERROR(ArmGdbStub, "Cannot create listener socket.");
		return false;
	}

	result = setsockopt(
		listenerSocketHandle,
		SOL_SOCKET,
		SO_REUSEADDR,
		(const char*)&reuse,
		sizeof(reuse)
	);

    if (result < 0) {
		LOG_ERROR(ArmGdbStub, "Failed to set socket option (reuse address).");
		return true;
    }

	addressLength = sizeof(serverAddress);
	result = bind(listenerSocketHandle, (struct sockaddr*)&serverAddress, addressLength);
    if (result < 0) {
        LOG_ERROR(ArmGdbStub, "Cannot bind listener socket.");
    }

	result = listen(listenerSocketHandle, 1);
    if (result < 0) {
        LOG_ERROR(ArmGdbStub, "Cannot listen on listener socket.");
	}
	
	LOG_INFO(ArmGdbStub, "Waiting on gdb to connect.");

	struct sockaddr_in clientAddress = {};
	addressLength = sizeof(clientAddress);
	socketHandle = accept(
		listenerSocketHandle,
		(struct sockaddr*)&clientAddress,
		&addressLength
	);

	if (socketHandle == InvalidSocket) {
		LOG_ERROR(ArmGdbStub, "Accept on listener socket failed.");
		return false;
	}

	if (listenerSocketHandle == InvalidSocket) {
		shutdown(socketHandle, SHUT_RDWR);
	}

	return true;
}

void GdbStub::Shutdown() {
	shutdown(socketHandle, SHUT_RDWR);
	status = Status::Uninitialized;
}

uint8_t GdbStub::ReadByte() {
	uint8_t c;
	int size = recv(socketHandle, &c, 1, 0);
	if (size != 1)
		LOG_WARNING(ArmGdbStub, "Failed to recv packet");
}

void GdbStub::WriteByte(uint8_t c) {
	int size = send(socketHandle, &c, 1, 0);
	if (size != 1)
		LOG_WARNING(ArmGdbStub, "Failed to send packet");
}

}