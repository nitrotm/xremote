/**
 * Xremote - raw packet checksum
 *
 * \author Antony Ducommun (nitro.tm@gmail.com)
 *
 * license : free of use for any purpose ;)
 */
#include "../xremote.h"


XRNETCHECKSUMLISTENER::XRNETCHECKSUMLISTENER(const bool enabled) : XRNETLISTENER(), enabled(enabled) {
}

XRNETCHECKSUMLISTENER::~XRNETCHECKSUMLISTENER() {
}

int XRNETCHECKSUMLISTENER::getHeaderSize() const {
	if (this->enabled) {
		return sizeof(unsigned int);
	}
	return 0;
}

bool XRNETCHECKSUMLISTENER::onReceivePacket(const XRNETPACKET &packet) {
	if (this->enabled) {
		const XRNETBUFFER & inBuffer = packet.getBuffer();

		if (inBuffer.getSize() < (int)sizeof(unsigned int)) {
			printf("packet size is too small\n");
			return false;
		}

		// get remote sum
		unsigned int remoteSum = 0;

		inBuffer.getData(0, sizeof(unsigned int), &remoteSum);

		// build stripped raw buffer
		XRNETBUFFER outBuffer(inBuffer.getSize() - sizeof(unsigned int));

		outBuffer.setData(inBuffer.getPtr(sizeof(unsigned int)));

		// compute local sum
		unsigned int	 localSum = 11;
		int			 half = outBuffer.getSize() / 2;

		for (int i = 0; i < half; i++) {
			localSum += 11 * localSum + (outBuffer[i] ^ outBuffer[half + i]);
		}

		// check local/remote sum equality
		if (remoteSum != localSum) {
			printf("chksum failed (%08X != %08X) !\n", remoteSum, localSum);
			return false;
		}
		return XRNETLISTENER::onReceivePacket(XRNETPACKET(packet, outBuffer));
	}
	return XRNETLISTENER::onReceivePacket(packet);
}

bool XRNETCHECKSUMLISTENER::onSendPacket(XRNETPACKET &packet) {
	if (this->enabled) {
		const XRNETBUFFER & inBuffer = packet.getBuffer();

		// compute check sum
		unsigned int	 sum = 11;
		int			 half = inBuffer.getSize() / 2;

		for (int i = 0; i < half; i++) {
			sum += 11 * sum + (inBuffer[i] ^ inBuffer[half + i]);
		}

		// build extended raw buffer
		XRNETBUFFER outBuffer(inBuffer.getSize() + sizeof(unsigned int));

		outBuffer.setData(0, sizeof(unsigned int), &sum);
		outBuffer.setData(sizeof(unsigned int), inBuffer.getSize(), inBuffer.getPtr());

		packet.setBuffer(outBuffer);
	}
	return XRNETLISTENER::onSendPacket(packet);
}
