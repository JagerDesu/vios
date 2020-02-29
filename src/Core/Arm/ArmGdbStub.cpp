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

// Hack because GDB is terrible
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

static uint8_t CalculateChecksum(const void* buffer, size_t size) {
	auto b = (const uint8_t*)buffer;
	int result = 0;
	for (size_t i = 0; i < size; i++) {
		result += b[i];
	}
	result = result % 256;
	return (uint8_t)result;
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

static void WritePacket(const char* response, size_t responseLength, uint8_t checksum, std::vector<uint8_t>& commandBuffer) {
	auto begin = (const uint8_t*)response;
	auto end = begin + responseLength;

	uint8_t chkbuf[3] = {};
	ByteToHex(checksum, (char*)chkbuf);

	commandBuffer.push_back(GdbProtocol::Begin);
	commandBuffer.insert(commandBuffer.end(), begin, end);
	commandBuffer.push_back(GdbProtocol::Checksum);
	commandBuffer.insert(commandBuffer.end(), chkbuf, chkbuf + 2);
}

static void WorkerProcedure() {
	for(;;) {

	}
}

GdbStub::GdbStub(Arm::Interface* arm) :
	socketHandle(InvalidSocket),
	port(0),
	status(Status::Uninitialized),
	arm(arm)
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
	status = Status::Running;
	return true;
}

void GdbStub::Shutdown() {
	shutdown(socketHandle, SHUT_RDWR);
	status = Status::Uninitialized;
}

void GdbStub::Run() {
	std::vector<uint8_t> buffer;
	uint8_t c;
	while ((c = ReadByte())!= '#') {
		if (c == '$')
			continue;
		if (c == '#')
			break;
		buffer.push_back(c);
	}


	char cb[2];

	cb[0] = ReadByte();
	cb[1] = ReadByte();

	uint8_t cs = HexToByte(cb);
	WriteByte('+');
	
	ConsumePacket(buffer.data(), buffer.size());
	buffer.clear();
}

void GdbStub::ConsumePacket(const uint8_t* buffer, size_t size) {
	auto begin = (const char*)buffer;
	auto end = begin + size;
	std::vector<uint8_t> responseBuffer;

	const char* QSupported = "qSupported";
	const size_t QSupportedLength = strlen(QSupported);

	for (auto c = begin; c < end; c++) {
		switch (*c) {
		case '?': // Why has the program been halted?
			break;
		case GdbProtocol::Ack: // Acknowledement
			c++;
			break;
		case GdbProtocol::Repeat:
			c++;
			responseBuffer.insert(responseBuffer.end(), lastPacket.begin(), lastPacket.end());
			break;
		case 'v': {
			const size_t MaxCommandBufferSize = 256;
			char commandBuffer[MaxCommandBufferSize];
			if (strncmp(c, "vSupport", MaxCommandBufferSize)) {
				
			}
			else {
				LOG_ERROR(ArmGdbStub, "Unknown \'v\' packet");
			}
			break;
		}
		default:
			LOG_DEBUG(ArmGdbStub, "Invalid GDB argument");
			break;
		}

		if (strncmp(QSupported, (const char*)c, QSupportedLength) == 0) {
			const char* response = "PacketSize=ffff";
			size_t responseSize = strlen(response);
			uint8_t checksum = CalculateChecksum(response, responseSize);
			WritePacket(response, responseSize, checksum, responseBuffer);
			c += QSupportedLength;
			break;
		}
	}
	lastPacket = responseBuffer;
	Write(responseBuffer.data(), responseBuffer.size());
}

uint8_t GdbStub::ReadByte() {
	uint8_t c;
	int size = recv(socketHandle, &c, 1, 0);
	if (size < 0)
		LOG_WARNING(ArmGdbStub, "Failed to recv packet");
	return c;
}

void GdbStub::WriteByte(uint8_t c) {
	int size = send(socketHandle, &c, 1, 0);
	if (size < 0)
		LOG_WARNING(ArmGdbStub, "Failed to send packet");
}

}