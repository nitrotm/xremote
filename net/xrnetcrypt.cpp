/**
 * Xremote - client/server commons
 *
 * \author Antony Ducommun (nitro.tm@gmail.com)
 *
 * license : free of use for any purpose ;)
 */
#include "../xremote.h"
#include "../thirdparty/sha2.h"
#include "../thirdparty/aes.h"



XRNETAESPACKET::XRNETAESPACKET(const XRNETRAWPACKET &packet, bool encrypt, unsigned char key[32]) :
	XRNETRAWPACKET(packet),
	encrypted(encrypt) {
	aes_context ctx;
	uint8 in[16];
	uint8 out[16];

	memcpy(this->key, key, 32);
	memset(&ctx, 0, sizeof(aes_context));
	aes_set_key(&ctx, this->key, sizeof(this->key) * 8);

	for (int i = 0; i < XREMOTE_PACKET_SIZE; i += 16) {
		int size = XREMOTE_PACKET_SIZE - i;
		
		if (size > 16) {
			size = 16;
		}
		memset(in, 0, 16);
		memcpy(in, &this->buffer[i], size);
		if (encrypt) {
			aes_encrypt(&ctx, in, out);
		} else {
			aes_decrypt(&ctx, in, out);
		}
		memcpy(&this->buffer[i], out, size);
	}
}

XRNETAESPACKET::~XRNETAESPACKET() {
}



XRNETCRYPTLISTENER::XRNETCRYPTLISTENER(XRNETLISTENER *listener, const string &password) :
	XRNETLISTENER(listener) {
	memset(this->key, 0, sizeof(this->key));
	if (password.length() > 0) {
		sha256_context ctx;

		memset(&ctx, 0, sizeof(sha256_context));
		sha256_starts(&ctx);
		sha256_update(&ctx, (uint8*)password.c_str(), password.length());
		sha256_finish(&ctx, this->key);
		this->encrypt = true;
	} else {
		this->encrypt = false;
	}
}

XRNETCRYPTLISTENER::~XRNETCRYPTLISTENER() {
}

bool XRNETCRYPTLISTENER::onReceivePacket(PXRNETRAWPACKET packet) {
	if (this->encrypt) {
		// decrypt packet
		PXRNETRAWPACKET decryptedPacket = new XRNETAESPACKET(*packet, false, this->key);

		if (XRNETLISTENER::onReceivePacket(decryptedPacket)) {
			delete decryptedPacket;
			return true;
		}
		delete decryptedPacket;
		return false;
	}
	return XRNETLISTENER::onReceivePacket(packet);
}

PXRNETRAWPACKET XRNETCRYPTLISTENER::onSendPacket(PXRNETRAWPACKET packet) {
	packet = XRNETLISTENER::onSendPacket(packet);
	if (packet != NULL) {
		if (this->encrypt) {
			// encrypt packet
			PXRNETRAWPACKET encryptedPacket = new XRNETAESPACKET(*packet, true, this->key);

			delete packet;
			return encryptedPacket;
		}
	}
	return packet;
}
