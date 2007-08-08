/**
 * Xremote - raw packet aes encryption
 *
 * \author Antony Ducommun (nitro.tm@gmail.com)
 *
 * license : free of use for any purpose ;)
 */
#include "../xremote.h"
#include "../thirdparty/sha2.h"
#include "../thirdparty/aes.h"


XRNETCRYPTLISTENER::XRNETCRYPTLISTENER(const string &password) : XRNETLISTENER() {
	memset(this->key, 0, sizeof(this->key));
	if (password.length() > 0) {
		sha256_context ctx;

		memset(&ctx, 0, sizeof(sha256_context));
		sha256_starts(&ctx);
		sha256_update(&ctx, (uint8*)password.c_str(), password.length());
		sha256_finish(&ctx, this->key);
		this->enabled = true;
	} else {
		this->enabled = false;
	}
}

XRNETCRYPTLISTENER::~XRNETCRYPTLISTENER() {
}

int XRNETCRYPTLISTENER::getHeaderSize() const {
	return 0;
}

bool XRNETCRYPTLISTENER::onReceivePacket(const XRNETPACKET &packet) {
	if (this->enabled) {
		const XRNETBUFFER & inBuffer = packet.getBuffer();

		if (inBuffer.getSize() % 16 != 0) {
			printf("packet size is not rounded to 16 bytes (%d bytes)\n", inBuffer.getSize());
			return false;
		}

		// setup encryption key
		aes_context ctx;

		memset(&ctx, 0, sizeof(aes_context));
		aes_set_key(&ctx, this->key, sizeof(this->key) * 8);

		// decrypt buffer
		XRNETBUFFER outBuffer(inBuffer.getSize());

		for (int i = 0; i < inBuffer.getSize(); i += 16) {
			uint8 in[16];
			uint8 out[16];

			memset(in, 0, 16);
			inBuffer.getData(i, 16, in);
			aes_decrypt(&ctx, in, out);
			outBuffer.setData(i, 16, out);
		}
		return XRNETLISTENER::onReceivePacket(XRNETPACKET(packet, outBuffer));
	}
	return XRNETLISTENER::onReceivePacket(packet);
}

bool XRNETCRYPTLISTENER::onSendPacket(XRNETPACKET &packet) {
	if (this->enabled) {
		const XRNETBUFFER & inBuffer = packet.getBuffer();

		if (inBuffer.getSize() % 16 != 0) {
			printf("packet size is not rounded to 16 bytes (%d bytes)\n", inBuffer.getSize());
			return false;
		}

		// setup encryption key
		aes_context ctx;

		memset(&ctx, 0, sizeof(aes_context));
		aes_set_key(&ctx, this->key, sizeof(this->key) * 8);

		// encrypt buffer
		XRNETBUFFER outBuffer(inBuffer.getSize());

		for (int i = 0; i < inBuffer.getSize(); i += 16) {
			uint8 in[16];
			uint8 out[16];

			memset(in, 0, 16);
			inBuffer.getData(i, 16, in);
			aes_encrypt(&ctx, in, out);
			outBuffer.setData(i, 16, out);
		}
		packet.setBuffer(outBuffer);
	}
	return XRNETLISTENER::onSendPacket(packet);
}
