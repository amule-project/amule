////////////////////////////////////////////////////////////////////////////////
// 
// This file contains a subset of the Crypto++ library (version 5.2.1), with 
// kind permission from Wei Dai. Please note that this file should not reflect 
// on the real Crypto++ library in any way, as this file been greatly mangled to 
// reduce the code-size, since aMule only makes use of RSA classes (for Secure 
// Identification) and the MD4 hashing class (for FileIDs). 
//
// For the full Crypto++ library, please refer to the official Crypto++ website,
// which can be found at, http://www.cryptopp.com
// 
////////////////////////////////////////////////////////////////////////////////
// 
// Compilation Copyright (c) 1995-2004 by Wei Dai.  All rights reserved.
// This copyright applies only to this software distribution package 
// as a compilation, and does not imply a copyright on any particular 
// file in the package.
// 
// The following files are copyrighted by their respective original authors,
// and their use is subject to additional licenses included in these files.
// 
// mars.cpp - Copyright 1998 Brian Gladman.

// All other files in this compilation are placed in the public domain by
// Wei Dai and other contributors.
// 
// I would like to thank the following authors for placing their works into
// the public domain:
// 
// Joan Daemen - 3way.cpp
// Leonard Janke - cast.cpp, seal.cpp
// Steve Reid - cast.cpp
// Phil Karn - des.cpp
// Michael Paul Johnson - diamond.cpp
// Andrew M. Kuchling - md2.cpp, md4.cpp
// Colin Plumb - md5.cpp, md5mac.cpp
// Seal Woods - rc6.cpp
// Chris Morgan - rijndael.cpp
// Paulo Baretto - rijndael.cpp, skipjack.cpp, square.cpp
// Richard De Moliner - safer.cpp
// Matthew Skala - twofish.cpp
// Kevin Springle - camellia.cpp, shacal2.cpp, ttmac.cpp, whrlpool.cpp, ripemd.cpp
// 
// Permission to use, copy, modify, and distribute this compilation for
// any purpose, including commercial applications, is hereby granted
// without fee, subject to the following restrictions:
// 
// 1. Any copy or modification of this compilation in any form, except
// in object code form as part of an application software, must include
// the above copyright notice and this license.
// 
// 2. Users of this software agree that any modification or extension
// they provide to Wei Dai will be considered public domain and not
// copyrighted unless it includes an explicit copyright notice.
// 
// 3. Wei Dai makes no warranty or representation that the operation of the
// software in this compilation will be error-free, and Wei Dai is under no
// obligation to provide any services, by way of maintenance, update, or
// otherwise.  THE SOFTWARE AND ANY DOCUMENTATION ARE PROVIDED "AS IS"
// WITHOUT EXPRESS OR IMPLIED WARRANTY INCLUDING, BUT NOT LIMITED TO,
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE. IN NO EVENT WILL WEI DAI OR ANY OTHER CONTRIBUTOR BE LIABLE FOR
// DIRECT, INCIDENTAL OR CONSEQUENTIAL DAMAGES, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
// 
// 4. Users will not use Wei Dai or any other contributor's name in any 
// publicity or advertising, without prior written consent in each case.
// 
// 5. Export of this software from the United States may require a
// specific license from the United States Government.  It is the
// responsibility of any person or organization contemplating export
// to obtain such a license before exporting.
// 
// 6. Certain parts of this software may be protected by patents.  It
// is the users' responsibility to obtain the appropriate
// licenses before using those parts.
// 
// If this compilation is used in object code form in an application
// software, acknowledgement of the author is not required but would be
// appreciated. The contribution of any useful modifications or extensions
// to Wei Dai is not required but would also be appreciated.
// 
////////////////////////////////////////////////////////////////////////////////
#include <wx/defs.h>   /* needed for endian setup */
#include "CryptoPP.h"

////////////////////////////////////////////////////////////////////////////////
#ifndef CRYPTOPP_FLTRIMPL_H
#define CRYPTOPP_FLTRIMPL_H

#define FILTER_BEGIN	\
	switch (m_continueAt)	\
	{	\
	case 0:	\
		m_inputPosition = 0;

#define FILTER_END_NO_MESSAGE_END_NO_RETURN	\
		break;	\
	default:	\
		assert(false);	\
	}

#define FILTER_END_NO_MESSAGE_END	\
	FILTER_END_NO_MESSAGE_END_NO_RETURN	\
	return 0;

/*
#define FILTER_END	\
	case -1:	\
		if (messageEnd && Output(-1, NULL, 0, messageEnd, blocking))	\
			return 1;	\
	FILTER_END_NO_MESSAGE_END
*/

#define FILTER_OUTPUT2(site, statement, output, length, messageEnd)	\
	{\
	case site:	\
	statement;	\
	if (Output(site, output, length, messageEnd, blocking))	\
		return STDMAX(1U, (unsigned int)length-m_inputPosition);\
	}

#define FILTER_OUTPUT(site, output, length, messageEnd)	\
	{\
	case site:	\
	if (Output(site, output, length, messageEnd, blocking))	\
		return STDMAX(1U, (unsigned int)length-m_inputPosition);\
	}

#endif
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
// cryptlib.cpp - written and placed in the public domain by Wei Dai


#ifndef CRYPTOPP_IMPORTS



NAMESPACE_BEGIN(CryptoPP)

CRYPTOPP_COMPILE_ASSERT_GLOBAL(sizeof(byte) == 1);
CRYPTOPP_COMPILE_ASSERT_GLOBAL(sizeof(word16) == 2);
CRYPTOPP_COMPILE_ASSERT_GLOBAL(sizeof(word32) == 4);
#ifdef WORD64_AVAILABLE
CRYPTOPP_COMPILE_ASSERT_GLOBAL(sizeof(word64) == 8);
#endif
#ifdef CRYPTOPP_NATIVE_DWORD_AVAILABLE
CRYPTOPP_COMPILE_ASSERT_GLOBAL(sizeof(dword) == 2*sizeof(word));
#endif

const std::string BufferedTransformation::NULL_CHANNEL;
const NullNameValuePairs g_nullNameValuePairs;

BufferedTransformation & TheBitBucket()
{
	static BitBucket bitBucket;
	return bitBucket;
}

Algorithm::Algorithm(bool /* checkSelfTestStatus */)
{
}

void SimpleKeyingInterface::SetKeyWithRounds(const byte *key, unsigned int length, int rounds)
{
	SetKey(key, length, MakeParameters(Name::Rounds(), rounds));
}

void SimpleKeyingInterface::SetKeyWithIV(const byte *key, unsigned int length, const byte *iv)
{
	SetKey(key, length, MakeParameters(Name::IV(), iv));
}

void SimpleKeyingInterface::ThrowIfInvalidKeyLength(const Algorithm &algorithm, unsigned int length)
{
	if (!IsValidKeyLength(length))
		throw InvalidKeyLength(algorithm.AlgorithmName(), length);
}

void SimpleKeyingInterface::ThrowIfResynchronizable()
{
	if (IsResynchronizable())
		throw InvalidArgument("SimpleKeyingInterface: this object requires an IV");
}

void SimpleKeyingInterface::ThrowIfInvalidIV(const byte *iv)
{
	if (!iv && !(IVRequirement() == INTERNALLY_GENERATED_IV || IVRequirement() == STRUCTURED_IV || !IsResynchronizable()))
		throw InvalidArgument("SimpleKeyingInterface: this object cannot use a null IV");
}

const byte * SimpleKeyingInterface::GetIVAndThrowIfInvalid(const NameValuePairs &params)
{
	const byte *iv;
	if (params.GetValue(Name::IV(), iv))
		ThrowIfInvalidIV(iv);
	else
		ThrowIfResynchronizable();
	return iv;
}

void BlockTransformation::ProcessAndXorMultipleBlocks(const byte *inBlocks, const byte *xorBlocks, byte *outBlocks, unsigned int numberOfBlocks) const
{
	unsigned int blockSize = BlockSize();
	while (numberOfBlocks--)
	{
		ProcessAndXorBlock(inBlocks, xorBlocks, outBlocks);
		inBlocks += blockSize;
		outBlocks += blockSize;
		if (xorBlocks)
			xorBlocks += blockSize;
	}
}

void StreamTransformation::ProcessLastBlock(byte *outString, const byte *inString, unsigned int length)
{
	assert(MinLastBlockSize() == 0);	// this function should be overriden otherwise

	if (length == MandatoryBlockSize())
		ProcessData(outString, inString, length);
	else if (length != 0)
		throw NotImplemented("StreamTransformation: this object does't support a special last block");
}

unsigned int RandomNumberGenerator::GenerateBit()
{
	return Parity(GenerateByte());
}

void RandomNumberGenerator::GenerateBlock(byte *output, unsigned int size)
{
	while (size--)
		*output++ = GenerateByte();
}

word32 RandomNumberGenerator::GenerateWord32(word32 min, word32 max)
{
	word32 range = max-min;
	const int maxBytes = BytePrecision(range);
	const int maxBits = BitPrecision(range);

	word32 value;

	do
	{
		value = 0;
		for (int i=0; i<maxBytes; i++)
			value = (value << 8) | GenerateByte();

		value = Crop(value, maxBits);
	} while (value > range);

	return value+min;
}

void RandomNumberGenerator::DiscardBytes(unsigned int n)
{
	while (n--)
		GenerateByte();
}

//! see NullRNG()
class ClassNullRNG : public RandomNumberGenerator
{
public:
	std::string AlgorithmName() const {return "NullRNG";}
	byte GenerateByte() {throw NotImplemented("NullRNG: NullRNG should only be passed to functions that don't need to generate random bytes");}
};

RandomNumberGenerator & NullRNG()
{
	static ClassNullRNG s_nullRNG;
	return s_nullRNG;
}

bool HashTransformation::TruncatedVerify(const byte *digestIn, unsigned int digestLength)
{
	ThrowIfInvalidTruncatedSize(digestLength);
	SecByteBlock digest(digestLength);
	TruncatedFinal(digest, digestLength);
	return memcmp(digest, digestIn, digestLength) == 0;
}

void HashTransformation::ThrowIfInvalidTruncatedSize(unsigned int size) const
{
	if (size > DigestSize())
		throw InvalidArgument("HashTransformation: can't truncate a " + IntToString(DigestSize()) + " byte digest to " + IntToString(size) + " bytes");
}

unsigned int BufferedTransformation::GetMaxWaitObjectCount() const
{
	const BufferedTransformation *t = AttachedTransformation();
	return t ? t->GetMaxWaitObjectCount() : 0;
}

void BufferedTransformation::GetWaitObjects(WaitObjectContainer &container)
{
	BufferedTransformation *t = AttachedTransformation();
	if (t)
		t->GetWaitObjects(container);
}

void BufferedTransformation::Initialize(const NameValuePairs &parameters, int /* propagation */)
{
	assert(!AttachedTransformation());
	IsolatedInitialize(parameters);
}

bool BufferedTransformation::Flush(bool hardFlush, int /* propagation */, bool blocking)
{
	assert(!AttachedTransformation());
	return IsolatedFlush(hardFlush, blocking);
}

bool BufferedTransformation::MessageSeriesEnd(int /* propagation */, bool blocking)
{
	assert(!AttachedTransformation());
	return IsolatedMessageSeriesEnd(blocking);
}

byte * BufferedTransformation::ChannelCreatePutSpace(const std::string &channel, unsigned int &size)
{
	if (channel.empty())
		return CreatePutSpace(size);
	else
		throw NoChannelSupport();
}

unsigned int BufferedTransformation::ChannelPut2(const std::string &channel, const byte *begin, unsigned int length, int messageEnd, bool blocking)
{
	if (channel.empty())
		return Put2(begin, length, messageEnd, blocking);
	else
		throw NoChannelSupport();
}

unsigned int BufferedTransformation::ChannelPutModifiable2(const std::string &channel, byte *begin, unsigned int length, int messageEnd, bool blocking)
{
	if (channel.empty())
		return PutModifiable2(begin, length, messageEnd, blocking);
	else
		return ChannelPut2(channel, begin, length, messageEnd, blocking);
}

bool BufferedTransformation::ChannelFlush(const std::string &channel, bool completeFlush, int propagation, bool blocking)
{
	if (channel.empty())
		return Flush(completeFlush, propagation, blocking);
	else
		throw NoChannelSupport();
}

bool BufferedTransformation::ChannelMessageSeriesEnd(const std::string &channel, int propagation, bool blocking)
{
	if (channel.empty())
		return MessageSeriesEnd(propagation, blocking);
	else
		throw NoChannelSupport();
}

unsigned long BufferedTransformation::MaxRetrievable() const
{
	if (AttachedTransformation())
		return AttachedTransformation()->MaxRetrievable();
	else
		return CopyTo(TheBitBucket());
}

bool BufferedTransformation::AnyRetrievable() const
{
	if (AttachedTransformation())
		return AttachedTransformation()->AnyRetrievable();
	else
	{
		byte b;
		return Peek(b) != 0;
	}
}

unsigned int BufferedTransformation::Get(byte &outByte)
{
	if (AttachedTransformation())
		return AttachedTransformation()->Get(outByte);
	else
		return Get(&outByte, 1);
}

unsigned int BufferedTransformation::Get(byte *outString, unsigned int getMax)
{
	if (AttachedTransformation())
		return AttachedTransformation()->Get(outString, getMax);
	else
	{
		ArraySink arraySink(outString, getMax);
		return TransferTo(arraySink, getMax);
	}
}

unsigned int BufferedTransformation::Peek(byte &outByte) const
{
	if (AttachedTransformation())
		return AttachedTransformation()->Peek(outByte);
	else
		return Peek(&outByte, 1);
}

unsigned int BufferedTransformation::Peek(byte *outString, unsigned int peekMax) const
{
	if (AttachedTransformation())
		return AttachedTransformation()->Peek(outString, peekMax);
	else
	{
		ArraySink arraySink(outString, peekMax);
		return CopyTo(arraySink, peekMax);
	}
}

unsigned long BufferedTransformation::Skip(unsigned long skipMax)
{
	if (AttachedTransformation())
		return AttachedTransformation()->Skip(skipMax);
	else
		return TransferTo(TheBitBucket(), skipMax);
}

unsigned long BufferedTransformation::TotalBytesRetrievable() const
{
	if (AttachedTransformation())
		return AttachedTransformation()->TotalBytesRetrievable();
	else
		return MaxRetrievable();
}

unsigned int BufferedTransformation::NumberOfMessages() const
{
	if (AttachedTransformation())
		return AttachedTransformation()->NumberOfMessages();
	else
		return CopyMessagesTo(TheBitBucket());
}

bool BufferedTransformation::AnyMessages() const
{
	if (AttachedTransformation())
		return AttachedTransformation()->AnyMessages();
	else
		return NumberOfMessages() != 0;
}

bool BufferedTransformation::GetNextMessage()
{
	if (AttachedTransformation())
		return AttachedTransformation()->GetNextMessage();
	else
	{
		assert(!AnyMessages());
		return false;
	}
}

unsigned int BufferedTransformation::SkipMessages(unsigned int count)
{
	if (AttachedTransformation())
		return AttachedTransformation()->SkipMessages(count);
	else
		return TransferMessagesTo(TheBitBucket(), count);
}

unsigned int BufferedTransformation::TransferMessagesTo2(BufferedTransformation &target, unsigned int &messageCount, const std::string &channel, bool blocking)
{
	if (AttachedTransformation())
		return AttachedTransformation()->TransferMessagesTo2(target, messageCount, channel, blocking);
	else
	{
		unsigned int maxMessages = messageCount;
		for (messageCount=0; messageCount < maxMessages && AnyMessages(); messageCount++)
		{
			unsigned int blockedBytes;
			unsigned long transferredBytes;

			while (AnyRetrievable())
			{
				transferredBytes = ULONG_MAX;
				blockedBytes = TransferTo2(target, transferredBytes, channel, blocking);
				if (blockedBytes > 0)
					return blockedBytes;
			}

			if (target.ChannelMessageEnd(channel, GetAutoSignalPropagation(), blocking))
				return 1;

			bool result = GetNextMessage();
			assert(result);
		}
		return 0;
	}
}

unsigned int BufferedTransformation::CopyMessagesTo(BufferedTransformation &target, unsigned int count, const std::string &channel) const
{
	if (AttachedTransformation())
		return AttachedTransformation()->CopyMessagesTo(target, count, channel);
	else
		return 0;
}

void BufferedTransformation::SkipAll()
{
	if (AttachedTransformation())
		AttachedTransformation()->SkipAll();
	else
	{
		while (SkipMessages()) {}
		while (Skip()) {}
	}
}

unsigned int BufferedTransformation::TransferAllTo2(BufferedTransformation &target, const std::string &channel, bool blocking)
{
	if (AttachedTransformation())
		return AttachedTransformation()->TransferAllTo2(target, channel, blocking);
	else
	{
		assert(!NumberOfMessageSeries());

		unsigned int messageCount;
		do
		{
			messageCount = UINT_MAX;
			unsigned int blockedBytes = TransferMessagesTo2(target, messageCount, channel, blocking);
			if (blockedBytes)
				return blockedBytes;
		}
		while (messageCount != 0);

		unsigned long byteCount;
		do
		{
			byteCount = ULONG_MAX;
			unsigned int blockedBytes = TransferTo2(target, byteCount, channel, blocking);
			if (blockedBytes)
				return blockedBytes;
		}
		while (byteCount != 0);

		return 0;
	}
}

void BufferedTransformation::CopyAllTo(BufferedTransformation &target, const std::string &channel) const
{
	if (AttachedTransformation())
		AttachedTransformation()->CopyAllTo(target, channel);
	else
	{
		assert(!NumberOfMessageSeries());
		while (CopyMessagesTo(target, UINT_MAX, channel)) {}
	}
}

void BufferedTransformation::SetRetrievalChannel(const std::string &channel)
{
	if (AttachedTransformation())
		AttachedTransformation()->SetRetrievalChannel(channel);
}

unsigned int BufferedTransformation::ChannelPutWord16(const std::string &channel, word16 value, ByteOrder order, bool blocking)
{
	FixedSizeSecBlock<byte, 2> buf;
	PutWord(false, order, buf, value);
	return ChannelPut(channel, buf, 2, blocking);
}

unsigned int BufferedTransformation::ChannelPutWord32(const std::string &channel, word32 value, ByteOrder order, bool blocking)
{
	FixedSizeSecBlock<byte, 4> buf;
	PutWord(false, order, buf, value);
	return ChannelPut(channel, buf, 4, blocking);
}

unsigned int BufferedTransformation::PutWord16(word16 value, ByteOrder order, bool blocking)
{
	return ChannelPutWord16(NULL_CHANNEL, value, order, blocking);
}

unsigned int BufferedTransformation::PutWord32(word32 value, ByteOrder order, bool blocking)
{
	return ChannelPutWord32(NULL_CHANNEL, value, order, blocking);
}

unsigned int BufferedTransformation::PeekWord16(word16 &value, ByteOrder order)
{
	byte buf[2] = {0, 0};
	unsigned int len = Peek(buf, 2);

	if (order)
		value = (buf[0] << 8) | buf[1];
	else
		value = (buf[1] << 8) | buf[0];

	return len;
}

unsigned int BufferedTransformation::PeekWord32(word32 &value, ByteOrder order)
{
	byte buf[4] = {0, 0, 0, 0};
	unsigned int len = Peek(buf, 4);

	if (order)
		value = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf [3];
	else
		value = (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf [0];

	return len;
}

unsigned int BufferedTransformation::GetWord16(word16 &value, ByteOrder order)
{
	return Skip(PeekWord16(value, order));
}

unsigned int BufferedTransformation::GetWord32(word32 &value, ByteOrder order)
{
	return Skip(PeekWord32(value, order));
}

void BufferedTransformation::Attach(BufferedTransformation *newOut)
{
	if (AttachedTransformation() && AttachedTransformation()->Attachable())
		AttachedTransformation()->Attach(newOut);
	else
		Detach(newOut);
}

void GeneratableCryptoMaterial::GenerateRandomWithKeySize(RandomNumberGenerator &rng, unsigned int keySize)
{
	GenerateRandom(rng, MakeParameters("KeySize", (int)keySize));
}

class PK_DefaultEncryptionFilter : public Unflushable<Filter>
{
public:
	PK_DefaultEncryptionFilter(RandomNumberGenerator &rng, const PK_Encryptor &encryptor, BufferedTransformation *attachment, const NameValuePairs &parameters)
		: m_rng(rng), m_encryptor(encryptor), m_parameters(parameters)
	{
		Detach(attachment);
	}

	unsigned int Put2(const byte *inString, unsigned int length, int messageEnd, bool blocking)
	{
		FILTER_BEGIN;
		m_plaintextQueue.Put(inString, length);

		if (messageEnd)
		{
			{
			unsigned int plaintextLength = m_plaintextQueue.CurrentSize();
			unsigned int ciphertextLength = m_encryptor.CiphertextLength(plaintextLength);

			SecByteBlock plaintext(plaintextLength);
			m_plaintextQueue.Get(plaintext, plaintextLength);
			m_ciphertext.resize(ciphertextLength);
			m_encryptor.Encrypt(m_rng, plaintext, plaintextLength, m_ciphertext, m_parameters);
			}
			
			FILTER_OUTPUT(1, m_ciphertext, m_ciphertext.size(), messageEnd);
		}
		FILTER_END_NO_MESSAGE_END;
	}

	RandomNumberGenerator &m_rng;
	const PK_Encryptor &m_encryptor;
	const NameValuePairs &m_parameters;
	ByteQueue m_plaintextQueue;
	SecByteBlock m_ciphertext;
};

BufferedTransformation * PK_Encryptor::CreateEncryptionFilter(RandomNumberGenerator &rng, BufferedTransformation *attachment, const NameValuePairs &parameters) const
{
	return new PK_DefaultEncryptionFilter(rng, *this, attachment, parameters);
}

unsigned int PK_Signer::Sign(RandomNumberGenerator &rng, PK_MessageAccumulator *messageAccumulator, byte *signature) const
{
	std::auto_ptr<PK_MessageAccumulator> m(messageAccumulator);
	return SignAndRestart(rng, *m, signature, false);
}

unsigned int PK_Signer::SignMessage(RandomNumberGenerator &rng, const byte *message, unsigned int messageLen, byte *signature) const
{
	std::auto_ptr<PK_MessageAccumulator> m(NewSignatureAccumulator(rng));
	m->Update(message, messageLen);
	return SignAndRestart(rng, *m, signature, false);
}

unsigned int PK_Signer::SignMessageWithRecovery(RandomNumberGenerator &rng, const byte *recoverableMessage, unsigned int recoverableMessageLength, 
	const byte *nonrecoverableMessage, unsigned int nonrecoverableMessageLength, byte *signature) const
{
	std::auto_ptr<PK_MessageAccumulator> m(NewSignatureAccumulator(rng));
	InputRecoverableMessage(*m, recoverableMessage, recoverableMessageLength);
	m->Update(nonrecoverableMessage, nonrecoverableMessageLength);
	return SignAndRestart(rng, *m, signature, false);
}

bool PK_Verifier::Verify(PK_MessageAccumulator *messageAccumulator) const
{
	std::auto_ptr<PK_MessageAccumulator> m(messageAccumulator);
	return VerifyAndRestart(*m);
}

bool PK_Verifier::VerifyMessage(const byte *message, unsigned int messageLen, const byte *signature, unsigned int signatureLength) const
{
	std::auto_ptr<PK_MessageAccumulator> m(NewVerificationAccumulator());
	InputSignature(*m, signature, signatureLength);
	m->Update(message, messageLen);
	return VerifyAndRestart(*m);
}

DecodingResult PK_Verifier::Recover(byte *recoveredMessage, PK_MessageAccumulator *messageAccumulator) const
{
	std::auto_ptr<PK_MessageAccumulator> m(messageAccumulator);
	return RecoverAndRestart(recoveredMessage, *m);
}

DecodingResult PK_Verifier::RecoverMessage(byte *recoveredMessage, 
	const byte *nonrecoverableMessage, unsigned int nonrecoverableMessageLength, 
	const byte *signature, unsigned int signatureLength) const
{
	std::auto_ptr<PK_MessageAccumulator> m(NewVerificationAccumulator());
	InputSignature(*m, signature, signatureLength);
	m->Update(nonrecoverableMessage, nonrecoverableMessageLength);
	return RecoverAndRestart(recoveredMessage, *m);
}

NAMESPACE_END

#endif
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
#ifndef CRYPTOPP_WORDS_H
#define CRYPTOPP_WORDS_H


NAMESPACE_BEGIN(CryptoPP)

inline unsigned int CountWords(const word *X, unsigned int N)
{
	while (N && X[N-1]==0)
		N--;
	return N;
}

inline void SetWords(word *r, word a, unsigned int n)
{
	for (unsigned int i=0; i<n; i++)
		r[i] = a;
}

inline void CopyWords(word *r, const word *a, unsigned int n)
{
	for (unsigned int i=0; i<n; i++)
		r[i] = a[i];
}

inline void XorWords(word *r, const word *a, const word *b, unsigned int n)
{
	for (unsigned int i=0; i<n; i++)
		r[i] = a[i] ^ b[i];
}

inline void XorWords(word *r, const word *a, unsigned int n)
{
	for (unsigned int i=0; i<n; i++)
		r[i] ^= a[i];
}

inline word ShiftWordsLeftByBits(word *r, unsigned int n, unsigned int shiftBits)
{
	assert (shiftBits<WORD_BITS);
	word u, carry=0;
	if (shiftBits)
		for (unsigned int i=0; i<n; i++)
		{
			u = r[i];
			r[i] = (u << shiftBits) | carry;
			carry = u >> (WORD_BITS-shiftBits);
		}
	return carry;
}

inline word ShiftWordsRightByBits(word *r, unsigned int n, unsigned int shiftBits)
{
	assert (shiftBits<WORD_BITS);
	word u, carry=0;
	if (shiftBits)
		for (int i=n-1; i>=0; i--)
		{
			u = r[i];
			r[i] = (u >> shiftBits) | carry;
			carry = u << (WORD_BITS-shiftBits);
		}
	return carry;
}

inline void ShiftWordsLeftByWords(word *r, unsigned int n, unsigned int shiftWords)
{
	shiftWords = STDMIN(shiftWords, n);
	if (shiftWords)
	{
		for (unsigned int i=n-1; i>=shiftWords; i--)
			r[i] = r[i-shiftWords];
		SetWords(r, 0, shiftWords);
	}
}

inline void ShiftWordsRightByWords(word *r, unsigned int n, unsigned int shiftWords)
{
	shiftWords = STDMIN(shiftWords, n);
	if (shiftWords)
	{
		for (unsigned int i=0; i+shiftWords<n; i++)
			r[i] = r[i+shiftWords];
		SetWords(r+n-shiftWords, 0, shiftWords);
	}
}

NAMESPACE_END

#endif
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
// misc.cpp - written and placed in the public domain by Wei Dai


#ifndef CRYPTOPP_IMPORTS


NAMESPACE_BEGIN(CryptoPP)

void xorbuf(byte *buf, const byte *mask, unsigned int count)
{
	if (((size_t)buf | (size_t)mask | count) % WORD_SIZE == 0)
		XorWords((word *)buf, (const word *)mask, count/WORD_SIZE);
	else
	{
		for (unsigned int i=0; i<count; i++)
			buf[i] ^= mask[i];
	}
}

void xorbuf(byte *output, const byte *input, const byte *mask, unsigned int count)
{
	if (((size_t)output | (size_t)input | (size_t)mask | count) % WORD_SIZE == 0)
		XorWords((word *)output, (const word *)input, (const word *)mask, count/WORD_SIZE);
	else
	{
		for (unsigned int i=0; i<count; i++)
			output[i] = input[i] ^ mask[i];
	}
}

unsigned int Parity(unsigned long value)
{
	for (unsigned int i=8*sizeof(value)/2; i>0; i/=2)
		value ^= value >> i;
	return (unsigned int)value&1;
}

unsigned int BytePrecision(unsigned long value)
{
	unsigned int i;
	for (i=sizeof(value); i; --i)
		if (value >> (i-1)*8)
			break;

	return i;
}

unsigned int BitPrecision(unsigned long value)
{
	if (!value)
		return 0;

	unsigned int l=0, h=8*sizeof(value);

	while (h-l > 1)
	{
		unsigned int t = (l+h)/2;
		if (value >> t)
			l = t;
		else
			h = t;
	}

	return h;
}

unsigned long Crop(unsigned long value, unsigned int size)
{
	if (size < 8*sizeof(value))
    	return (value & ((1L << size) - 1));
	else
		return value;
}

#if !(defined(_MSC_VER) && (_MSC_VER < 1300))
using std::new_handler;
using std::set_new_handler;
#endif

void CallNewHandler()
{
	new_handler newHandler = set_new_handler(NULL);
	if (newHandler)
		set_new_handler(newHandler);

	if (newHandler)
		newHandler();
	else
		throw std::bad_alloc();
}

NAMESPACE_END

#endif
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
// nbtheory.h - written and placed in the public domain by Wei Dai

#ifndef CRYPTOPP_NBTHEORY_H
#define CRYPTOPP_NBTHEORY_H


NAMESPACE_BEGIN(CryptoPP)

// obtain pointer to small prime table and get its size
CRYPTOPP_DLL const word16 * GetPrimeTable(unsigned int &size);

// ************ primality testing ****************

CRYPTOPP_DLL bool IsSmallPrime(const Integer &p);

// returns true if p is divisible by some prime less than bound
// bound not be greater than the largest entry in the prime table
CRYPTOPP_DLL bool TrialDivision(const Integer &p, unsigned bound);

// returns true if p is NOT divisible by small primes
CRYPTOPP_DLL bool SmallDivisorsTest(const Integer &p);

CRYPTOPP_DLL bool IsStrongProbablePrime(const Integer &n, const Integer &b);
CRYPTOPP_DLL bool IsStrongLucasProbablePrime(const Integer &n);

// Rabin-Miller primality test, i.e. repeating the strong probable prime test 
// for several rounds with random bases
CRYPTOPP_DLL bool RabinMillerTest(RandomNumberGenerator &rng, const Integer &w, unsigned int rounds);

// primality test, used to generate primes
CRYPTOPP_DLL bool IsPrime(const Integer &p);

// more reliable than IsPrime(), used to verify primes generated by others
CRYPTOPP_DLL bool VerifyPrime(RandomNumberGenerator &rng, const Integer &p, unsigned int level = 1);

class PrimeSelector
{
public:
	virtual ~PrimeSelector() {};
	const PrimeSelector *GetSelectorPointer() const {return this;}
	virtual bool IsAcceptable(const Integer &candidate) const =0;
};

// use a fast sieve to find the first probable prime in {x | p<=x<=max and x%mod==equiv}
// returns true iff successful, value of p is undefined if no such prime exists
CRYPTOPP_DLL bool FirstPrime(Integer &p, const Integer &max, const Integer &equiv, const Integer &mod, const PrimeSelector *pSelector);

CRYPTOPP_DLL unsigned int PrimeSearchInterval(const Integer &max);

CRYPTOPP_DLL AlgorithmParameters<AlgorithmParameters<AlgorithmParameters<NullNameValuePairs, Integer::RandomNumberType>, Integer>, Integer>
	MakeParametersForTwoPrimesOfEqualSize(unsigned int productBitLength);

// ********** other number theoretic functions ************

inline Integer GCD(const Integer &a, const Integer &b)
	{return Integer::Gcd(a,b);}
inline bool RelativelyPrime(const Integer &a, const Integer &b)
	{return Integer::Gcd(a,b) == Integer::One();}
inline Integer LCM(const Integer &a, const Integer &b)
	{return a/Integer::Gcd(a,b)*b;}
inline Integer EuclideanMultiplicativeInverse(const Integer &a, const Integer &b)
	{return a.InverseMod(b);}

// use Chinese Remainder Theorem to calculate x given x mod p and x mod q
CRYPTOPP_DLL Integer CRT(const Integer &xp, const Integer &p, const Integer &xq, const Integer &q);
// use this one if u = inverse of p mod q has been precalculated
CRYPTOPP_DLL Integer CRT(const Integer &xp, const Integer &p, const Integer &xq, const Integer &q, const Integer &u);

// if b is prime, then Jacobi(a, b) returns 0 if a%b==0, 1 if a is quadratic residue mod b, -1 otherwise
// check a number theory book for what Jacobi symbol means when b is not prime
CRYPTOPP_DLL int Jacobi(const Integer &a, const Integer &b);

// calculates the Lucas function V_e(p, 1) mod n
CRYPTOPP_DLL Integer Lucas(const Integer &e, const Integer &p, const Integer &n);
// calculates x such that m==Lucas(e, x, p*q), p q primes
CRYPTOPP_DLL Integer InverseLucas(const Integer &e, const Integer &m, const Integer &p, const Integer &q);
// use this one if u=inverse of p mod q has been precalculated
CRYPTOPP_DLL Integer InverseLucas(const Integer &e, const Integer &m, const Integer &p, const Integer &q, const Integer &u);

inline Integer ModularExponentiation(const Integer &a, const Integer &e, const Integer &m)
	{return a_exp_b_mod_c(a, e, m);}
// returns x such that a==ModularExponentiation(x, e, p*q), p q primes,
// and e relatively prime to (p-1)*(q-1)
CRYPTOPP_DLL Integer ModularRoot(const Integer &a, const Integer &e, const Integer &p, const Integer &q);
// use this one if dp=d%(p-1), dq=d%(q-1), (d is inverse of e mod (p-1)*(q-1))
// and u=inverse of p mod q have been precalculated
CRYPTOPP_DLL Integer ModularRoot(const Integer &a, const Integer &dp, const Integer &dq, const Integer &p, const Integer &q, const Integer &u);

NAMESPACE_END

#endif
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
// nbtheory.cpp - written and placed in the public domain by Wei Dai


#ifndef CRYPTOPP_IMPORTS



NAMESPACE_BEGIN(CryptoPP)

const word s_lastSmallPrime = 32719;

struct NewPrimeTable
{
	std::vector<word16> * operator()() const
	{
		const unsigned int maxPrimeTableSize = 3511;

		std::auto_ptr<std::vector<word16> > pPrimeTable(new std::vector<word16>);
		std::vector<word16> &primeTable = *pPrimeTable;
		primeTable.reserve(maxPrimeTableSize);

		primeTable.push_back(2);
		unsigned int testEntriesEnd = 1;
		
		for (unsigned int p=3; p<=s_lastSmallPrime; p+=2)
		{
			unsigned int j;
			for (j=1; j<testEntriesEnd; j++)
				if (p%primeTable[j] == 0)
					break;
			if (j == testEntriesEnd)
			{
				primeTable.push_back(p);
				testEntriesEnd = STDMIN((size_t)54U, primeTable.size());
			}
		}

		return pPrimeTable.release();
	}
};

const word16 * GetPrimeTable(unsigned int &size)
{
	const std::vector<word16> &primeTable = Singleton<std::vector<word16>, NewPrimeTable>().Ref();
	size = primeTable.size();
	return &primeTable[0];
}

bool IsSmallPrime(const Integer &p)
{
	unsigned int primeTableSize;
	const word16 * primeTable = GetPrimeTable(primeTableSize);

	if (p.IsPositive() && p <= primeTable[primeTableSize-1])
		return std::binary_search(primeTable, primeTable+primeTableSize, (word16)p.ConvertToLong());
	else
		return false;
}

bool TrialDivision(const Integer &p, unsigned bound)
{
	unsigned int primeTableSize;
	const word16 * primeTable = GetPrimeTable(primeTableSize);

	assert(primeTable[primeTableSize-1] >= bound);

	unsigned int i;
	for (i = 0; primeTable[i]<bound; i++)
		if ((p % primeTable[i]) == 0)
			return true;

	if (bound == primeTable[i])
		return (p % bound == 0);
	else
		return false;
}

bool SmallDivisorsTest(const Integer &p)
{
	unsigned int primeTableSize;
	const word16 * primeTable = GetPrimeTable(primeTableSize);
	return !TrialDivision(p, primeTable[primeTableSize-1]);
}

bool IsStrongProbablePrime(const Integer &n, const Integer &b)
{
	if (n <= 3)
		return n==2 || n==3;

	assert(n>3 && b>1 && b<n-1);

	if ((n.IsEven() && n!=2) || GCD(b, n) != 1)
		return false;

	Integer nminus1 = (n-1);
	unsigned int a;

	// calculate a = largest power of 2 that divides (n-1)
	for (a=0; ; a++)
		if (nminus1.GetBit(a))
			break;
	Integer m = nminus1>>a;

	Integer z = a_exp_b_mod_c(b, m, n);
	if (z==1 || z==nminus1)
		return true;
	for (unsigned j=1; j<a; j++)
	{
		z = z.Squared()%n;
		if (z==nminus1)
			return true;
		if (z==1)
			return false;
	}
	return false;
}

bool RabinMillerTest(RandomNumberGenerator &rng, const Integer &n, unsigned int rounds)
{
	if (n <= 3)
		return n==2 || n==3;

	assert(n>3);

	Integer b;
	for (unsigned int i=0; i<rounds; i++)
	{
		b.Randomize(rng, 2, n-2);
		if (!IsStrongProbablePrime(n, b))
			return false;
	}
	return true;
}

bool IsStrongLucasProbablePrime(const Integer &n)
{
	if (n <= 1)
		return false;

	if (n.IsEven())
		return n==2;

	assert(n>2);

	Integer b=3;
	unsigned int i=0;
	int j;

	while ((j=Jacobi(b.Squared()-4, n)) == 1)
	{
		if (++i==64 && n.IsSquare())	// avoid infinite loop if n is a square
			return false;
		++b; ++b;
	}

	if (j==0) 
		return false;

	Integer n1 = n+1;
	unsigned int a;

	// calculate a = largest power of 2 that divides n1
	for (a=0; ; a++)
		if (n1.GetBit(a))
			break;
	Integer m = n1>>a;

	Integer z = Lucas(m, b, n);
	if (z==2 || z==n-2)
		return true;
	for (i=1; i<a; i++)
	{
		z = (z.Squared()-2)%n;
		if (z==n-2)
			return true;
		if (z==2)
			return false;
	}
	return false;
}

struct NewLastSmallPrimeSquared
{
	Integer * operator()() const
	{
		return new Integer(Integer(s_lastSmallPrime).Squared());
	}
};

bool IsPrime(const Integer &p)
{
	if (p <= s_lastSmallPrime)
		return IsSmallPrime(p);
	else if (p <= Singleton<Integer, NewLastSmallPrimeSquared>().Ref())
		return SmallDivisorsTest(p);
	else
		return SmallDivisorsTest(p) && IsStrongProbablePrime(p, 3) && IsStrongLucasProbablePrime(p);
}

bool VerifyPrime(RandomNumberGenerator &rng, const Integer &p, unsigned int level)
{
	bool pass = IsPrime(p) && RabinMillerTest(rng, p, 1);
	if (level >= 1)
		pass = pass && RabinMillerTest(rng, p, 10);
	return pass;
}

unsigned int PrimeSearchInterval(const Integer &max)
{
	return max.BitCount();
}

static inline bool FastProbablePrimeTest(const Integer &n)
{
	return IsStrongProbablePrime(n,2);
}

AlgorithmParameters<AlgorithmParameters<AlgorithmParameters<NullNameValuePairs, Integer::RandomNumberType>, Integer>, Integer>
	MakeParametersForTwoPrimesOfEqualSize(unsigned int productBitLength)
{
	if (productBitLength < 16)
		throw InvalidArgument("invalid bit length");

	Integer minP, maxP;

	if (productBitLength%2==0)
	{
		minP = Integer(182) << (productBitLength/2-8);
		maxP = Integer::Power2(productBitLength/2)-1;
	}
	else
	{
		minP = Integer::Power2((productBitLength-1)/2);
		maxP = Integer(181) << ((productBitLength+1)/2-8);
	}

	return MakeParameters("RandomNumberType", Integer::PRIME)("Min", minP)("Max", maxP);
}

class PrimeSieve
{
public:
	// delta == 1 or -1 means double sieve with p = 2*q + delta
	PrimeSieve(const Integer &first, const Integer &last, const Integer &step, signed int delta=0);
	bool NextCandidate(Integer &c);

	void DoSieve();
	static void SieveSingle(std::vector<bool> &sieve, word16 p, const Integer &first, const Integer &step, word16 stepInv);

	Integer m_first, m_last, m_step;
	signed int m_delta;
	word m_next;
	std::vector<bool> m_sieve;
};

PrimeSieve::PrimeSieve(const Integer &first, const Integer &last, const Integer &step, signed int delta)
	: m_first(first), m_last(last), m_step(step), m_delta(delta), m_next(0)
{
	DoSieve();
}

bool PrimeSieve::NextCandidate(Integer &c)
{
	m_next = std::find(m_sieve.begin()+m_next, m_sieve.end(), false) - m_sieve.begin();
	if (m_next == m_sieve.size())
	{
		m_first += m_sieve.size()*m_step;
		if (m_first > m_last)
			return false;
		else
		{
			m_next = 0;
			DoSieve();
			return NextCandidate(c);
		}
	}
	else
	{
		c = m_first + m_next*m_step;
		++m_next;
		return true;
	}
}

void PrimeSieve::SieveSingle(std::vector<bool> &sieve, word16 p, const Integer &first, const Integer &step, word16 stepInv)
{
	if (stepInv)
	{
		unsigned int sieveSize = sieve.size();
		word j = word((word32(p-(first%p))*stepInv) % p);
		// if the first multiple of p is p, skip it
		if (first.WordCount() <= 1 && first + step*j == p)
			j += p;
		for (; j < sieveSize; j += p)
			sieve[j] = true;
	}
}

void PrimeSieve::DoSieve()
{
	unsigned int primeTableSize;
	const word16 * primeTable = GetPrimeTable(primeTableSize);

	const unsigned int maxSieveSize = 32768;
	unsigned int sieveSize = STDMIN(Integer(maxSieveSize), (m_last-m_first)/m_step+1).ConvertToLong();

	m_sieve.clear();
	m_sieve.resize(sieveSize, false);

	if (m_delta == 0)
	{
		for (unsigned int i = 0; i < primeTableSize; ++i)
			SieveSingle(m_sieve, primeTable[i], m_first, m_step, m_step.InverseMod(primeTable[i]));
	}
	else
	{
		assert(m_step%2==0);
		Integer qFirst = (m_first-m_delta) >> 1;
		Integer halfStep = m_step >> 1;
		for (unsigned int i = 0; i < primeTableSize; ++i)
		{
			word16 p = primeTable[i];
			word16 stepInv = m_step.InverseMod(p);
			SieveSingle(m_sieve, p, m_first, m_step, stepInv);

			word16 halfStepInv = 2*stepInv < p ? 2*stepInv : 2*stepInv-p;
			SieveSingle(m_sieve, p, qFirst, halfStep, halfStepInv);
		}
	}
}

bool FirstPrime(Integer &p, const Integer &max, const Integer &equiv, const Integer &mod, const PrimeSelector *pSelector)
{
	assert(!equiv.IsNegative() && equiv < mod);

	Integer gcd = GCD(equiv, mod);
	if (gcd != Integer::One())
	{
		// the only possible prime p such that p%mod==equiv where GCD(mod,equiv)!=1 is GCD(mod,equiv)
		if (p <= gcd && gcd <= max && IsPrime(gcd) && (!pSelector || pSelector->IsAcceptable(gcd)))
		{
			p = gcd;
			return true;
		}
		else
			return false;
	}

	unsigned int primeTableSize;
	const word16 * primeTable = GetPrimeTable(primeTableSize);

	if (p <= primeTable[primeTableSize-1])
	{
		const word16 *pItr;

		--p;
		if (p.IsPositive())
			pItr = std::upper_bound(primeTable, primeTable+primeTableSize, (word)p.ConvertToLong());
		else
			pItr = primeTable;

		while (pItr < primeTable+primeTableSize && !(*pItr%mod == equiv && (!pSelector || pSelector->IsAcceptable(*pItr))))
			++pItr;

		if (pItr < primeTable+primeTableSize)
		{
			p = *pItr;
			return p <= max;
		}

		p = primeTable[primeTableSize-1]+1;
	}

	assert(p > primeTable[primeTableSize-1]);

	if (mod.IsOdd())
		return FirstPrime(p, max, CRT(equiv, mod, 1, 2, 1), mod<<1, pSelector);

	p += (equiv-p)%mod;

	if (p>max)
		return false;

	PrimeSieve sieve(p, max, mod);

	while (sieve.NextCandidate(p))
	{
		if ((!pSelector || pSelector->IsAcceptable(p)) && FastProbablePrimeTest(p) && IsPrime(p))
			return true;
	}

	return false;
}

Integer CRT(const Integer &xp, const Integer &p, const Integer &xq, const Integer &q, const Integer &u)
{
	// isn't operator overloading great?
	return p * (u * (xq-xp) % q) + xp;
}

Integer CRT(const Integer &xp, const Integer &p, const Integer &xq, const Integer &q)
{
	return CRT(xp, p, xq, q, EuclideanMultiplicativeInverse(p, q));
}

Integer ModularRoot(const Integer &a, const Integer &dp, const Integer &dq,
					const Integer &p, const Integer &q, const Integer &u)
{
	Integer p2 = ModularExponentiation((a % p), dp, p);
	Integer q2 = ModularExponentiation((a % q), dq, q);
	return CRT(p2, p, q2, q, u);
}

Integer ModularRoot(const Integer &a, const Integer &e,
					const Integer &p, const Integer &q)
{
	Integer dp = EuclideanMultiplicativeInverse(e, p-1);
	Integer dq = EuclideanMultiplicativeInverse(e, q-1);
	Integer u = EuclideanMultiplicativeInverse(p, q);
	assert(!!dp && !!dq && !!u);
	return ModularRoot(a, dp, dq, p, q, u);
}

int Jacobi(const Integer &aIn, const Integer &bIn)
{
	assert(bIn.IsOdd());

	Integer b = bIn, a = aIn%bIn;
	int result = 1;

	while (!!a)
	{
		unsigned i=0;
		while (a.GetBit(i)==0)
			i++;
		a>>=i;

		if (i%2==1 && (b%8==3 || b%8==5))
			result = -result;

		if (a%4==3 && b%4==3)
			result = -result;

		std::swap(a, b);
		a %= b;
	}

	return (b==1) ? result : 0;
}

Integer Lucas(const Integer &e, const Integer &pIn, const Integer &n)
{
	unsigned i = e.BitCount();
	if (i==0)
		return Integer::Two();

	MontgomeryRepresentation m(n);
	Integer p=m.ConvertIn(pIn%n), two=m.ConvertIn(Integer::Two());
	Integer v=p, v1=m.Subtract(m.Square(p), two);

	i--;
	while (i--)
	{
		if (e.GetBit(i))
		{
			// v = (v*v1 - p) % m;
			v = m.Subtract(m.Multiply(v,v1), p);
			// v1 = (v1*v1 - 2) % m;
			v1 = m.Subtract(m.Square(v1), two);
		}
		else
		{
			// v1 = (v*v1 - p) % m;
			v1 = m.Subtract(m.Multiply(v,v1), p);
			// v = (v*v - 2) % m;
			v = m.Subtract(m.Square(v), two);
		}
	}
	return m.ConvertOut(v);
}

Integer InverseLucas(const Integer &e, const Integer &m, const Integer &p, const Integer &q, const Integer &u)
{
	Integer d = (m*m-4);
	Integer p2 = p-Jacobi(d,p);
	Integer q2 = q-Jacobi(d,q);
	return CRT(Lucas(EuclideanMultiplicativeInverse(e,p2), m, p), p, Lucas(EuclideanMultiplicativeInverse(e,q2), m, q), q, u);
}

Integer InverseLucas(const Integer &e, const Integer &m, const Integer &p, const Integer &q)
{
	return InverseLucas(e, m, p, q, EuclideanMultiplicativeInverse(p, q));
}

NAMESPACE_END

#endif
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
#ifndef CRYPTOPP_OIDS_H
#define CRYPTOPP_OIDS_H

// crypto-related ASN.1 object identifiers


NAMESPACE_BEGIN(CryptoPP)

NAMESPACE_BEGIN(ASN1)

#define DEFINE_OID(value, name)	inline OID name() {return value;}

DEFINE_OID(1, iso)
	DEFINE_OID(iso()+2, member_body)
		DEFINE_OID(member_body()+840, iso_us)
			DEFINE_OID(iso_us()+10040, ansi_x9_57)
				DEFINE_OID(ansi_x9_57()+4+1, id_dsa)
			DEFINE_OID(iso_us()+10045, ansi_x9_62)
				DEFINE_OID(ansi_x9_62()+1, id_fieldType)
					DEFINE_OID(id_fieldType()+1, prime_field)
					DEFINE_OID(id_fieldType()+2, characteristic_two_field)
						DEFINE_OID(characteristic_two_field()+3, id_characteristic_two_basis)
							DEFINE_OID(id_characteristic_two_basis()+1, gnBasis)
							DEFINE_OID(id_characteristic_two_basis()+2, tpBasis)
							DEFINE_OID(id_characteristic_two_basis()+3, ppBasis)
				DEFINE_OID(ansi_x9_62()+2, id_publicKeyType)
					DEFINE_OID(id_publicKeyType()+1, id_ecPublicKey)
				DEFINE_OID(ansi_x9_62()+3, ansi_x9_62_curves)
					DEFINE_OID(ansi_x9_62_curves()+1, ansi_x9_62_curves_prime)
						DEFINE_OID(ansi_x9_62_curves_prime()+1, secp192r1)
						DEFINE_OID(ansi_x9_62_curves_prime()+7, secp256r1)
			DEFINE_OID(iso_us()+113549, rsadsi)
				DEFINE_OID(rsadsi()+1, pkcs)
					DEFINE_OID(pkcs()+1, pkcs_1)
						DEFINE_OID(pkcs_1()+1, rsaEncryption)
				DEFINE_OID(rsadsi()+2, rsadsi_digestAlgorithm)
					DEFINE_OID(rsadsi_digestAlgorithm()+2, id_md2)
					DEFINE_OID(rsadsi_digestAlgorithm()+5, id_md5)
	DEFINE_OID(iso()+3, identified_organization)
		DEFINE_OID(identified_organization()+14, oiw)
			DEFINE_OID(oiw()+14, oiw_secsig)
				DEFINE_OID(oiw_secsig()+2, oiw_secsig_algorithms)
					DEFINE_OID(oiw_secsig_algorithms()+26, id_sha1)
		DEFINE_OID(identified_organization()+36, teletrust)
			DEFINE_OID(teletrust()+3+2+1, id_ripemd160)
		DEFINE_OID(identified_organization()+132, certicom)
			DEFINE_OID(certicom()+0, certicom_ellipticCurve)
				// these are sorted by curve type and then by OID
				// first curves based on GF(p)
				DEFINE_OID(certicom_ellipticCurve()+6, secp112r1)
				DEFINE_OID(certicom_ellipticCurve()+7, secp112r2)
				DEFINE_OID(certicom_ellipticCurve()+8, secp160r1)
				DEFINE_OID(certicom_ellipticCurve()+9, secp160k1)
				DEFINE_OID(certicom_ellipticCurve()+10, secp256k1)
				DEFINE_OID(certicom_ellipticCurve()+28, secp128r1)
				DEFINE_OID(certicom_ellipticCurve()+29, secp128r2)
				DEFINE_OID(certicom_ellipticCurve()+30, secp160r2)
				DEFINE_OID(certicom_ellipticCurve()+31, secp192k1)
				DEFINE_OID(certicom_ellipticCurve()+32, secp224k1)
				DEFINE_OID(certicom_ellipticCurve()+33, secp224r1)
				DEFINE_OID(certicom_ellipticCurve()+34, secp384r1)
				DEFINE_OID(certicom_ellipticCurve()+35, secp521r1)
				// then curves based on GF(2^n)
				DEFINE_OID(certicom_ellipticCurve()+1, sect163k1)
				DEFINE_OID(certicom_ellipticCurve()+2, sect163r1)
				DEFINE_OID(certicom_ellipticCurve()+3, sect239k1)
				DEFINE_OID(certicom_ellipticCurve()+4, sect113r1)
				DEFINE_OID(certicom_ellipticCurve()+5, sect113r2)
				DEFINE_OID(certicom_ellipticCurve()+15, sect163r2)
				DEFINE_OID(certicom_ellipticCurve()+16, sect283k1)
				DEFINE_OID(certicom_ellipticCurve()+17, sect283r1)
				DEFINE_OID(certicom_ellipticCurve()+22, sect131r1)
				DEFINE_OID(certicom_ellipticCurve()+23, sect131r2)
				DEFINE_OID(certicom_ellipticCurve()+24, sect193r1)
				DEFINE_OID(certicom_ellipticCurve()+25, sect193r2)
				DEFINE_OID(certicom_ellipticCurve()+26, sect233k1)
				DEFINE_OID(certicom_ellipticCurve()+27, sect233r1)
				DEFINE_OID(certicom_ellipticCurve()+36, sect409k1)
				DEFINE_OID(certicom_ellipticCurve()+37, sect409r1)
				DEFINE_OID(certicom_ellipticCurve()+38, sect571k1)
				DEFINE_OID(certicom_ellipticCurve()+39, sect571r1)
DEFINE_OID(2, joint_iso_ccitt)
	DEFINE_OID(joint_iso_ccitt()+16, country)
		DEFINE_OID(country()+840, joint_iso_ccitt_us)
			DEFINE_OID(joint_iso_ccitt_us()+1, us_organization)
				DEFINE_OID(us_organization()+101, us_gov)
					DEFINE_OID(us_gov()+3, csor)
						DEFINE_OID(csor()+4, nistalgorithms)
							DEFINE_OID(nistalgorithms()+1, aes)
								DEFINE_OID(aes()+1, id_aes128_ECB)
								DEFINE_OID(aes()+2, id_aes128_cbc)
								DEFINE_OID(aes()+3, id_aes128_ofb)
								DEFINE_OID(aes()+4, id_aes128_cfb)
								DEFINE_OID(aes()+21, id_aes192_ECB)
								DEFINE_OID(aes()+22, id_aes192_cbc)
								DEFINE_OID(aes()+23, id_aes192_ofb)
								DEFINE_OID(aes()+24, id_aes192_cfb)
								DEFINE_OID(aes()+41, id_aes256_ECB)
								DEFINE_OID(aes()+42, id_aes256_cbc)
								DEFINE_OID(aes()+43, id_aes256_ofb)
								DEFINE_OID(aes()+44, id_aes256_cfb)
							DEFINE_OID(nistalgorithms()+2, nist_hashalgs)
								DEFINE_OID(nist_hashalgs()+1, id_sha256)
								DEFINE_OID(nist_hashalgs()+2, id_sha384)
								DEFINE_OID(nist_hashalgs()+3, id_sha512)

NAMESPACE_END

NAMESPACE_END

#endif
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
// integer.cpp - written and placed in the public domain by Wei Dai
// contains public domain code contributed by Alister Lee and Leonard Janke


#ifndef CRYPTOPP_IMPORTS



#ifdef SSE2_INTRINSICS_AVAILABLE
	#ifdef __GNUC__
		#include <xmmintrin.h> // Do_not_auto_remove
		#include <signal.h> // Do_not_auto_remove
		#include <setjmp.h> // Do_not_auto_remove
		#ifdef CRYPTOPP_MEMALIGN_AVAILABLE
			#include <malloc.h> // Do_not_auto_remove
		#else
			#include <stdlib.h> // Do_not_auto_remove
		#endif
	#else
		#include <emmintrin.h> // Do_not_auto_remove
	#endif
#elif defined(_MSC_VER) && defined(_M_IX86)
	#pragma message("You do not seem to have the Visual C++ Processor Pack installed, so use of SSE2 intrinsics will be disabled.")
#elif defined(__GNUC__) && defined(__i386__)
	#warning "You do not have GCC 3.3 or later, or did not specify -msse2 compiler option, so use of SSE2 intrinsics will be disabled."
#endif

NAMESPACE_BEGIN(CryptoPP)

bool FunctionAssignIntToInteger(const std::type_info &valueType, void *pInteger, const void *pInt)
{
	if (valueType != typeid(Integer))
		return false;
	*reinterpret_cast<Integer *>(pInteger) = *reinterpret_cast<const int *>(pInt);
	return true;
}

static const char s_RunAtStartup = (AssignIntToInteger = FunctionAssignIntToInteger, 0);

#ifdef SSE2_INTRINSICS_AVAILABLE
template <class T>
CPP_TYPENAME AllocatorBase<T>::pointer AlignedAllocator<T>::allocate(size_type n, const void *)
{
	CheckSize(n);
	if (n == 0)
		return NULL;
	if (n >= 4)
	{
		void *p;
	#ifdef CRYPTOPP_MM_MALLOC_AVAILABLE
		while (!(p = _mm_malloc(sizeof(T)*n, 16)))
	#elif defined(CRYPTOPP_MEMALIGN_AVAILABLE)
		while (!(p = memalign(16, sizeof(T)*n)))
	#elif defined(CRYPTOPP_MALLOC_ALIGNMENT_IS_16)
		while (!(p = malloc(sizeof(T)*n)))
	#else
		while (!(p = (byte *)malloc(sizeof(T)*n + 8)))	// assume malloc alignment is at least 8
	#endif
			CallNewHandler();

	#ifdef CRYPTOPP_NO_ALIGNED_ALLOC
		assert(m_pBlock == NULL);
		m_pBlock = p;
		if (!IsAlignedOn(p, 16))
		{
			assert(IsAlignedOn(p, 8));
			p = (byte *)p + 8;
		}
	#endif

		assert(IsAlignedOn(p, 16));
		return (T*)p;
	}
	return new T[n];
}

template <class T>
void AlignedAllocator<T>::deallocate(void *p, size_type n)
{
	memset(p, 0, n*sizeof(T));
	if (n >= 4)
	{
		#ifdef CRYPTOPP_MM_MALLOC_AVAILABLE
			_mm_free(p);
		#elif defined(CRYPTOPP_NO_ALIGNED_ALLOC)
			assert(m_pBlock == p || (byte *)m_pBlock+8 == p);
			free(m_pBlock);
			m_pBlock = NULL;
		#else
			free(p);
		#endif
	}
	else
		delete [] (T *)p;
}
#endif

static int Compare(const word *A, const word *B, unsigned int N)
{
	while (N--)
		if (A[N] > B[N])
			return 1;
		else if (A[N] < B[N])
			return -1;

	return 0;
}

static word Increment(word *A, unsigned int N, word B=1)
{
	assert(N);
	word t = A[0];
	A[0] = t+B;
	if (A[0] >= t)
		return 0;
	for (unsigned i=1; i<N; i++)
		if (++A[i])
			return 0;
	return 1;
}

static word Decrement(word *A, unsigned int N, word B=1)
{
	assert(N);
	word t = A[0];
	A[0] = t-B;
	if (A[0] <= t)
		return 0;
	for (unsigned i=1; i<N; i++)
		if (A[i]--)
			return 0;
	return 1;
}

static void TwosComplement(word *A, unsigned int N)
{
	Decrement(A, N);
	for (unsigned i=0; i<N; i++)
		A[i] = ~A[i];
}

static word AtomicInverseModPower2(word A)
{
	assert(A%2==1);

	word R=A%8;

	for (unsigned i=3; i<WORD_BITS; i*=2)
		R = R*(2-R*A);

	assert(R*A==1);
	return R;
}

// ********************************************************

class DWord
{
public:
	DWord() {}

#ifdef CRYPTOPP_NATIVE_DWORD_AVAILABLE
	explicit DWord(word low)
	{
		m_whole = low;
	}
#else
	explicit DWord(word low)
	{
		m_halfs.low = low;
		m_halfs.high = 0;
	}
#endif

	DWord(word low, word high)
	{
		m_halfs.low = low;
		m_halfs.high = high;
	}

	static DWord Multiply(word a, word b)
	{
		DWord r;
		#ifdef CRYPTOPP_NATIVE_DWORD_AVAILABLE
			r.m_whole = (dword)a * b;
		#elif defined(__alpha__)
			r.m_halfs.low = a*b; __asm__("umulh %1,%2,%0" : "=r" (r.m_halfs.high) : "r" (a), "r" (b));
		#elif defined(__ia64__)
			r.m_halfs.low = a*b; __asm__("xmpy.hu %0=%1,%2" : "=f" (r.m_halfs.high) : "f" (a), "f" (b));
		#elif defined(_ARCH_PPC64)
			r.m_halfs.low = a*b; __asm__("mulhdu %0,%1,%2" : "=r" (r.m_halfs.high) : "r" (a), "r" (b) : "cc");
		#elif defined(__x86_64__)
			__asm__("mulq %3" : "=d" (r.m_halfs.high), "=a" (r.m_halfs.low) : "a" (a), "rm" (b) : "cc");
		#elif defined(__mips64)
			__asm__("dmultu %2,%3" : "=h" (r.m_halfs.high), "=l" (r.m_halfs.low) : "r" (a), "r" (b));
		#elif defined(_M_IX86)
			// for testing
			word64 t = (word64)a * b;
			r.m_halfs.high = ((word32 *)(&t))[1];
			r.m_halfs.low = (word32)t;
		#else
			#error can not implement DWord
		#endif
		return r;
	}

	static DWord MultiplyAndAdd(word a, word b, word c)
	{
		DWord r = Multiply(a, b);
		return r += c;
	}

	DWord & operator+=(word a)
	{
		#ifdef CRYPTOPP_NATIVE_DWORD_AVAILABLE
			m_whole = m_whole + a;
		#else
			m_halfs.low += a;
			m_halfs.high += (m_halfs.low < a);
		#endif
		return *this;
	}

	DWord operator+(word a)
	{
		DWord r;
		#ifdef CRYPTOPP_NATIVE_DWORD_AVAILABLE
			r.m_whole = m_whole + a;
		#else
			r.m_halfs.low = m_halfs.low + a;
			r.m_halfs.high = m_halfs.high + (r.m_halfs.low < a);
		#endif
		return r;
	}

	DWord operator-(DWord a)
	{
		DWord r;
		#ifdef CRYPTOPP_NATIVE_DWORD_AVAILABLE
			r.m_whole = m_whole - a.m_whole;
		#else
			r.m_halfs.low = m_halfs.low - a.m_halfs.low;
			r.m_halfs.high = m_halfs.high - a.m_halfs.high - (r.m_halfs.low > m_halfs.low);
		#endif
		return r;
	}

	DWord operator-(word a)
	{
		DWord r;
		#ifdef CRYPTOPP_NATIVE_DWORD_AVAILABLE
			r.m_whole = m_whole - a;
		#else
			r.m_halfs.low = m_halfs.low - a;
			r.m_halfs.high = m_halfs.high - (r.m_halfs.low > m_halfs.low);
		#endif
		return r;
	}

	// returns quotient, which must fit in a word
	word operator/(word divisor);

	word operator%(word a);

	bool operator!() const
	{
	#ifdef CRYPTOPP_NATIVE_DWORD_AVAILABLE
		return !m_whole;
	#else
		return !m_halfs.high && !m_halfs.low;
	#endif
	}

	word GetLowHalf() const {return m_halfs.low;}
	word GetHighHalf() const {return m_halfs.high;}
	word GetHighHalfAsBorrow() const {return 0-m_halfs.high;}

private:
	union
	{
	#ifdef CRYPTOPP_NATIVE_DWORD_AVAILABLE
		dword m_whole;
	#endif
		struct
		{
		#ifdef IS_LITTLE_ENDIAN
			word low;
			word high;
		#else
			word high;
			word low;
		#endif
		} m_halfs;
	};
};

class Word
{
public:
	Word() {}

	Word(word value)
	{
		m_whole = value;
	}

	Word(hword low, hword high)
	{
		m_whole = low | (word(high) << (WORD_BITS/2));
	}

	static Word Multiply(hword a, hword b)
	{
		Word r;
		r.m_whole = (word)a * b;
		return r;
	}

	Word operator-(Word a)
	{
		Word r;
		r.m_whole = m_whole - a.m_whole;
		return r;
	}

	Word operator-(hword a)
	{
		Word r;
		r.m_whole = m_whole - a;
		return r;
	}

	// returns quotient, which must fit in a word
	hword operator/(hword divisor)
	{
		return hword(m_whole / divisor);
	}

	bool operator!() const
	{
		return !m_whole;
	}

	word GetWhole() const {return m_whole;}
	hword GetLowHalf() const {return hword(m_whole);}
	hword GetHighHalf() const {return hword(m_whole>>(WORD_BITS/2));}
	hword GetHighHalfAsBorrow() const {return 0-hword(m_whole>>(WORD_BITS/2));}
	
private:
	word m_whole;
};

// do a 3 word by 2 word divide, returns quotient and leaves remainder in A
template <class S, class D>
S DivideThreeWordsByTwo(S *A, S B0, S B1, D* /* dummy */ = NULL)
{
	// assert {A[2],A[1]} < {B1,B0}, so quotient can fit in a S
	assert(A[2] < B1 || (A[2]==B1 && A[1] < B0));

	// estimate the quotient: do a 2 S by 1 S divide
	S Q;
	if (S(B1+1) == 0)
		Q = A[2];
	else
		Q = D(A[1], A[2]) / S(B1+1);

	// now subtract Q*B from A
	D p = D::Multiply(B0, Q);
	D u = (D) A[0] - p.GetLowHalf();
	A[0] = u.GetLowHalf();
	u = (D) A[1] - p.GetHighHalf() - u.GetHighHalfAsBorrow() - D::Multiply(B1, Q);
	A[1] = u.GetLowHalf();
	A[2] += u.GetHighHalf();

	// Q <= actual quotient, so fix it
	while (A[2] || A[1] > B1 || (A[1]==B1 && A[0]>=B0))
	{
		u = (D) A[0] - B0;
		A[0] = u.GetLowHalf();
		u = (D) A[1] - B1 - u.GetHighHalfAsBorrow();
		A[1] = u.GetLowHalf();
		A[2] += u.GetHighHalf();
		Q++;
		assert(Q);	// shouldn't overflow
	}

	return Q;
}

// do a 4 word by 2 word divide, returns 2 word quotient in Q0 and Q1
template <class S, class D>
inline D DivideFourWordsByTwo(S *T, const D &Al, const D &Ah, const D &B)
{
	if (!B) // if divisor is 0, we assume divisor==2**(2*WORD_BITS)
		return D(Ah.GetLowHalf(), Ah.GetHighHalf());
	else
	{
		S Q[2];
		T[0] = Al.GetLowHalf();
		T[1] = Al.GetHighHalf(); 
		T[2] = Ah.GetLowHalf();
		T[3] = Ah.GetHighHalf();
		Q[1] = DivideThreeWordsByTwo<S, D>(T+1, B.GetLowHalf(), B.GetHighHalf());
		Q[0] = DivideThreeWordsByTwo<S, D>(T, B.GetLowHalf(), B.GetHighHalf());
		return D(Q[0], Q[1]);
	}
}

// returns quotient, which must fit in a word
inline word DWord::operator/(word a)
{
	#ifdef CRYPTOPP_NATIVE_DWORD_AVAILABLE
		return word(m_whole / a);
	#else
		hword r[4];
		return DivideFourWordsByTwo<hword, Word>(r, m_halfs.low, m_halfs.high, a).GetWhole();
	#endif
}

inline word DWord::operator%(word a)
{
	#ifdef CRYPTOPP_NATIVE_DWORD_AVAILABLE
		return word(m_whole % a);
	#else
		if (a < (word(1) << (WORD_BITS/2)))
		{
			hword h = hword(a);
			word r = m_halfs.high % h;
			r = ((m_halfs.low >> (WORD_BITS/2)) + (r << (WORD_BITS/2))) % h;
			return hword((hword(m_halfs.low) + (r << (WORD_BITS/2))) % h);
		}
		else
		{
			hword r[4];
			DivideFourWordsByTwo<hword, Word>(r, m_halfs.low, m_halfs.high, a);
			return Word(r[0], r[1]).GetWhole();
		}
	#endif
}

// ********************************************************

class Portable
{
public:
	static word Add(word *C, const word *A, const word *B, unsigned int N);
	static word Subtract(word *C, const word *A, const word *B, unsigned int N);

	static inline void Multiply2(word *C, const word *A, const word *B);
	static inline word Multiply2Add(word *C, const word *A, const word *B);
	static void Multiply4(word *C, const word *A, const word *B);
	static void Multiply8(word *C, const word *A, const word *B);
	static inline unsigned int MultiplyRecursionLimit() {return 8;}

	static inline void Multiply2Bottom(word *C, const word *A, const word *B);
	static void Multiply4Bottom(word *C, const word *A, const word *B);
	static void Multiply8Bottom(word *C, const word *A, const word *B);
	static inline unsigned int MultiplyBottomRecursionLimit() {return 8;}

	static void Square2(word *R, const word *A);
	static void Square4(word *R, const word *A);
	static void Square8(word* /* R */, const word* /* A */) {assert(false);}
	static inline unsigned int SquareRecursionLimit() {return 4;}
};

word Portable::Add(word *C, const word *A, const word *B, unsigned int N)
{
	assert (N%2 == 0);

	DWord u(0, 0);
	for (unsigned int i = 0; i < N; i+=2)
	{
		u = DWord(A[i]) + B[i] + u.GetHighHalf();
		C[i] = u.GetLowHalf();
		u = DWord(A[i+1]) + B[i+1] + u.GetHighHalf();
		C[i+1] = u.GetLowHalf();
	}
	return u.GetHighHalf();
}

word Portable::Subtract(word *C, const word *A, const word *B, unsigned int N)
{
	assert (N%2 == 0);

	DWord u(0, 0);
	for (unsigned int i = 0; i < N; i+=2)
	{
		u = (DWord) A[i] - B[i] - u.GetHighHalfAsBorrow();
		C[i] = u.GetLowHalf();
		u = (DWord) A[i+1] - B[i+1] - u.GetHighHalfAsBorrow();
		C[i+1] = u.GetLowHalf();
	}
	return 0-u.GetHighHalf();
}

void Portable::Multiply2(word *C, const word *A, const word *B)
{
	// this segment is the branchless equivalent of above
	word D[4] = {A[1]-A[0], A[0]-A[1], B[0]-B[1], B[1]-B[0]};
	unsigned int ai = A[1] < A[0];
	unsigned int bi = B[0] < B[1];
	unsigned int di = ai & bi;
	DWord d = DWord::Multiply(D[di], D[di+2]);
	D[1] = D[3] = 0;
	unsigned int si = ai + !bi;
	word s = D[si];

	DWord A0B0 = DWord::Multiply(A[0], B[0]);
	C[0] = A0B0.GetLowHalf();

	DWord A1B1 = DWord::Multiply(A[1], B[1]);
	DWord t = (DWord) A0B0.GetHighHalf() + A0B0.GetLowHalf() + d.GetLowHalf() + A1B1.GetLowHalf();
	C[1] = t.GetLowHalf();

	t = A1B1 + t.GetHighHalf() + A0B0.GetHighHalf() + d.GetHighHalf() + A1B1.GetHighHalf() - s;
	C[2] = t.GetLowHalf();
	C[3] = t.GetHighHalf();
}

inline void Portable::Multiply2Bottom(word *C, const word *A, const word *B)
{
	DWord t = DWord::Multiply(A[0], B[0]);
	C[0] = t.GetLowHalf();
	C[1] = t.GetHighHalf() + A[0]*B[1] + A[1]*B[0];
}

word Portable::Multiply2Add(word *C, const word *A, const word *B)
{
	word D[4] = {A[1]-A[0], A[0]-A[1], B[0]-B[1], B[1]-B[0]};
	unsigned int ai = A[1] < A[0];
	unsigned int bi = B[0] < B[1];
	unsigned int di = ai & bi;
	DWord d = DWord::Multiply(D[di], D[di+2]);
	D[1] = D[3] = 0;
	unsigned int si = ai + !bi;
	word s = D[si];

	DWord A0B0 = DWord::Multiply(A[0], B[0]);
	DWord t = A0B0 + C[0];
	C[0] = t.GetLowHalf();

	DWord A1B1 = DWord::Multiply(A[1], B[1]);
	t = (DWord) t.GetHighHalf() + A0B0.GetLowHalf() + d.GetLowHalf() + A1B1.GetLowHalf() + C[1];
	C[1] = t.GetLowHalf();

	t = (DWord) t.GetHighHalf() + A1B1.GetLowHalf() + A0B0.GetHighHalf() + d.GetHighHalf() + A1B1.GetHighHalf() - s + C[2];
	C[2] = t.GetLowHalf();

	t = (DWord) t.GetHighHalf() + A1B1.GetHighHalf() + C[3];
	C[3] = t.GetLowHalf();
	return t.GetHighHalf();
}

#define MulAcc(x, y)								\
	p = DWord::MultiplyAndAdd(A[x], B[y], c);		\
	c = p.GetLowHalf();								\
	p = (DWord) d + p.GetHighHalf();					\
	d = p.GetLowHalf();								\
	e += p.GetHighHalf();

#define SaveMulAcc(s, x, y) 						\
	R[s] = c;										\
	p = DWord::MultiplyAndAdd(A[x], B[y], d);				\
	c = p.GetLowHalf();								\
	p = (DWord) e + p.GetHighHalf();					\
	d = p.GetLowHalf();								\
	e = p.GetHighHalf();

#define SquAcc(x, y)								\
	q = DWord::Multiply(A[x], A[y]);	\
	p = q + c; 					\
	c = p.GetLowHalf();								\
	p = (DWord) d + p.GetHighHalf();					\
	d = p.GetLowHalf();								\
	e += p.GetHighHalf();			\
	p = q + c; 					\
	c = p.GetLowHalf();								\
	p = (DWord) d + p.GetHighHalf();					\
	d = p.GetLowHalf();								\
	e += p.GetHighHalf();

#define SaveSquAcc(s, x, y) 						\
	R[s] = c;										\
	q = DWord::Multiply(A[x], A[y]);	\
	p = q + d; 					\
	c = p.GetLowHalf();								\
	p = (DWord) e + p.GetHighHalf();					\
	d = p.GetLowHalf();								\
	e = p.GetHighHalf();			\
	p = q + c; 					\
	c = p.GetLowHalf();								\
	p = (DWord) d + p.GetHighHalf();					\
	d = p.GetLowHalf();								\
	e += p.GetHighHalf();

void Portable::Multiply4(word *R, const word *A, const word *B)
{
	DWord p;
	word c, d, e;

	p = DWord::Multiply(A[0], B[0]);
	R[0] = p.GetLowHalf();
	c = p.GetHighHalf();
	d = e = 0;

	MulAcc(0, 1);
	MulAcc(1, 0);

	SaveMulAcc(1, 2, 0);
	MulAcc(1, 1);
	MulAcc(0, 2);

	SaveMulAcc(2, 0, 3);
	MulAcc(1, 2);
	MulAcc(2, 1);
	MulAcc(3, 0);

	SaveMulAcc(3, 3, 1);
	MulAcc(2, 2);
	MulAcc(1, 3);

	SaveMulAcc(4, 2, 3);
	MulAcc(3, 2);

	R[5] = c;
	p = DWord::MultiplyAndAdd(A[3], B[3], d);
	R[6] = p.GetLowHalf();
	R[7] = e + p.GetHighHalf();
}

void Portable::Square2(word *R, const word *A)
{
	DWord p, q;
	word c, d, e;

	p = DWord::Multiply(A[0], A[0]);
	R[0] = p.GetLowHalf();
	c = p.GetHighHalf();
	d = e = 0;

	SquAcc(0, 1);

	R[1] = c;
	p = DWord::MultiplyAndAdd(A[1], A[1], d);
	R[2] = p.GetLowHalf();
	R[3] = e + p.GetHighHalf();
}

void Portable::Square4(word *R, const word *A)
{
#ifdef _MSC_VER
	// VC60 workaround: MSVC 6.0 has an optimization bug that makes
	// (dword)A*B where either A or B has been cast to a dword before
	// very expensive. Revisit this function when this
	// bug is fixed.
	Multiply4(R, A, A);
#else
	const word *B = A;
	DWord p, q;
	word c, d, e;

	p = DWord::Multiply(A[0], A[0]);
	R[0] = p.GetLowHalf();
	c = p.GetHighHalf();
	d = e = 0;

	SquAcc(0, 1);

	SaveSquAcc(1, 2, 0);
	MulAcc(1, 1);

	SaveSquAcc(2, 0, 3);
	SquAcc(1, 2);

	SaveSquAcc(3, 3, 1);
	MulAcc(2, 2);

	SaveSquAcc(4, 2, 3);

	R[5] = c;
	p = DWord::MultiplyAndAdd(A[3], A[3], d);
	R[6] = p.GetLowHalf();
	R[7] = e + p.GetHighHalf();
#endif
}

void Portable::Multiply8(word *R, const word *A, const word *B)
{
	DWord p;
	word c, d, e;

	p = DWord::Multiply(A[0], B[0]);
	R[0] = p.GetLowHalf();
	c = p.GetHighHalf();
	d = e = 0;

	MulAcc(0, 1);
	MulAcc(1, 0);

	SaveMulAcc(1, 2, 0);
	MulAcc(1, 1);
	MulAcc(0, 2);

	SaveMulAcc(2, 0, 3);
	MulAcc(1, 2);
	MulAcc(2, 1);
	MulAcc(3, 0);

	SaveMulAcc(3, 0, 4);
	MulAcc(1, 3);
	MulAcc(2, 2);
	MulAcc(3, 1);
	MulAcc(4, 0);

	SaveMulAcc(4, 0, 5);
	MulAcc(1, 4);
	MulAcc(2, 3);
	MulAcc(3, 2);
	MulAcc(4, 1);
	MulAcc(5, 0);

	SaveMulAcc(5, 0, 6);
	MulAcc(1, 5);
	MulAcc(2, 4);
	MulAcc(3, 3);
	MulAcc(4, 2);
	MulAcc(5, 1);
	MulAcc(6, 0);

	SaveMulAcc(6, 0, 7);
	MulAcc(1, 6);
	MulAcc(2, 5);
	MulAcc(3, 4);
	MulAcc(4, 3);
	MulAcc(5, 2);
	MulAcc(6, 1);
	MulAcc(7, 0);

	SaveMulAcc(7, 1, 7);
	MulAcc(2, 6);
	MulAcc(3, 5);
	MulAcc(4, 4);
	MulAcc(5, 3);
	MulAcc(6, 2);
	MulAcc(7, 1);

	SaveMulAcc(8, 2, 7);
	MulAcc(3, 6);
	MulAcc(4, 5);
	MulAcc(5, 4);
	MulAcc(6, 3);
	MulAcc(7, 2);

	SaveMulAcc(9, 3, 7);
	MulAcc(4, 6);
	MulAcc(5, 5);
	MulAcc(6, 4);
	MulAcc(7, 3);

	SaveMulAcc(10, 4, 7);
	MulAcc(5, 6);
	MulAcc(6, 5);
	MulAcc(7, 4);

	SaveMulAcc(11, 5, 7);
	MulAcc(6, 6);
	MulAcc(7, 5);

	SaveMulAcc(12, 6, 7);
	MulAcc(7, 6);

	R[13] = c;
	p = DWord::MultiplyAndAdd(A[7], B[7], d);
	R[14] = p.GetLowHalf();
	R[15] = e + p.GetHighHalf();
}

void Portable::Multiply4Bottom(word *R, const word *A, const word *B)
{
	DWord p;
	word c, d, e;

	p = DWord::Multiply(A[0], B[0]);
	R[0] = p.GetLowHalf();
	c = p.GetHighHalf();
	d = e = 0;

	MulAcc(0, 1);
	MulAcc(1, 0);

	SaveMulAcc(1, 2, 0);
	MulAcc(1, 1);
	MulAcc(0, 2);

	R[2] = c;
	R[3] = d + A[0] * B[3] + A[1] * B[2] + A[2] * B[1] + A[3] * B[0];
}

void Portable::Multiply8Bottom(word *R, const word *A, const word *B)
{
	DWord p;
	word c, d, e;

	p = DWord::Multiply(A[0], B[0]);
	R[0] = p.GetLowHalf();
	c = p.GetHighHalf();
	d = e = 0;

	MulAcc(0, 1);
	MulAcc(1, 0);

	SaveMulAcc(1, 2, 0);
	MulAcc(1, 1);
	MulAcc(0, 2);

	SaveMulAcc(2, 0, 3);
	MulAcc(1, 2);
	MulAcc(2, 1);
	MulAcc(3, 0);

	SaveMulAcc(3, 0, 4);
	MulAcc(1, 3);
	MulAcc(2, 2);
	MulAcc(3, 1);
	MulAcc(4, 0);

	SaveMulAcc(4, 0, 5);
	MulAcc(1, 4);
	MulAcc(2, 3);
	MulAcc(3, 2);
	MulAcc(4, 1);
	MulAcc(5, 0);

	SaveMulAcc(5, 0, 6);
	MulAcc(1, 5);
	MulAcc(2, 4);
	MulAcc(3, 3);
	MulAcc(4, 2);
	MulAcc(5, 1);
	MulAcc(6, 0);

	R[6] = c;
	R[7] = d + A[0] * B[7] + A[1] * B[6] + A[2] * B[5] + A[3] * B[4] +
				A[4] * B[3] + A[5] * B[2] + A[6] * B[1] + A[7] * B[0];
}

#undef MulAcc
#undef SaveMulAcc
#undef SquAcc
#undef SaveSquAcc

#ifdef CRYPTOPP_X86ASM_AVAILABLE

// ************** x86 feature detection ***************

static void CpuId(word32 input, word32 *output)
{
#ifdef __GNUC__
	__asm__
	(
		// save ebx in case -fPIC is being used
		"push %%ebx; cpuid; mov %%ebx, %%edi; pop %%ebx"
		: "=a" (output[0]), "=D" (output[1]), "=c" (output[2]), "=d" (output[3])
		: "a" (input)
	);
#else
	__asm
	{
		mov eax, input
		cpuid
		mov edi, output
		mov [edi], eax
		mov [edi+4], ebx
		mov [edi+8], ecx
		mov [edi+12], edx
	}
#endif
}

static bool IsP4()
{
	word32 cpuid[4];

	CpuId(0, cpuid);
	std::swap(cpuid[2], cpuid[3]);
	if (memcmp(cpuid+1, "GenuineIntel", 12) != 0)
		return false;

	CpuId(1, cpuid);
	return ((cpuid[0] >> 8) & 0xf) == 0xf;

}


// ************** Pentium/P4 optimizations ***************

class PentiumOptimized : public Portable
{
public:
	static word CRYPTOPP_CDECL Add(word *C, const word *A, const word *B, unsigned int N);
	static word CRYPTOPP_CDECL Subtract(word *C, const word *A, const word *B, unsigned int N);
	static void CRYPTOPP_CDECL Multiply4(word *C, const word *A, const word *B);
	static void CRYPTOPP_CDECL Multiply8(word *C, const word *A, const word *B);
	static void CRYPTOPP_CDECL Multiply8Bottom(word *C, const word *A, const word *B);
};

class P4Optimized
{
public:
	static word CRYPTOPP_CDECL Add(word *C, const word *A, const word *B, unsigned int N);
	static word CRYPTOPP_CDECL Subtract(word *C, const word *A, const word *B, unsigned int N);
};

typedef word (CRYPTOPP_CDECL * PAddSub)(word *C, const word *A, const word *B, unsigned int N);
typedef void (CRYPTOPP_CDECL * PMul)(word *C, const word *A, const word *B);

static PAddSub s_pAdd, s_pSub;
#ifdef SSE2_INTRINSICS_AVAILABLE
static PMul s_pMul4, s_pMul8, s_pMul8B;
#endif

static void SetPentiumFunctionPointers()
{
	if (IsP4())
	{
		s_pAdd = &P4Optimized::Add;
		s_pSub = &P4Optimized::Subtract;
	}
	else
	{
		s_pAdd = &PentiumOptimized::Add;
		s_pSub = &PentiumOptimized::Subtract;
	}

#ifdef SSE2_INTRINSICS_AVAILABLE
	s_pMul4 = &PentiumOptimized::Multiply4;
	s_pMul8 = &PentiumOptimized::Multiply8;
	s_pMul8B = &PentiumOptimized::Multiply8Bottom;
#endif
}

static const char s_RunAtStartupSetPentiumFunctionPointers = (SetPentiumFunctionPointers(), 0);

class LowLevel : public PentiumOptimized
{
public:
	inline static word Add(word *C, const word *A, const word *B, unsigned int N)
		{return s_pAdd(C, A, B, N);}
	inline static word Subtract(word *C, const word *A, const word *B, unsigned int N)
		{return s_pSub(C, A, B, N);}
	inline static void Square4(word *R, const word *A)
		{Multiply4(R, A, A);}
#ifdef SSE2_INTRINSICS_AVAILABLE
	inline static void Multiply4(word *C, const word *A, const word *B)
		{s_pMul4(C, A, B);}
	inline static void Multiply8(word *C, const word *A, const word *B)
		{s_pMul8(C, A, B);}
	inline static void Multiply8Bottom(word *C, const word *A, const word *B)
		{s_pMul8B(C, A, B);}
#endif
};

// use some tricks to share assembly code between MSVC and GCC
#ifdef _MSC_VER
	#define CRYPTOPP_NAKED __declspec(naked)
	#define AS1(x) __asm x
	#define AS2(x, y) __asm x, y
	#define AddPrologue \
		__asm	push ebp \
		__asm	push ebx \
		__asm	push esi \
		__asm	push edi \
		__asm	mov		ecx, [esp+20] \
		__asm	mov		edx, [esp+24] \
		__asm	mov		ebx, [esp+28] \
		__asm	mov		esi, [esp+32]
	#define AddEpilogue \
		__asm	pop edi \
		__asm	pop esi \
		__asm	pop ebx \
		__asm	pop ebp \
		__asm	ret
	#define MulPrologue \
		__asm	push ebp \
		__asm	push ebx \
		__asm	push esi \
		__asm	push edi \
		__asm	mov ecx, [esp+28] \
		__asm	mov esi, [esp+24] \
		__asm	push [esp+20]
	#define MulEpilogue \
		__asm	add esp, 4 \
		__asm	pop edi \
		__asm	pop esi \
		__asm	pop ebx \
		__asm	pop ebp \
		__asm	ret
#else
	#define CRYPTOPP_NAKED
	#define AS1(x) #x ";"
	#define AS2(x, y) #x ", " #y ";"
	#define AddPrologue \
		__asm__ __volatile__ \
		( \
			"push %%ebx;"	/* save this manually, in case of -fPIC */ \
			"mov %2, %%ebx;" \
			".intel_syntax noprefix;" \
			"push ebp;"
	#define AddEpilogue \
			"pop ebp;" \
			".att_syntax prefix;" \
			"pop %%ebx;" \
		  			: \
					: "c" (C), "d" (A), "m" (B), "S" (N) \
					: "%edi", "memory", "cc" \
		); 
	#define MulPrologue \
		__asm__ __volatile__ \
		( \
			"push %%ebx;"	/* save this manually, in case of -fPIC */ \
			"push %%ebp;" \
			"push %0;" \
			".intel_syntax noprefix;"
	#define MulEpilogue \
			"add esp, 4;" \
			"pop ebp;" \
			"pop ebx;" \
			".att_syntax prefix;" \
			: \
			: "rm" (Z), "S" (X), "c" (Y) \
			: "%eax", "%edx", "%edi", "memory", "cc" \
		);
#endif

CRYPTOPP_NAKED word PentiumOptimized::Add(word *C, const word *A, const word *B, unsigned int N)
{
	AddPrologue

	// now: ebx = B, ecx = C, edx = A, esi = N
	AS2(	sub ecx, edx)	// hold the distance between C & A so we can add this to A to get C
	AS2(	xor eax, eax)	// clear eax

	AS2(	sub eax, esi)	// eax is a negative index from end of B
	AS2(	lea ebx, [ebx+4*esi])	// ebx is end of B

	AS2(	sar eax, 1)		// unit of eax is now dwords; this also clears the carry flag
	AS1(	jz	loopendAdd)		// if no dwords then nothing to do

	AS1(loopstartAdd:)
	AS2(	mov    esi,[edx])			// load lower word of A
	AS2(	mov    ebp,[edx+4])			// load higher word of A

	AS2(	mov    edi,[ebx+8*eax])		// load lower word of B
	AS2(	lea    edx,[edx+8])			// advance A and C

	AS2(	adc    esi,edi)				// add lower words
	AS2(	mov    edi,[ebx+8*eax+4])	// load higher word of B

	AS2(	adc    ebp,edi)				// add higher words
	AS1(	inc    eax)					// advance B

	AS2(	mov    [edx+ecx-8],esi)		// store lower word result
	AS2(	mov    [edx+ecx-4],ebp)		// store higher word result

	AS1(	jnz    loopstartAdd)			// loop until eax overflows and becomes zero

	AS1(loopendAdd:)
	AS2(	adc eax, 0)		// store carry into eax (return result register)

	AddEpilogue

	// Just to get rid of warnings
	// return 0;
}

CRYPTOPP_NAKED word PentiumOptimized::Subtract(word *C, const word *A, const word *B, unsigned int N)
{
	AddPrologue

	// now: ebx = B, ecx = C, edx = A, esi = N
	AS2(	sub ecx, edx)	// hold the distance between C & A so we can add this to A to get C
	AS2(	xor eax, eax)	// clear eax

	AS2(	sub eax, esi)	// eax is a negative index from end of B
	AS2(	lea ebx, [ebx+4*esi])	// ebx is end of B

	AS2(	sar eax, 1)		// unit of eax is now dwords; this also clears the carry flag
	AS1(	jz	loopendSub)		// if no dwords then nothing to do

	AS1(loopstartSub:)
	AS2(	mov    esi,[edx])			// load lower word of A
	AS2(	mov    ebp,[edx+4])			// load higher word of A

	AS2(	mov    edi,[ebx+8*eax])		// load lower word of B
	AS2(	lea    edx,[edx+8])			// advance A and C

	AS2(	sbb    esi,edi)				// subtract lower words
	AS2(	mov    edi,[ebx+8*eax+4])	// load higher word of B

	AS2(	sbb    ebp,edi)				// subtract higher words
	AS1(	inc    eax)					// advance B

	AS2(	mov    [edx+ecx-8],esi)		// store lower word result
	AS2(	mov    [edx+ecx-4],ebp)		// store higher word result

	AS1(	jnz    loopstartSub)			// loop until eax overflows and becomes zero

	AS1(loopendSub:)
	AS2(	adc eax, 0)		// store carry into eax (return result register)

	AddEpilogue

	// Just to get rid of warnings
	// return 0;
}

// On Pentium 4, the adc and sbb instructions are very expensive, so avoid them.

CRYPTOPP_NAKED word P4Optimized::Add(word *C, const word *A, const word *B, unsigned int N)
{
	AddPrologue

	// now: ebx = B, ecx = C, edx = A, esi = N
	AS2(	xor		eax, eax)
	AS1(	neg		esi)
	AS1(	jz		loopendAddP4)		// if no dwords then nothing to do

	AS2(	mov		edi, [edx])
	AS2(	mov		ebp, [ebx])
	AS1(	jmp		carry1AddP4)

	AS1(loopstartAddP4:)
	AS2(	mov		edi, [edx+8])
	AS2(	add		ecx, 8)
	AS2(	add		edx, 8)
	AS2(	mov		ebp, [ebx])
	AS2(	add		edi, eax)
	AS1(	jc		carry1AddP4)
	AS2(	xor		eax, eax)

	AS1(carry1AddP4:)
	AS2(	add		edi, ebp)
	AS2(	mov		ebp, 1)
	AS2(	mov		[ecx], edi)
	AS2(	mov		edi, [edx+4])
	AS2(	cmovc	eax, ebp)
	AS2(	mov		ebp, [ebx+4])
	AS2(	add		ebx, 8)
	AS2(	add		edi, eax)
	AS1(	jc		carry2AddP4)
	AS2(	xor		eax, eax)

	AS1(carry2AddP4:)
	AS2(	add		edi, ebp)
	AS2(	mov		ebp, 1)
	AS2(	cmovc	eax, ebp)
	AS2(	mov		[ecx+4], edi)
	AS2(	add		esi, 2)
	AS1(	jnz		loopstartAddP4)

	AS1(loopendAddP4:)

	AddEpilogue

	// Just to get rid of warnings
	// return 0;
}

CRYPTOPP_NAKED word P4Optimized::Subtract(word *C, const word *A, const word *B, unsigned int N)
{
	AddPrologue

	// now: ebx = B, ecx = C, edx = A, esi = N
	AS2(	xor		eax, eax)
	AS1(	neg		esi)
	AS1(	jz		loopendSubP4)		// if no dwords then nothing to do

	AS2(	mov		edi, [edx])
	AS2(	mov		ebp, [ebx])
	AS1(	jmp		carry1SubP4)

	AS1(loopstartSubP4:)
	AS2(	mov		edi, [edx+8])
	AS2(	add		edx, 8)
	AS2(	add		ecx, 8)
	AS2(	mov		ebp, [ebx])
	AS2(	sub		edi, eax)
	AS1(	jc		carry1SubP4)
	AS2(	xor		eax, eax)

	AS1(carry1SubP4:)
	AS2(	sub		edi, ebp)
	AS2(	mov		ebp, 1)
	AS2(	mov		[ecx], edi)
	AS2(	mov		edi, [edx+4])
	AS2(	cmovc	eax, ebp)
	AS2(	mov		ebp, [ebx+4])
	AS2(	add		ebx, 8)
	AS2(	sub		edi, eax)
	AS1(	jc		carry2SubP4)
	AS2(	xor		eax, eax)

	AS1(carry2SubP4:)
	AS2(	sub		edi, ebp)
	AS2(	mov		ebp, 1)
	AS2(	cmovc	eax, ebp)
	AS2(	mov		[ecx+4], edi)
	AS2(	add		esi, 2)
	AS1(	jnz		loopstartSubP4)

	AS1(loopendSubP4:)

	AddEpilogue

	// Just to get rid of warnings
	// return 0;
}

// multiply assembly code originally contributed by Leonard Janke

#define MulStartup \
	AS2(xor ebp, ebp) \
	AS2(xor edi, edi) \
	AS2(xor ebx, ebx) 

#define MulShiftCarry \
	AS2(mov ebp, edx) \
	AS2(mov edi, ebx) \
	AS2(xor ebx, ebx)

#define MulAccumulateBottom(i,j) \
	AS2(mov eax, [ecx+4*j]) \
	AS2(imul eax, dword ptr [esi+4*i]) \
	AS2(add ebp, eax)

#define MulAccumulate(i,j) \
	AS2(mov eax, [ecx+4*j]) \
	AS1(mul dword ptr [esi+4*i]) \
	AS2(add ebp, eax) \
	AS2(adc edi, edx) \
	AS2(adc bl, bh)

#define MulStoreDigit(i)  \
	AS2(mov edx, edi) \
	AS2(mov edi, [esp]) \
	AS2(mov [edi+4*i], ebp)

#define MulLastDiagonal(digits) \
	AS2(mov eax, [ecx+4*(digits-1)]) \
	AS1(mul dword ptr [esi+4*(digits-1)]) \
	AS2(add ebp, eax) \
	AS2(adc edx, edi) \
	AS2(mov edi, [esp]) \
	AS2(mov [edi+4*(2*digits-2)], ebp) \
	AS2(mov [edi+4*(2*digits-1)], edx)

CRYPTOPP_NAKED void PentiumOptimized::Multiply4(word* Z, const word* X, const word* Y)
{
	MulPrologue
	// now: [esp] = Z, esi = X, ecx = Y
	MulStartup
	MulAccumulate(0,0)
	MulStoreDigit(0)
	MulShiftCarry

	MulAccumulate(1,0)
	MulAccumulate(0,1)
	MulStoreDigit(1)
	MulShiftCarry

	MulAccumulate(2,0)
	MulAccumulate(1,1)
	MulAccumulate(0,2)
	MulStoreDigit(2)
	MulShiftCarry

	MulAccumulate(3,0)
	MulAccumulate(2,1)
	MulAccumulate(1,2)
	MulAccumulate(0,3)
	MulStoreDigit(3)
	MulShiftCarry

	MulAccumulate(3,1)
	MulAccumulate(2,2)
	MulAccumulate(1,3)
	MulStoreDigit(4)
	MulShiftCarry

	MulAccumulate(3,2)
	MulAccumulate(2,3)
	MulStoreDigit(5)
	MulShiftCarry

	MulLastDiagonal(4)
	MulEpilogue
}

CRYPTOPP_NAKED void PentiumOptimized::Multiply8(word* Z, const word* X, const word* Y)
{
	MulPrologue
	// now: [esp] = Z, esi = X, ecx = Y
	MulStartup
	MulAccumulate(0,0)
	MulStoreDigit(0)
	MulShiftCarry

	MulAccumulate(1,0)
	MulAccumulate(0,1)
	MulStoreDigit(1)
	MulShiftCarry

	MulAccumulate(2,0)
	MulAccumulate(1,1)
	MulAccumulate(0,2)
	MulStoreDigit(2)
	MulShiftCarry

	MulAccumulate(3,0)
	MulAccumulate(2,1)
	MulAccumulate(1,2)
	MulAccumulate(0,3)
	MulStoreDigit(3)
	MulShiftCarry

	MulAccumulate(4,0)
	MulAccumulate(3,1)
	MulAccumulate(2,2)
	MulAccumulate(1,3)
	MulAccumulate(0,4)
	MulStoreDigit(4)
	MulShiftCarry

	MulAccumulate(5,0)
	MulAccumulate(4,1)
	MulAccumulate(3,2)
	MulAccumulate(2,3)
	MulAccumulate(1,4)
	MulAccumulate(0,5)
	MulStoreDigit(5)
	MulShiftCarry

	MulAccumulate(6,0)
	MulAccumulate(5,1)
	MulAccumulate(4,2)
	MulAccumulate(3,3)
	MulAccumulate(2,4)
	MulAccumulate(1,5)
	MulAccumulate(0,6)
	MulStoreDigit(6)
	MulShiftCarry

	MulAccumulate(7,0)
	MulAccumulate(6,1)
	MulAccumulate(5,2)
	MulAccumulate(4,3)
	MulAccumulate(3,4)
	MulAccumulate(2,5)
	MulAccumulate(1,6)
	MulAccumulate(0,7)
	MulStoreDigit(7)
	MulShiftCarry

	MulAccumulate(7,1)
	MulAccumulate(6,2)
	MulAccumulate(5,3)
	MulAccumulate(4,4)
	MulAccumulate(3,5)
	MulAccumulate(2,6)
	MulAccumulate(1,7)
	MulStoreDigit(8)
	MulShiftCarry

	MulAccumulate(7,2)
	MulAccumulate(6,3)
	MulAccumulate(5,4)
	MulAccumulate(4,5)
	MulAccumulate(3,6)
	MulAccumulate(2,7)
	MulStoreDigit(9)
	MulShiftCarry

	MulAccumulate(7,3)
	MulAccumulate(6,4)
	MulAccumulate(5,5)
	MulAccumulate(4,6)
	MulAccumulate(3,7)
	MulStoreDigit(10)
	MulShiftCarry

	MulAccumulate(7,4)
	MulAccumulate(6,5)
	MulAccumulate(5,6)
	MulAccumulate(4,7)
	MulStoreDigit(11)
	MulShiftCarry

	MulAccumulate(7,5)
	MulAccumulate(6,6)
	MulAccumulate(5,7)
	MulStoreDigit(12)
	MulShiftCarry

	MulAccumulate(7,6)
	MulAccumulate(6,7)
	MulStoreDigit(13)
	MulShiftCarry

	MulLastDiagonal(8)
	MulEpilogue
}

CRYPTOPP_NAKED void PentiumOptimized::Multiply8Bottom(word* Z, const word* X, const word* Y)
{
	MulPrologue
	// now: [esp] = Z, esi = X, ecx = Y
	MulStartup
	MulAccumulate(0,0)
	MulStoreDigit(0)
	MulShiftCarry

	MulAccumulate(1,0)
	MulAccumulate(0,1)
	MulStoreDigit(1)
	MulShiftCarry

	MulAccumulate(2,0)
	MulAccumulate(1,1)
	MulAccumulate(0,2)
	MulStoreDigit(2)
	MulShiftCarry

	MulAccumulate(3,0)
	MulAccumulate(2,1)
	MulAccumulate(1,2)
	MulAccumulate(0,3)
	MulStoreDigit(3)
	MulShiftCarry

	MulAccumulate(4,0)
	MulAccumulate(3,1)
	MulAccumulate(2,2)
	MulAccumulate(1,3)
	MulAccumulate(0,4)
	MulStoreDigit(4)
	MulShiftCarry

	MulAccumulate(5,0)
	MulAccumulate(4,1)
	MulAccumulate(3,2)
	MulAccumulate(2,3)
	MulAccumulate(1,4)
	MulAccumulate(0,5)
	MulStoreDigit(5)
	MulShiftCarry

	MulAccumulate(6,0)
	MulAccumulate(5,1)
	MulAccumulate(4,2)
	MulAccumulate(3,3)
	MulAccumulate(2,4)
	MulAccumulate(1,5)
	MulAccumulate(0,6)
	MulStoreDigit(6)
	MulShiftCarry

	MulAccumulateBottom(7,0)
	MulAccumulateBottom(6,1)
	MulAccumulateBottom(5,2)
	MulAccumulateBottom(4,3)
	MulAccumulateBottom(3,4)
	MulAccumulateBottom(2,5)
	MulAccumulateBottom(1,6)
	MulAccumulateBottom(0,7)
	MulStoreDigit(7)
	MulEpilogue
}

#undef AS1
#undef AS2

#else	// not x86 - no processor specific code at this layer

typedef Portable LowLevel;

#endif

#ifdef SSE2_INTRINSICS_AVAILABLE

#ifdef __GNUC__
#define CRYPTOPP_FASTCALL
#else
#define CRYPTOPP_FASTCALL __fastcall
#endif

static void CRYPTOPP_FASTCALL P4_Mul(__m128i *C, const __m128i *A, const __m128i *B)
{
	__m128i a3210 = _mm_load_si128(A);
	__m128i b3210 = _mm_load_si128(B);

	__m128i sum;

	__m128i z = _mm_setzero_si128();
	__m128i a2b2_a0b0 = _mm_mul_epu32(a3210, b3210);
	C[0] = a2b2_a0b0;

	__m128i a3120 = _mm_shuffle_epi32(a3210, _MM_SHUFFLE(3, 1, 2, 0));
	__m128i b3021 = _mm_shuffle_epi32(b3210, _MM_SHUFFLE(3, 0, 2, 1));
	__m128i a1b0_a0b1 = _mm_mul_epu32(a3120, b3021);
	__m128i a1b0 = _mm_unpackhi_epi32(a1b0_a0b1, z);
	__m128i a0b1 = _mm_unpacklo_epi32(a1b0_a0b1, z);
	C[1] = _mm_add_epi64(a1b0, a0b1);

	__m128i a31 = _mm_srli_epi64(a3210, 32);
	__m128i b31 = _mm_srli_epi64(b3210, 32);
	__m128i a3b3_a1b1 = _mm_mul_epu32(a31, b31);
	C[6] = a3b3_a1b1;

	__m128i a1b1 = _mm_unpacklo_epi32(a3b3_a1b1, z);
	__m128i b3012 = _mm_shuffle_epi32(b3210, _MM_SHUFFLE(3, 0, 1, 2));
	__m128i a2b0_a0b2 = _mm_mul_epu32(a3210, b3012);
	__m128i a0b2 = _mm_unpacklo_epi32(a2b0_a0b2, z);
	__m128i a2b0 = _mm_unpackhi_epi32(a2b0_a0b2, z);
	sum = _mm_add_epi64(a1b1, a0b2);
	C[2] = _mm_add_epi64(sum, a2b0);

	__m128i a2301 = _mm_shuffle_epi32(a3210, _MM_SHUFFLE(2, 3, 0, 1));
	__m128i b2103 = _mm_shuffle_epi32(b3210, _MM_SHUFFLE(2, 1, 0, 3));
	__m128i a3b0_a1b2 = _mm_mul_epu32(a2301, b3012);
	__m128i a2b1_a0b3 = _mm_mul_epu32(a3210, b2103);
	__m128i a3b0 = _mm_unpackhi_epi32(a3b0_a1b2, z);
	__m128i a1b2 = _mm_unpacklo_epi32(a3b0_a1b2, z);
	__m128i a2b1 = _mm_unpackhi_epi32(a2b1_a0b3, z);
	__m128i a0b3 = _mm_unpacklo_epi32(a2b1_a0b3, z);
	__m128i sum1 = _mm_add_epi64(a3b0, a1b2);
	sum = _mm_add_epi64(a2b1, a0b3);
	C[3] = _mm_add_epi64(sum, sum1);

	__m128i	a3b1_a1b3 = _mm_mul_epu32(a2301, b2103);
	__m128i a2b2 = _mm_unpackhi_epi32(a2b2_a0b0, z);
	__m128i a3b1 = _mm_unpackhi_epi32(a3b1_a1b3, z);
	__m128i a1b3 = _mm_unpacklo_epi32(a3b1_a1b3, z);
	sum = _mm_add_epi64(a2b2, a3b1);
	C[4] = _mm_add_epi64(sum, a1b3);

	__m128i a1302 = _mm_shuffle_epi32(a3210, _MM_SHUFFLE(1, 3, 0, 2));
	__m128i b1203 = _mm_shuffle_epi32(b3210, _MM_SHUFFLE(1, 2, 0, 3));
	__m128i a3b2_a2b3 = _mm_mul_epu32(a1302, b1203);
	__m128i a3b2 = _mm_unpackhi_epi32(a3b2_a2b3, z);
	__m128i a2b3 = _mm_unpacklo_epi32(a3b2_a2b3, z);
	C[5] = _mm_add_epi64(a3b2, a2b3);
}

#endif	// #ifdef SSE2_INTRINSICS_AVAILABLE

// ********************************************************

#define A0		A
#define A1		(A+N2)
#define B0		B
#define B1		(B+N2)

#define T0		T
#define T1		(T+N2)
#define T2		(T+N)
#define T3		(T+N+N2)

#define R0		R
#define R1		(R+N2)
#define R2		(R+N)
#define R3		(R+N+N2)

// R[2*N] - result = A*B
// T[2*N] - temporary work space
// A[N] --- multiplier
// B[N] --- multiplicant

void RecursiveMultiply(word *R, word *T, const word *A, const word *B, unsigned int N)
{
	assert(N>=2 && N%2==0);

	if (LowLevel::MultiplyRecursionLimit() >= 8 && N==8)
		LowLevel::Multiply8(R, A, B);
	else if (LowLevel::MultiplyRecursionLimit() >= 4 && N==4)
		LowLevel::Multiply4(R, A, B);
	else if (N==2)
		LowLevel::Multiply2(R, A, B);
	else
	{
		const unsigned int N2 = N/2;
		int carry;

		int aComp = Compare(A0, A1, N2);
		int bComp = Compare(B0, B1, N2);

		switch (2*aComp + aComp + bComp)
		{
		case -4:
			LowLevel::Subtract(R0, A1, A0, N2);
			LowLevel::Subtract(R1, B0, B1, N2);
			RecursiveMultiply(T0, T2, R0, R1, N2);
			LowLevel::Subtract(T1, T1, R0, N2);
			carry = -1;
			break;
		case -2:
			LowLevel::Subtract(R0, A1, A0, N2);
			LowLevel::Subtract(R1, B0, B1, N2);
			RecursiveMultiply(T0, T2, R0, R1, N2);
			carry = 0;
			break;
		case 2:
			LowLevel::Subtract(R0, A0, A1, N2);
			LowLevel::Subtract(R1, B1, B0, N2);
			RecursiveMultiply(T0, T2, R0, R1, N2);
			carry = 0;
			break;
		case 4:
			LowLevel::Subtract(R0, A1, A0, N2);
			LowLevel::Subtract(R1, B0, B1, N2);
			RecursiveMultiply(T0, T2, R0, R1, N2);
			LowLevel::Subtract(T1, T1, R1, N2);
			carry = -1;
			break;
		default:
			SetWords(T0, 0, N);
			carry = 0;
		}

		RecursiveMultiply(R0, T2, A0, B0, N2);
		RecursiveMultiply(R2, T2, A1, B1, N2);

		// now T[01] holds (A1-A0)*(B0-B1), R[01] holds A0*B0, R[23] holds A1*B1

		carry += LowLevel::Add(T0, T0, R0, N);
		carry += LowLevel::Add(T0, T0, R2, N);
		carry += LowLevel::Add(R1, R1, T0, N);

		assert (carry >= 0 && carry <= 2);
		Increment(R3, N2, carry);
	}
}

// R[2*N] - result = A*A
// T[2*N] - temporary work space
// A[N] --- number to be squared

void RecursiveSquare(word *R, word *T, const word *A, unsigned int N)
{
	assert(N && N%2==0);
	if (LowLevel::SquareRecursionLimit() >= 8 && N==8)
		LowLevel::Square8(R, A);
	if (LowLevel::SquareRecursionLimit() >= 4 && N==4)
		LowLevel::Square4(R, A);
	else if (N==2)
		LowLevel::Square2(R, A);
	else
	{
		const unsigned int N2 = N/2;

		RecursiveSquare(R0, T2, A0, N2);
		RecursiveSquare(R2, T2, A1, N2);
		RecursiveMultiply(T0, T2, A0, A1, N2);

		word carry = LowLevel::Add(R1, R1, T0, N);
		carry += LowLevel::Add(R1, R1, T0, N);
		Increment(R3, N2, carry);
	}
}

// R[N] - bottom half of A*B
// T[N] - temporary work space
// A[N] - multiplier
// B[N] - multiplicant

void RecursiveMultiplyBottom(word *R, word *T, const word *A, const word *B, unsigned int N)
{
	assert(N>=2 && N%2==0);
	if (LowLevel::MultiplyBottomRecursionLimit() >= 8 && N==8)
		LowLevel::Multiply8Bottom(R, A, B);
	else if (LowLevel::MultiplyBottomRecursionLimit() >= 4 && N==4)
		LowLevel::Multiply4Bottom(R, A, B);
	else if (N==2)
		LowLevel::Multiply2Bottom(R, A, B);
	else
	{
		const unsigned int N2 = N/2;

		RecursiveMultiply(R, T, A0, B0, N2);
		RecursiveMultiplyBottom(T0, T1, A1, B0, N2);
		LowLevel::Add(R1, R1, T0, N2);
		RecursiveMultiplyBottom(T0, T1, A0, B1, N2);
		LowLevel::Add(R1, R1, T0, N2);
	}
}

// R[N] --- upper half of A*B
// T[2*N] - temporary work space
// L[N] --- lower half of A*B
// A[N] --- multiplier
// B[N] --- multiplicant

void RecursiveMultiplyTop(word *R, word *T, const word *L, const word *A, const word *B, unsigned int N)
{
	assert(N>=2 && N%2==0);

	if (N==4)
	{
		LowLevel::Multiply4(T, A, B);
		memcpy(R, T+4, 4*WORD_SIZE);
	}
	else if (N==2)
	{
		LowLevel::Multiply2(T, A, B);
		memcpy(R, T+2, 2*WORD_SIZE);
	}
	else
	{
		const unsigned int N2 = N/2;
		int carry;

		int aComp = Compare(A0, A1, N2);
		int bComp = Compare(B0, B1, N2);

		switch (2*aComp + aComp + bComp)
		{
		case -4:
			LowLevel::Subtract(R0, A1, A0, N2);
			LowLevel::Subtract(R1, B0, B1, N2);
			RecursiveMultiply(T0, T2, R0, R1, N2);
			LowLevel::Subtract(T1, T1, R0, N2);
			carry = -1;
			break;
		case -2:
			LowLevel::Subtract(R0, A1, A0, N2);
			LowLevel::Subtract(R1, B0, B1, N2);
			RecursiveMultiply(T0, T2, R0, R1, N2);
			carry = 0;
			break;
		case 2:
			LowLevel::Subtract(R0, A0, A1, N2);
			LowLevel::Subtract(R1, B1, B0, N2);
			RecursiveMultiply(T0, T2, R0, R1, N2);
			carry = 0;
			break;
		case 4:
			LowLevel::Subtract(R0, A1, A0, N2);
			LowLevel::Subtract(R1, B0, B1, N2);
			RecursiveMultiply(T0, T2, R0, R1, N2);
			LowLevel::Subtract(T1, T1, R1, N2);
			carry = -1;
			break;
		default:
			SetWords(T0, 0, N);
			carry = 0;
		}

		RecursiveMultiply(T2, R0, A1, B1, N2);

		// now T[01] holds (A1-A0)*(B0-B1), T[23] holds A1*B1

		word c2 = LowLevel::Subtract(R0, L+N2, L, N2);
		c2 += LowLevel::Subtract(R0, R0, T0, N2);
		word t = (Compare(R0, T2, N2) == -1);

		carry += t;
		carry += Increment(R0, N2, c2+t);
		carry += LowLevel::Add(R0, R0, T1, N2);
		carry += LowLevel::Add(R0, R0, T3, N2);
		assert (carry >= 0 && carry <= 2);

		CopyWords(R1, T3, N2);
		Increment(R1, N2, carry);
	}
}

inline word Add(word *C, const word *A, const word *B, unsigned int N)
{
	return LowLevel::Add(C, A, B, N);
}

inline word Subtract(word *C, const word *A, const word *B, unsigned int N)
{
	return LowLevel::Subtract(C, A, B, N);
}

inline void Multiply(word *R, word *T, const word *A, const word *B, unsigned int N)
{
	RecursiveMultiply(R, T, A, B, N);
}

inline void Square(word *R, word *T, const word *A, unsigned int N)
{
	RecursiveSquare(R, T, A, N);
}

inline void MultiplyBottom(word *R, word *T, const word *A, const word *B, unsigned int N)
{
	RecursiveMultiplyBottom(R, T, A, B, N);
}

inline void MultiplyTop(word *R, word *T, const word *L, const word *A, const word *B, unsigned int N)
{
	RecursiveMultiplyTop(R, T, L, A, B, N);
}

static word LinearMultiply(word *C, const word *A, word B, unsigned int N)
{
	word carry=0;
	for(unsigned i=0; i<N; i++)
	{
		DWord p = DWord::MultiplyAndAdd(A[i], B, carry);
		C[i] = p.GetLowHalf();
		carry = p.GetHighHalf();
	}
	return carry;
}

// R[NA+NB] - result = A*B
// T[NA+NB] - temporary work space
// A[NA] ---- multiplier
// B[NB] ---- multiplicant

void AsymmetricMultiply(word *R, word *T, const word *A, unsigned int NA, const word *B, unsigned int NB)
{
	if (NA == NB)
	{
		if (A == B)
			Square(R, T, A, NA);
		else
			Multiply(R, T, A, B, NA);

		return;
	}

	if (NA > NB)
	{
		std::swap(A, B);
		std::swap(NA, NB);
	}

	assert(NB % NA == 0);
	assert((NB/NA)%2 == 0); 	// NB is an even multiple of NA

	if (NA==2 && !A[1])
	{
		switch (A[0])
		{
		case 0:
			SetWords(R, 0, NB+2);
			return;
		case 1:
			CopyWords(R, B, NB);
			R[NB] = R[NB+1] = 0;
			return;
		default:
			R[NB] = LinearMultiply(R, B, A[0], NB);
			R[NB+1] = 0;
			return;
		}
	}

	Multiply(R, T, A, B, NA);
	CopyWords(T+2*NA, R+NA, NA);

	unsigned i;

	for (i=2*NA; i<NB; i+=2*NA)
		Multiply(T+NA+i, T, A, B+i, NA);
	for (i=NA; i<NB; i+=2*NA)
		Multiply(R+i, T, A, B+i, NA);

	if (Add(R+NA, R+NA, T+2*NA, NB-NA))
		Increment(R+NB, NA);
}

// R[N] ----- result = A inverse mod 2**(WORD_BITS*N)
// T[3*N/2] - temporary work space
// A[N] ----- an odd number as input

void RecursiveInverseModPower2(word *R, word *T, const word *A, unsigned int N)
{
	if (N==2)
	{
		T[0] = AtomicInverseModPower2(A[0]);
		T[1] = 0;
		LowLevel::Multiply2Bottom(T+2, T, A);
		TwosComplement(T+2, 2);
		Increment(T+2, 2, 2);
		LowLevel::Multiply2Bottom(R, T, T+2);
	}
	else
	{
		const unsigned int N2 = N/2;
		RecursiveInverseModPower2(R0, T0, A0, N2);
		T0[0] = 1;
		SetWords(T0+1, 0, N2-1);
		MultiplyTop(R1, T1, T0, R0, A0, N2);
		MultiplyBottom(T0, T1, R0, A1, N2);
		Add(T0, R1, T0, N2);
		TwosComplement(T0, N2);
		MultiplyBottom(R1, T1, R0, T0, N2);
	}
}

// R[N] --- result = X/(2**(WORD_BITS*N)) mod M
// T[3*N] - temporary work space
// X[2*N] - number to be reduced
// M[N] --- modulus
// U[N] --- multiplicative inverse of M mod 2**(WORD_BITS*N)

void MontgomeryReduce(word *R, word *T, const word *X, const word *M, const word *U, unsigned int N)
{
	MultiplyBottom(R, T, X, U, N);
	MultiplyTop(T, T+N, X, R, M, N);
	word borrow = Subtract(T, X+N, T, N);
	// defend against timing attack by doing this Add even when not needed
	word carry = Add(T+N, T, M, N);
	assert(carry || !borrow);
	CopyWords(R, T + (borrow ? N : 0), N);
}

// R[N] --- result = X/(2**(WORD_BITS*N/2)) mod M
// T[2*N] - temporary work space
// X[2*N] - number to be reduced
// M[N] --- modulus
// U[N/2] - multiplicative inverse of M mod 2**(WORD_BITS*N/2)
// V[N] --- 2**(WORD_BITS*3*N/2) mod M

void HalfMontgomeryReduce(word *R, word *T, const word *X, const word *M, const word *U, const word *V, unsigned int N)
{
	assert(N%2==0 && N>=4);

#define M0		M
#define M1		(M+N2)
#define V0		V
#define V1		(V+N2)

#define X0		X
#define X1		(X+N2)
#define X2		(X+N)
#define X3		(X+N+N2)

	const unsigned int N2 = N/2;
	Multiply(T0, T2, V0, X3, N2);
	int c2 = Add(T0, T0, X0, N);
	MultiplyBottom(T3, T2, T0, U, N2);
	MultiplyTop(T2, R, T0, T3, M0, N2);
	c2 -= Subtract(T2, T1, T2, N2);
	Multiply(T0, R, T3, M1, N2);
	c2 -= Subtract(T0, T2, T0, N2);
	int c3 = -(int)Subtract(T1, X2, T1, N2);
	Multiply(R0, T2, V1, X3, N2);
	c3 += Add(R, R, T, N);

	if (c2>0)
		c3 += Increment(R1, N2);
	else if (c2<0)
		c3 -= Decrement(R1, N2, -c2);

	assert(c3>=-1 && c3<=1);
	if (c3>0)
		Subtract(R, R, M, N);
	else if (c3<0)
		Add(R, R, M, N);

#undef M0
#undef M1
#undef V0
#undef V1

#undef X0
#undef X1
#undef X2
#undef X3
}

#undef A0
#undef A1
#undef B0
#undef B1

#undef T0
#undef T1
#undef T2
#undef T3

#undef R0
#undef R1
#undef R2
#undef R3

static inline void AtomicDivide(word *Q, const word *A, const word *B)
{
	word T[4];
	DWord q = DivideFourWordsByTwo<word, DWord>(T, DWord(A[0], A[1]), DWord(A[2], A[3]), DWord(B[0], B[1]));
	Q[0] = q.GetLowHalf();
	Q[1] = q.GetHighHalf();
}

// for use by Divide(), corrects the underestimated quotient {Q1,Q0}
static void CorrectQuotientEstimate(word *R, word *T, word *Q, const word *B, unsigned int N)
{
	assert(N && N%2==0);

	if (Q[1])
	{
		T[N] = T[N+1] = 0;
		unsigned i;
		for (i=0; i<N; i+=4)
			LowLevel::Multiply2(T+i, Q, B+i);
		for (i=2; i<N; i+=4)
			if (LowLevel::Multiply2Add(T+i, Q, B+i))
				T[i+5] += (++T[i+4]==0);
	}
	else
	{
		T[N] = LinearMultiply(T, B, Q[0], N);
		T[N+1] = 0;
	}

	word borrow = Subtract(R, R, T, N+2);
	assert(!borrow && !R[N+1]);

	while (R[N] || Compare(R, B, N) >= 0)
	{
		R[N] -= Subtract(R, R, B, N);
		Q[1] += (++Q[0]==0);
		assert(Q[0] || Q[1]); // no overflow
	}
}

// R[NB] -------- remainder = A%B
// Q[NA-NB+2] --- quotient	= A/B
// T[NA+2*NB+4] - temp work space
// A[NA] -------- dividend
// B[NB] -------- divisor

void Divide(word *R, word *Q, word *T, const word *A, unsigned int NA, const word *B, unsigned int NB)
{
	assert(NA && NB && NA%2==0 && NB%2==0);
	assert(B[NB-1] || B[NB-2]);
	assert(NB <= NA);

	// set up temporary work space
	word *const TA=T;
	word *const TB=T+NA+2;
	word *const TP=T+NA+2+NB;

	// copy B into TB and normalize it so that TB has highest bit set to 1
	unsigned shiftWords = (B[NB-1]==0);
	TB[0] = TB[NB-1] = 0;
	CopyWords(TB+shiftWords, B, NB-shiftWords);
	unsigned shiftBits = WORD_BITS - BitPrecision(TB[NB-1]);
	assert(shiftBits < WORD_BITS);
	ShiftWordsLeftByBits(TB, NB, shiftBits);

	// copy A into TA and normalize it
	TA[0] = TA[NA] = TA[NA+1] = 0;
	CopyWords(TA+shiftWords, A, NA);
	ShiftWordsLeftByBits(TA, NA+2, shiftBits);

	if (TA[NA+1]==0 && TA[NA] <= 1)
	{
		Q[NA-NB+1] = Q[NA-NB] = 0;
		while (TA[NA] || Compare(TA+NA-NB, TB, NB) >= 0)
		{
			TA[NA] -= Subtract(TA+NA-NB, TA+NA-NB, TB, NB);
			++Q[NA-NB];
		}
	}
	else
	{
		NA+=2;
		assert(Compare(TA+NA-NB, TB, NB) < 0);
	}

	word BT[2];
	BT[0] = TB[NB-2] + 1;
	BT[1] = TB[NB-1] + (BT[0]==0);

	// start reducing TA mod TB, 2 words at a time
	for (unsigned i=NA-2; i>=NB; i-=2)
	{
		AtomicDivide(Q+i-NB, TA+i-2, BT);
		CorrectQuotientEstimate(TA+i-NB, TP, Q+i-NB, TB, NB);
	}

	// copy TA into R, and denormalize it
	CopyWords(R, TA+shiftWords, NB);
	ShiftWordsRightByBits(R, NB, shiftBits);
}

static inline unsigned int EvenWordCount(const word *X, unsigned int N)
{
	while (N && X[N-2]==0 && X[N-1]==0)
		N-=2;
	return N;
}

// return k
// R[N] --- result = A^(-1) * 2^k mod M
// T[4*N] - temporary work space
// A[NA] -- number to take inverse of
// M[N] --- modulus

unsigned int AlmostInverse(word *R, word *T, const word *A, unsigned int NA, const word *M, unsigned int N)
{
	assert(NA<=N && N && N%2==0);

	word *b = T;
	word *c = T+N;
	word *f = T+2*N;
	word *g = T+3*N;
	unsigned int bcLen=2, fgLen=EvenWordCount(M, N);
	unsigned int k=0, s=0;

	SetWords(T, 0, 3*N);
	b[0]=1;
	CopyWords(f, A, NA);
	CopyWords(g, M, N);

	while (1)
	{
		word t=f[0];
		while (!t)
		{
			if (EvenWordCount(f, fgLen)==0)
			{
				SetWords(R, 0, N);
				return 0;
			}

			ShiftWordsRightByWords(f, fgLen, 1);
			if (c[bcLen-1]) bcLen+=2;
			assert(bcLen <= N);
			ShiftWordsLeftByWords(c, bcLen, 1);
			k+=WORD_BITS;
			t=f[0];
		}

		unsigned int i=0;
		while (t%2 == 0)
		{
			t>>=1;
			i++;
		}
		k+=i;

		if (t==1 && f[1]==0 && EvenWordCount(f, fgLen)==2)
		{
			if (s%2==0)
				CopyWords(R, b, N);
			else
				Subtract(R, M, b, N);
			return k;
		}

		ShiftWordsRightByBits(f, fgLen, i);
		t=ShiftWordsLeftByBits(c, bcLen, i);
		if (t)
		{
			c[bcLen] = t;
			bcLen+=2;
			assert(bcLen <= N);
		}

		if (f[fgLen-2]==0 && g[fgLen-2]==0 && f[fgLen-1]==0 && g[fgLen-1]==0)
			fgLen-=2;

		if (Compare(f, g, fgLen)==-1)
		{
			std::swap(f, g);
			std::swap(b, c);
			s++;
		}

		Subtract(f, f, g, fgLen);

		if (Add(b, b, c, bcLen))
		{
			b[bcLen] = 1;
			bcLen+=2;
			assert(bcLen <= N);
		}
	}
}

// R[N] - result = A/(2^k) mod M
// A[N] - input
// M[N] - modulus

void DivideByPower2Mod(word *R, const word *A, unsigned int k, const word *M, unsigned int N)
{
	CopyWords(R, A, N);

	while (k--)
	{
		if (R[0]%2==0)
			ShiftWordsRightByBits(R, N, 1);
		else
		{
			word carry = Add(R, R, M, N);
			ShiftWordsRightByBits(R, N, 1);
			R[N-1] += carry<<(WORD_BITS-1);
		}
	}
}

// R[N] - result = A*(2^k) mod M
// A[N] - input
// M[N] - modulus

void MultiplyByPower2Mod(word *R, const word *A, unsigned int k, const word *M, unsigned int N)
{
	CopyWords(R, A, N);

	while (k--)
		if (ShiftWordsLeftByBits(R, N, 1) || Compare(R, M, N)>=0)
			Subtract(R, R, M, N);
}

// ******************************************************************

static const unsigned int RoundupSizeTable[] = {2, 2, 2, 4, 4, 8, 8, 8, 8};

static inline unsigned int RoundupSize(unsigned int n)
{
	if (n<=8)
		return RoundupSizeTable[n];
	else if (n<=16)
		return 16;
	else if (n<=32)
		return 32;
	else if (n<=64)
		return 64;
	else return 1U << BitPrecision(n-1);
}

Integer::Integer()
	: reg(2), sign(POSITIVE)
{
	reg[0] = reg[1] = 0;
}

Integer::Integer(const Integer& t)
	: ASN1Object(), reg(RoundupSize(t.WordCount())), sign(t.sign)
{
	CopyWords(reg, t.reg, reg.size());
}

Integer::Integer(Sign s, lword value)
	: reg(2), sign(s)
{
	reg[0] = word(value);
	reg[1] = word(SafeRightShift<WORD_BITS>(value));
}

Integer::Integer(signed long value)
	: reg(2)
{
	if (value >= 0)
		sign = POSITIVE;
	else
	{
		sign = NEGATIVE;
		value = -value;
	}
	reg[0] = word(value);
	reg[1] = word(SafeRightShift<WORD_BITS>((unsigned long)value));
}

Integer::Integer(Sign s, word high, word low)
	: reg(2), sign(s)
{
	reg[0] = low;
	reg[1] = high;
}

bool Integer::IsConvertableToLong() const
{
	if (ByteCount() > sizeof(long))
		return false;

	unsigned long value = reg[0];
	value += SafeLeftShift<WORD_BITS, unsigned long>(reg[1]);

	if (sign==POSITIVE)
		return (signed long)value >= 0;
	else
		return -(signed long)value < 0;
}

signed long Integer::ConvertToLong() const
{
	assert(IsConvertableToLong());

	unsigned long value = reg[0];
	value += SafeLeftShift<WORD_BITS, unsigned long>(reg[1]);
	return sign==POSITIVE ? value : -(signed long)value;
}

Integer::Integer(BufferedTransformation &encodedInteger, unsigned int byteCount, Signedness s)
{
	Decode(encodedInteger, byteCount, s);
}

Integer::Integer(const byte *encodedInteger, unsigned int byteCount, Signedness s)
{
	Decode(encodedInteger, byteCount, s);
}

Integer::Integer(BufferedTransformation &bt)
{
	BERDecode(bt);
}

Integer::Integer(RandomNumberGenerator &rng, unsigned int bitcount)
{
	Randomize(rng, bitcount);
}

Integer::Integer(RandomNumberGenerator &rng, const Integer &min, const Integer &max, RandomNumberType rnType, const Integer &equiv, const Integer &mod)
{
	if (!Randomize(rng, min, max, rnType, equiv, mod))
		throw Integer::RandomNumberNotFound();
}

Integer Integer::Power2(unsigned int e)
{
	Integer r((word)0, BitsToWords(e+1));
	r.SetBit(e);
	return r;
}

template <long i>
struct NewInteger
{
	Integer * operator()() const
	{
		return new Integer(i);
	}
};

const Integer &Integer::Zero()
{
	return Singleton<Integer>().Ref();
}

const Integer &Integer::One()
{
	return Singleton<Integer, NewInteger<1> >().Ref();
}

const Integer &Integer::Two()
{
	return Singleton<Integer, NewInteger<2> >().Ref();
}

bool Integer::operator!() const
{
	return IsNegative() ? false : (reg[0]==0 && WordCount()==0);
}

Integer& Integer::operator=(const Integer& t)
{
	if (this != &t)
	{
		reg.New(RoundupSize(t.WordCount()));
		CopyWords(reg, t.reg, reg.size());
		sign = t.sign;
	}
	return *this;
}

bool Integer::GetBit(unsigned int n) const
{
	if (n/WORD_BITS >= reg.size())
		return 0;
	else
		return bool((reg[n/WORD_BITS] >> (n % WORD_BITS)) & 1);
}

void Integer::SetBit(unsigned int n, bool value)
{
	if (value)
	{
		reg.CleanGrow(RoundupSize(BitsToWords(n+1)));
		reg[n/WORD_BITS] |= (word(1) << (n%WORD_BITS));
	}
	else
	{
		if (n/WORD_BITS < reg.size())
			reg[n/WORD_BITS] &= ~(word(1) << (n%WORD_BITS));
	}
}

byte Integer::GetByte(unsigned int n) const
{
	if (n/WORD_SIZE >= reg.size())
		return 0;
	else
		return byte(reg[n/WORD_SIZE] >> ((n%WORD_SIZE)*8));
}

void Integer::SetByte(unsigned int n, byte value)
{
	reg.CleanGrow(RoundupSize(BytesToWords(n+1)));
	reg[n/WORD_SIZE] &= ~(word(0xff) << 8*(n%WORD_SIZE));
	reg[n/WORD_SIZE] |= (word(value) << 8*(n%WORD_SIZE));
}

unsigned long Integer::GetBits(unsigned int i, unsigned int n) const
{
	assert(n <= sizeof(unsigned long)*8);
	unsigned long v = 0;
	for (unsigned int j=0; j<n; j++)
		v |= GetBit(i+j) << j;
	return v;
}

Integer Integer::operator-() const
{
	Integer result(*this);
	result.Negate();
	return result;
}

Integer Integer::AbsoluteValue() const
{
	Integer result(*this);
	result.sign = POSITIVE;
	return result;
}

void Integer::swap(Integer &a)
{
	reg.swap(a.reg);
	std::swap(sign, a.sign);
}

Integer::Integer(word value, unsigned int length)
	: reg(RoundupSize(length)), sign(POSITIVE)
{
	reg[0] = value;
	SetWords(reg+1, 0, reg.size()-1);
}

template <class T>
static Integer StringToInteger(const T *str)
{
	word radix;
	// GCC workaround
	// std::char_traits doesn't exist in GCC 2.x
	// std::char_traits<wchar_t>::length() not defined in GCC 3.2 and STLport 4.5.3
	unsigned int length;
	for (length = 0; str[length] != 0; length++) {}

	Integer v;

	if (length == 0)
		return v;

	switch (str[length-1])
	{
	case 'h':
	case 'H':
		radix=16;
		break;
	case 'o':
	case 'O':
		radix=8;
		break;
	case 'b':
	case 'B':
		radix=2;
		break;
	default:
		radix=10;
	}

	if (length > 2 && str[0] == '0' && str[1] == 'x')
		radix = 16;

	for (unsigned i=0; i<length; i++)
	{
		word digit;

		if (str[i] >= '0' && str[i] <= '9')
			digit = str[i] - '0';
		else if (str[i] >= 'A' && str[i] <= 'F')
			digit = str[i] - 'A' + 10;
		else if (str[i] >= 'a' && str[i] <= 'f')
			digit = str[i] - 'a' + 10;
		else
			digit = radix;

		if (digit < radix)
		{
			v *= radix;
			v += digit;
		}
	}

	if (str[0] == '-')
		v.Negate();

	return v;
}

Integer::Integer(const char *str)
	: reg(2), sign(POSITIVE)
{
	*this = StringToInteger(str);
}

Integer::Integer(const wchar_t *str)
	: reg(2), sign(POSITIVE)
{
	*this = StringToInteger(str);
}

unsigned int Integer::WordCount() const
{
	return CountWords(reg, reg.size());
}

unsigned int Integer::ByteCount() const
{
	unsigned wordCount = WordCount();
	if (wordCount)
		return (wordCount-1)*WORD_SIZE + BytePrecision(reg[wordCount-1]);
	else
		return 0;
}

unsigned int Integer::BitCount() const
{
	unsigned wordCount = WordCount();
	if (wordCount)
		return (wordCount-1)*WORD_BITS + BitPrecision(reg[wordCount-1]);
	else
		return 0;
}

void Integer::Decode(const byte *input, unsigned int inputLen, Signedness s)
{
	StringStore store(input, inputLen);
	Decode(store, inputLen, s);
}

void Integer::Decode(BufferedTransformation &bt, unsigned int inputLen, Signedness s)
{
	assert(bt.MaxRetrievable() >= inputLen);

	byte b;
	bt.Peek(b);
	sign = ((s==SIGNED) && (b & 0x80)) ? NEGATIVE : POSITIVE;

	while (inputLen>0 && (sign==POSITIVE ? b==0 : b==0xff))
	{
		bt.Skip(1);
		inputLen--;
		bt.Peek(b);
	}

	reg.CleanNew(RoundupSize(BytesToWords(inputLen)));

	for (unsigned int i=inputLen; i > 0; i--)
	{
		bt.Get(b);
		reg[(i-1)/WORD_SIZE] |= word(b) << ((i-1)%WORD_SIZE)*8;
	}

	if (sign == NEGATIVE)
	{
		for (unsigned i=inputLen; i<reg.size()*WORD_SIZE; i++)
			reg[i/WORD_SIZE] |= word(0xff) << (i%WORD_SIZE)*8;
		TwosComplement(reg, reg.size());
	}
}

unsigned int Integer::MinEncodedSize(Signedness signedness) const
{
	unsigned int outputLen = STDMAX(1U, ByteCount());
	if (signedness == UNSIGNED)
		return outputLen;
	if (NotNegative() && (GetByte(outputLen-1) & 0x80))
		outputLen++;
	if (IsNegative() && *this < -Power2(outputLen*8-1))
		outputLen++;
	return outputLen;
}

unsigned int Integer::Encode(byte *output, unsigned int outputLen, Signedness signedness) const
{
	ArraySink sink(output, outputLen);
	return Encode(sink, outputLen, signedness);
}

unsigned int Integer::Encode(BufferedTransformation &bt, unsigned int outputLen, Signedness signedness) const
{
	if (signedness == UNSIGNED || NotNegative())
	{
		for (unsigned int i=outputLen; i > 0; i--)
			bt.Put(GetByte(i-1));
	}
	else
	{
		// take two's complement of *this
		Integer temp = Integer::Power2(8*STDMAX(ByteCount(), outputLen)) + *this;
		for (unsigned i=0; i<outputLen; i++)
			bt.Put(temp.GetByte(outputLen-i-1));
	}
	return outputLen;
}

void Integer::DEREncode(BufferedTransformation &bt) const
{
	DERGeneralEncoder enc(bt, INTEGER);
	Encode(enc, MinEncodedSize(SIGNED), SIGNED);
	enc.MessageEnd();
}

void Integer::BERDecode(const byte *input, unsigned int len)
{
	StringStore store(input, len);
	BERDecode(store);
}

void Integer::BERDecode(BufferedTransformation &bt)
{
	BERGeneralDecoder dec(bt, INTEGER);
	if (!dec.IsDefiniteLength() || dec.MaxRetrievable() < dec.RemainingLength())
		BERDecodeError();
	Decode(dec, dec.RemainingLength(), SIGNED);
	dec.MessageEnd();
}

void Integer::DEREncodeAsOctetString(BufferedTransformation &bt, unsigned int length) const
{
	DERGeneralEncoder enc(bt, OCTET_STRING);
	Encode(enc, length);
	enc.MessageEnd();
}

void Integer::BERDecodeAsOctetString(BufferedTransformation &bt, unsigned int length)
{
	BERGeneralDecoder dec(bt, OCTET_STRING);
	if (!dec.IsDefiniteLength() || dec.RemainingLength() != length)
		BERDecodeError();
	Decode(dec, length);
	dec.MessageEnd();
}

unsigned int Integer::OpenPGPEncode(byte *output, unsigned int len) const
{
	ArraySink sink(output, len);
	return OpenPGPEncode(sink);
}

unsigned int Integer::OpenPGPEncode(BufferedTransformation &bt) const
{
	word16 bitCount = BitCount();
	bt.PutWord16(bitCount);
	return 2 + Encode(bt, BitsToBytes(bitCount));
}

void Integer::OpenPGPDecode(const byte *input, unsigned int len)
{
	StringStore store(input, len);
	OpenPGPDecode(store);
}

void Integer::OpenPGPDecode(BufferedTransformation &bt)
{
	word16 bitCount;
	if (bt.GetWord16(bitCount) != 2 || bt.MaxRetrievable() < BitsToBytes(bitCount))
		throw OpenPGPDecodeErr();
	Decode(bt, BitsToBytes(bitCount));
}

void Integer::Randomize(RandomNumberGenerator &rng, unsigned int nbits)
{
	const unsigned int nbytes = nbits/8 + 1;
	SecByteBlock buf(nbytes);
	rng.GenerateBlock(buf, nbytes);
	if (nbytes)
		buf[0] = (byte)Crop(buf[0], nbits % 8);
	Decode(buf, nbytes, UNSIGNED);
}

void Integer::Randomize(RandomNumberGenerator &rng, const Integer &min, const Integer &max)
{
	if (min > max)
		throw InvalidArgument("Integer: Min must be no greater than Max");

	Integer range = max - min;
	const unsigned int nbits = range.BitCount();

	do
	{
		Randomize(rng, nbits);
	}
	while (*this > range);

	*this += min;
}

bool Integer::Randomize(RandomNumberGenerator &rng, const Integer &min, const Integer &max, RandomNumberType rnType, const Integer &equiv, const Integer &mod)
{
	return GenerateRandomNoThrow(rng, MakeParameters("Min", min)("Max", max)("RandomNumberType", rnType)("EquivalentTo", equiv)("Mod", mod));
}

class KDF2_RNG : public RandomNumberGenerator
{
public:
	KDF2_RNG(const byte *seed, unsigned int seedSize)
		: m_counter(0), m_counterAndSeed(seedSize + 4)
	{
		memcpy(m_counterAndSeed + 4, seed, seedSize);
	}

	byte GenerateByte()
	{
		byte b;
		GenerateBlock(&b, 1);
		return b;
	}

	void GenerateBlock(byte *output, unsigned int size)
	{
		UnalignedPutWord(BIG_ENDIAN_ORDER, m_counterAndSeed, m_counter);
		++m_counter;
		P1363_KDF2<SHA1>::DeriveKey(output, size, m_counterAndSeed, m_counterAndSeed.size(), NULL, 0);
	}

private:
	word32 m_counter;
	SecByteBlock m_counterAndSeed;
};

bool Integer::GenerateRandomNoThrow(RandomNumberGenerator &i_rng, const NameValuePairs &params)
{
	Integer min = params.GetValueWithDefault("Min", Integer::Zero());
	Integer max;
	if (!params.GetValue("Max", max))
	{
		int bitLength;
		if (params.GetIntValue("BitLength", bitLength))
			max = Integer::Power2(bitLength);
		else
			throw InvalidArgument("Integer: missing Max argument");
	}
	if (min > max)
		throw InvalidArgument("Integer: Min must be no greater than Max");

	Integer equiv = params.GetValueWithDefault("EquivalentTo", Integer::Zero());
	Integer mod = params.GetValueWithDefault("Mod", Integer::One());

	if (equiv.IsNegative() || equiv >= mod)
		throw InvalidArgument("Integer: invalid EquivalentTo and/or Mod argument");

	Integer::RandomNumberType rnType = params.GetValueWithDefault("RandomNumberType", Integer::ANY);

	member_ptr<KDF2_RNG> kdf2Rng;
	ConstByteArrayParameter seed;
	if (params.GetValue("Seed", seed))
	{
		ByteQueue bq;
		DERSequenceEncoder seq(bq);
		min.DEREncode(seq);
		max.DEREncode(seq);
		equiv.DEREncode(seq);
		mod.DEREncode(seq);
		DEREncodeUnsigned(seq, rnType);
		DEREncodeOctetString(seq, seed.begin(), seed.size());
		seq.MessageEnd();

		SecByteBlock finalSeed(bq.MaxRetrievable());
		bq.Get(finalSeed, finalSeed.size());
		kdf2Rng.reset(new KDF2_RNG(finalSeed.begin(), finalSeed.size()));
	}
	RandomNumberGenerator &rng = kdf2Rng.get() ? (RandomNumberGenerator &)*kdf2Rng : i_rng;

	switch (rnType)
	{
		case ANY:
			if (mod == One())
				Randomize(rng, min, max);
			else
			{
				Integer min1 = min + (equiv-min)%mod;
				if (max < min1)
					return false;
				Randomize(rng, Zero(), (max - min1) / mod);
				*this *= mod;
				*this += min1;
			}
			return true;

		case PRIME:
		{
			const PrimeSelector *pSelector = params.GetValueWithDefault(Name::PointerToPrimeSelector(), (const PrimeSelector *)NULL);

			int i;
			i = 0;
			while (1)
			{
				if (++i==16)
				{
					// check if there are any suitable primes in [min, max]
					Integer first = min;
					if (FirstPrime(first, max, equiv, mod, pSelector))
					{
						// if there is only one suitable prime, we're done
						*this = first;
						if (!FirstPrime(first, max, equiv, mod, pSelector))
							return true;
					}
					else
						return false;
				}

				Randomize(rng, min, max);
				if (FirstPrime(*this, STDMIN(*this+mod*PrimeSearchInterval(max), max), equiv, mod, pSelector))
					return true;
			}
		}

		default:
			throw InvalidArgument("Integer: invalid RandomNumberType argument");
	}
}

std::istream& operator>>(std::istream& in, Integer &a)
{
	char c;
	unsigned int length = 0;
	SecBlock<char> str(length + 16);

	std::ws(in);

	do
	{
		in.read(&c, 1);
		str[length++] = c;
		if (length >= str.size())
			str.Grow(length + 16);
	}
	while (in && (c=='-' || c=='x' || (c>='0' && c<='9') || (c>='a' && c<='f') || (c>='A' && c<='F') || c=='h' || c=='H' || c=='o' || c=='O' || c==',' || c=='.'));

	if (in.gcount())
		in.putback(c);
	str[length-1] = '\0';
	a = Integer(str);

	return in;
}

std::ostream& operator<<(std::ostream& out, const Integer &a)
{
	// Get relevant conversion specifications from ostream.
	long f = out.flags() & std::ios::basefield; // Get base digits.
	int base, block;
	char suffix;
	switch(f)
	{
	case std::ios::oct :
		base = 8;
		block = 8;
		suffix = 'o';
		break;
	case std::ios::hex :
		base = 16;
		block = 4;
		suffix = 'h';
		break;
	default :
		base = 10;
		block = 3;
		suffix = '.';
	}

	SecBlock<char> s(a.BitCount() / (BitPrecision(base)-1) + 1);
	Integer temp1=a, temp2;
	unsigned i=0;
	const char vec[]="0123456789ABCDEF";

	if (a.IsNegative())
	{
		out << '-';
		temp1.Negate();
	}

	if (!a)
		out << '0';

	while (!!temp1)
	{
		word digit;
		Integer::Divide(digit, temp2, temp1, base);
		s[i++]=vec[digit];
		temp1=temp2;
	}

	while (i--)
	{
		out << s[i];
//		if (i && !(i%block))
//			out << ",";
	}
	return out << suffix;
}

Integer& Integer::operator++()
{
	if (NotNegative())
	{
		if (Increment(reg, reg.size()))
		{
			reg.CleanGrow(2*reg.size());
			reg[reg.size()/2]=1;
		}
	}
	else
	{
		word borrow = Decrement(reg, reg.size());
		assert(!borrow);
		if (WordCount()==0)
			*this = Zero();
	}
	return *this;
}

Integer& Integer::operator--()
{
	if (IsNegative())
	{
		if (Increment(reg, reg.size()))
		{
			reg.CleanGrow(2*reg.size());
			reg[reg.size()/2]=1;
		}
	}
	else
	{
		if (Decrement(reg, reg.size()))
			*this = -One();
	}
	return *this;
}

void PositiveAdd(Integer &sum, const Integer &a, const Integer& b)
{
	word carry;
	if (a.reg.size() == b.reg.size())
		carry = Add(sum.reg, a.reg, b.reg, a.reg.size());
	else if (a.reg.size() > b.reg.size())
	{
		carry = Add(sum.reg, a.reg, b.reg, b.reg.size());
		CopyWords(sum.reg+b.reg.size(), a.reg+b.reg.size(), a.reg.size()-b.reg.size());
		carry = Increment(sum.reg+b.reg.size(), a.reg.size()-b.reg.size(), carry);
	}
	else
	{
		carry = Add(sum.reg, a.reg, b.reg, a.reg.size());
		CopyWords(sum.reg+a.reg.size(), b.reg+a.reg.size(), b.reg.size()-a.reg.size());
		carry = Increment(sum.reg+a.reg.size(), b.reg.size()-a.reg.size(), carry);
	}

	if (carry)
	{
		sum.reg.CleanGrow(2*sum.reg.size());
		sum.reg[sum.reg.size()/2] = 1;
	}
	sum.sign = Integer::POSITIVE;
}

void PositiveSubtract(Integer &diff, const Integer &a, const Integer& b)
{
	unsigned aSize = a.WordCount();
	aSize += aSize%2;
	unsigned bSize = b.WordCount();
	bSize += bSize%2;

	if (aSize == bSize)
	{
		if (Compare(a.reg, b.reg, aSize) >= 0)
		{
			Subtract(diff.reg, a.reg, b.reg, aSize);
			diff.sign = Integer::POSITIVE;
		}
		else
		{
			Subtract(diff.reg, b.reg, a.reg, aSize);
			diff.sign = Integer::NEGATIVE;
		}
	}
	else if (aSize > bSize)
	{
		word borrow = Subtract(diff.reg, a.reg, b.reg, bSize);
		CopyWords(diff.reg+bSize, a.reg+bSize, aSize-bSize);
		borrow = Decrement(diff.reg+bSize, aSize-bSize, borrow);
		assert(!borrow);
		diff.sign = Integer::POSITIVE;
	}
	else
	{
		word borrow = Subtract(diff.reg, b.reg, a.reg, aSize);
		CopyWords(diff.reg+aSize, b.reg+aSize, bSize-aSize);
		borrow = Decrement(diff.reg+aSize, bSize-aSize, borrow);
		assert(!borrow);
		diff.sign = Integer::NEGATIVE;
	}
}

Integer Integer::Plus(const Integer& b) const
{
	Integer sum((word)0, STDMAX(reg.size(), b.reg.size()));
	if (NotNegative())
	{
		if (b.NotNegative())
			PositiveAdd(sum, *this, b);
		else
			PositiveSubtract(sum, *this, b);
	}
	else
	{
		if (b.NotNegative())
			PositiveSubtract(sum, b, *this);
		else
		{
			PositiveAdd(sum, *this, b);
			sum.sign = Integer::NEGATIVE;
		}
	}
	return sum;
}

Integer& Integer::operator+=(const Integer& t)
{
	reg.CleanGrow(t.reg.size());
	if (NotNegative())
	{
		if (t.NotNegative())
			PositiveAdd(*this, *this, t);
		else
			PositiveSubtract(*this, *this, t);
	}
	else
	{
		if (t.NotNegative())
			PositiveSubtract(*this, t, *this);
		else
		{
			PositiveAdd(*this, *this, t);
			sign = Integer::NEGATIVE;
		}
	}
	return *this;
}

Integer Integer::Minus(const Integer& b) const
{
	Integer diff((word)0, STDMAX(reg.size(), b.reg.size()));
	if (NotNegative())
	{
		if (b.NotNegative())
			PositiveSubtract(diff, *this, b);
		else
			PositiveAdd(diff, *this, b);
	}
	else
	{
		if (b.NotNegative())
		{
			PositiveAdd(diff, *this, b);
			diff.sign = Integer::NEGATIVE;
		}
		else
			PositiveSubtract(diff, b, *this);
	}
	return diff;
}

Integer& Integer::operator-=(const Integer& t)
{
	reg.CleanGrow(t.reg.size());
	if (NotNegative())
	{
		if (t.NotNegative())
			PositiveSubtract(*this, *this, t);
		else
			PositiveAdd(*this, *this, t);
	}
	else
	{
		if (t.NotNegative())
		{
			PositiveAdd(*this, *this, t);
			sign = Integer::NEGATIVE;
		}
		else
			PositiveSubtract(*this, t, *this);
	}
	return *this;
}

Integer& Integer::operator<<=(unsigned int n)
{
	const unsigned int wordCount = WordCount();
	const unsigned int shiftWords = n / WORD_BITS;
	const unsigned int shiftBits = n % WORD_BITS;

	reg.CleanGrow(RoundupSize(wordCount+BitsToWords(n)));
	ShiftWordsLeftByWords(reg, wordCount + shiftWords, shiftWords);
	ShiftWordsLeftByBits(reg+shiftWords, wordCount+BitsToWords(shiftBits), shiftBits);
	return *this;
}

Integer& Integer::operator>>=(unsigned int n)
{
	const unsigned int wordCount = WordCount();
	const unsigned int shiftWords = n / WORD_BITS;
	const unsigned int shiftBits = n % WORD_BITS;

	ShiftWordsRightByWords(reg, wordCount, shiftWords);
	if (wordCount > shiftWords)
		ShiftWordsRightByBits(reg, wordCount-shiftWords, shiftBits);
	if (IsNegative() && WordCount()==0)   // avoid -0
		*this = Zero();
	return *this;
}

void PositiveMultiply(Integer &product, const Integer &a, const Integer &b)
{
	unsigned aSize = RoundupSize(a.WordCount());
	unsigned bSize = RoundupSize(b.WordCount());

	product.reg.CleanNew(RoundupSize(aSize+bSize));
	product.sign = Integer::POSITIVE;

	SecAlignedWordBlock workspace(aSize + bSize);
	AsymmetricMultiply(product.reg, workspace, a.reg, aSize, b.reg, bSize);
}

void Multiply(Integer &product, const Integer &a, const Integer &b)
{
	PositiveMultiply(product, a, b);

	if (a.NotNegative() != b.NotNegative())
		product.Negate();
}

Integer Integer::Times(const Integer &b) const
{
	Integer product;
	Multiply(product, *this, b);
	return product;
}

void PositiveDivide(Integer &remainder, Integer &quotient,
				   const Integer &a, const Integer &b)
{
	unsigned aSize = a.WordCount();
	unsigned bSize = b.WordCount();

	if (!bSize)
		throw Integer::DivideByZero();

	if (a.PositiveCompare(b) == -1)
	{
		remainder = a;
		remainder.sign = Integer::POSITIVE;
		quotient = Integer::Zero();
		return;
	}

	aSize += aSize%2;	// round up to next even number
	bSize += bSize%2;

	remainder.reg.CleanNew(RoundupSize(bSize));
	remainder.sign = Integer::POSITIVE;
	quotient.reg.CleanNew(RoundupSize(aSize-bSize+2));
	quotient.sign = Integer::POSITIVE;

	SecAlignedWordBlock T(aSize+2*bSize+4);
	Divide(remainder.reg, quotient.reg, T, a.reg, aSize, b.reg, bSize);
}

void Integer::Divide(Integer &remainder, Integer &quotient, const Integer &dividend, const Integer &divisor)
{
	PositiveDivide(remainder, quotient, dividend, divisor);

	if (dividend.IsNegative())
	{
		quotient.Negate();
		if (remainder.NotZero())
		{
			--quotient;
			remainder = divisor.AbsoluteValue() - remainder;
		}
	}

	if (divisor.IsNegative())
		quotient.Negate();
}

void Integer::DivideByPowerOf2(Integer &r, Integer &q, const Integer &a, unsigned int n)
{
	q = a;
	q >>= n;

	const unsigned int wordCount = BitsToWords(n);
	if (wordCount <= a.WordCount())
	{
		r.reg.resize(RoundupSize(wordCount));
		CopyWords(r.reg, a.reg, wordCount);
		SetWords(r.reg+wordCount, 0, r.reg.size()-wordCount);
		if (n % WORD_BITS != 0)
			r.reg[wordCount-1] %= (1 << (n % WORD_BITS));
	}
	else
	{
		r.reg.resize(RoundupSize(a.WordCount()));
		CopyWords(r.reg, a.reg, r.reg.size());
	}
	r.sign = POSITIVE;

	if (a.IsNegative() && r.NotZero())
	{
		--q;
		r = Power2(n) - r;
	}
}

Integer Integer::DividedBy(const Integer &b) const
{
	Integer remainder, quotient;
	Integer::Divide(remainder, quotient, *this, b);
	return quotient;
}

Integer Integer::Modulo(const Integer &b) const
{
	Integer remainder, quotient;
	Integer::Divide(remainder, quotient, *this, b);
	return remainder;
}

void Integer::Divide(word &remainder, Integer &quotient, const Integer &dividend, word divisor)
{
	if (!divisor)
		throw Integer::DivideByZero();

	assert(divisor);

	if ((divisor & (divisor-1)) == 0)	// divisor is a power of 2
	{
		quotient = dividend >> (BitPrecision(divisor)-1);
		remainder = dividend.reg[0] & (divisor-1);
		return;
	}

	unsigned int i = dividend.WordCount();
	quotient.reg.CleanNew(RoundupSize(i));
	remainder = 0;
	while (i--)
	{
		quotient.reg[i] = DWord(dividend.reg[i], remainder) / divisor;
		remainder = DWord(dividend.reg[i], remainder) % divisor;
	}

	if (dividend.NotNegative())
		quotient.sign = POSITIVE;
	else
	{
		quotient.sign = NEGATIVE;
		if (remainder)
		{
			--quotient;
			remainder = divisor - remainder;
		}
	}
}

Integer Integer::DividedBy(word b) const
{
	word remainder;
	Integer quotient;
	Integer::Divide(remainder, quotient, *this, b);
	return quotient;
}

word Integer::Modulo(word divisor) const
{
	if (!divisor)
		throw Integer::DivideByZero();

	assert(divisor);

	word remainder;

	if ((divisor & (divisor-1)) == 0)	// divisor is a power of 2
		remainder = reg[0] & (divisor-1);
	else
	{
		unsigned int i = WordCount();

		if (divisor <= 5)
		{
			DWord sum(0, 0);
			while (i--)
				sum += reg[i];
			remainder = sum % divisor;
		}
		else
		{
			remainder = 0;
			while (i--)
				remainder = DWord(reg[i], remainder) % divisor;
		}
	}

	if (IsNegative() && remainder)
		remainder = divisor - remainder;

	return remainder;
}

void Integer::Negate()
{
	if (!!(*this))	// don't flip sign if *this==0
		sign = Sign(1-sign);
}

int Integer::PositiveCompare(const Integer& t) const
{
	unsigned size = WordCount(), tSize = t.WordCount();

	if (size == tSize)
		return CryptoPP::Compare(reg, t.reg, size);
	else
		return size > tSize ? 1 : -1;
}

int Integer::Compare(const Integer& t) const
{
	if (NotNegative())
	{
		if (t.NotNegative())
			return PositiveCompare(t);
		else
			return 1;
	}
	else
	{
		if (t.NotNegative())
			return -1;
		else
			return -PositiveCompare(t);
	}
}

Integer Integer::SquareRoot() const
{
	if (!IsPositive())
		return Zero();

	// overestimate square root
	Integer x, y = Power2((BitCount()+1)/2);
	assert(y*y >= *this);

	do
	{
		x = y;
		y = (x + *this/x) >> 1;
	} while (y<x);

	return x;
}

bool Integer::IsSquare() const
{
	Integer r = SquareRoot();
	return *this == r.Squared();
}

bool Integer::IsUnit() const
{
	return (WordCount() == 1) && (reg[0] == 1);
}

Integer Integer::MultiplicativeInverse() const
{
	return IsUnit() ? *this : Zero();
}

Integer a_times_b_mod_c(const Integer &x, const Integer& y, const Integer& m)
{
	return x*y%m;
}

Integer a_exp_b_mod_c(const Integer &x, const Integer& e, const Integer& m)
{
	ModularArithmetic mr(m);
	return mr.Exponentiate(x, e);
}

Integer Integer::Gcd(const Integer &a, const Integer &b)
{
	return EuclideanDomainOf<Integer>().Gcd(a, b);
}

Integer Integer::InverseMod(const Integer &m) const
{
	assert(m.NotNegative());

	if (IsNegative() || *this>=m)
		return (*this%m).InverseMod(m);

	if (m.IsEven())
	{
		if (!m || IsEven())
			return Zero();	// no inverse
		if (*this == One())
			return One();

		Integer u = m.InverseMod(*this);
		return !u ? Zero() : (m*(*this-u)+1)/(*this);
	}

	SecBlock<word> T(m.reg.size() * 4);
	Integer r((word)0, m.reg.size());
	unsigned k = AlmostInverse(r.reg, T, reg, reg.size(), m.reg, m.reg.size());
	DivideByPower2Mod(r.reg, r.reg, k, m.reg, m.reg.size());
	return r;
}

word Integer::InverseMod(const word mod) const
{
	word g0 = mod, g1 = *this % mod;
	word v0 = 0, v1 = 1;
	word y;

	while (g1)
	{
		if (g1 == 1)
			return v1;
		y = g0 / g1;
		g0 = g0 % g1;
		v0 += y * v1;

		if (!g0)
			break;
		if (g0 == 1)
			return mod-v0;
		y = g1 / g0;
		g1 = g1 % g0;
		v1 += y * v0;
	}
	return 0;
}

// ********************************************************

ModularArithmetic::ModularArithmetic(BufferedTransformation &bt)
{
	BERSequenceDecoder seq(bt);
	OID oid(seq);
	if (oid != ASN1::prime_field())
		BERDecodeError();
	modulus.BERDecode(seq);
	seq.MessageEnd();
	result.reg.resize(modulus.reg.size());
}

void ModularArithmetic::DEREncode(BufferedTransformation &bt) const
{
	DERSequenceEncoder seq(bt);
	ASN1::prime_field().DEREncode(seq);
	modulus.DEREncode(seq);
	seq.MessageEnd();
}

void ModularArithmetic::DEREncodeElement(BufferedTransformation &out, const Element &a) const
{
	a.DEREncodeAsOctetString(out, MaxElementByteLength());
}

void ModularArithmetic::BERDecodeElement(BufferedTransformation &in, Element &a) const
{
	a.BERDecodeAsOctetString(in, MaxElementByteLength());
}

const Integer& ModularArithmetic::Half(const Integer &a) const
{
	if (a.reg.size()==modulus.reg.size())
	{
		CryptoPP::DivideByPower2Mod(result.reg.begin(), a.reg, 1, modulus.reg, a.reg.size());
		return result;
	}
	else
		return result1 = (a.IsEven() ? (a >> 1) : ((a+modulus) >> 1));
}

const Integer& ModularArithmetic::Add(const Integer &a, const Integer &b) const
{
	if (a.reg.size()==modulus.reg.size() && b.reg.size()==modulus.reg.size())
	{
		if (CryptoPP::Add(result.reg.begin(), a.reg, b.reg, a.reg.size())
			|| Compare(result.reg, modulus.reg, a.reg.size()) >= 0)
		{
			CryptoPP::Subtract(result.reg.begin(), result.reg, modulus.reg, a.reg.size());
		}
		return result;
	}
	else
	{
		result1 = a+b;
		if (result1 >= modulus)
			result1 -= modulus;
		return result1;
	}
}

Integer& ModularArithmetic::Accumulate(Integer &a, const Integer &b) const
{
	if (a.reg.size()==modulus.reg.size() && b.reg.size()==modulus.reg.size())
	{
		if (CryptoPP::Add(a.reg, a.reg, b.reg, a.reg.size())
			|| Compare(a.reg, modulus.reg, a.reg.size()) >= 0)
		{
			CryptoPP::Subtract(a.reg, a.reg, modulus.reg, a.reg.size());
		}
	}
	else
	{
		a+=b;
		if (a>=modulus)
			a-=modulus;
	}

	return a;
}

const Integer& ModularArithmetic::Subtract(const Integer &a, const Integer &b) const
{
	if (a.reg.size()==modulus.reg.size() && b.reg.size()==modulus.reg.size())
	{
		if (CryptoPP::Subtract(result.reg.begin(), a.reg, b.reg, a.reg.size()))
			CryptoPP::Add(result.reg.begin(), result.reg, modulus.reg, a.reg.size());
		return result;
	}
	else
	{
		result1 = a-b;
		if (result1.IsNegative())
			result1 += modulus;
		return result1;
	}
}

Integer& ModularArithmetic::Reduce(Integer &a, const Integer &b) const
{
	if (a.reg.size()==modulus.reg.size() && b.reg.size()==modulus.reg.size())
	{
		if (CryptoPP::Subtract(a.reg, a.reg, b.reg, a.reg.size()))
			CryptoPP::Add(a.reg, a.reg, modulus.reg, a.reg.size());
	}
	else
	{
		a-=b;
		if (a.IsNegative())
			a+=modulus;
	}

	return a;
}

const Integer& ModularArithmetic::Inverse(const Integer &a) const
{
	if (!a)
		return a;

	CopyWords(result.reg.begin(), modulus.reg, modulus.reg.size());
	if (CryptoPP::Subtract(result.reg.begin(), result.reg, a.reg, a.reg.size()))
		Decrement(result.reg.begin()+a.reg.size(), 1, modulus.reg.size()-a.reg.size());

	return result;
}

Integer ModularArithmetic::CascadeExponentiate(const Integer &x, const Integer &e1, const Integer &y, const Integer &e2) const
{
	if (modulus.IsOdd())
	{
		MontgomeryRepresentation dr(modulus);
		return dr.ConvertOut(dr.CascadeExponentiate(dr.ConvertIn(x), e1, dr.ConvertIn(y), e2));
	}
	else
		return AbstractRing<Integer>::CascadeExponentiate(x, e1, y, e2);
}

void ModularArithmetic::SimultaneousExponentiate(Integer *results, const Integer &base, const Integer *exponents, unsigned int exponentsCount) const
{
	if (modulus.IsOdd())
	{
		MontgomeryRepresentation dr(modulus);
		dr.SimultaneousExponentiate(results, dr.ConvertIn(base), exponents, exponentsCount);
		for (unsigned int i=0; i<exponentsCount; i++)
			results[i] = dr.ConvertOut(results[i]);
	}
	else
		AbstractRing<Integer>::SimultaneousExponentiate(results, base, exponents, exponentsCount);
}

MontgomeryRepresentation::MontgomeryRepresentation(const Integer &m)	// modulus must be odd
	: ModularArithmetic(m),
	  u((word)0, modulus.reg.size()),
	  workspace(5*modulus.reg.size())
{
	if (!modulus.IsOdd())
		throw InvalidArgument("MontgomeryRepresentation: Montgomery representation requires an odd modulus");

	RecursiveInverseModPower2(u.reg, workspace, modulus.reg, modulus.reg.size());
}

const Integer& MontgomeryRepresentation::Multiply(const Integer &a, const Integer &b) const
{
	word *const T = workspace.begin();
	word *const R = result.reg.begin();
	const unsigned int N = modulus.reg.size();
	assert(a.reg.size()<=N && b.reg.size()<=N);

	AsymmetricMultiply(T, T+2*N, a.reg, a.reg.size(), b.reg, b.reg.size());
	SetWords(T+a.reg.size()+b.reg.size(), 0, 2*N-a.reg.size()-b.reg.size());
	MontgomeryReduce(R, T+2*N, T, modulus.reg, u.reg, N);
	return result;
}

const Integer& MontgomeryRepresentation::Square(const Integer &a) const
{
	word *const T = workspace.begin();
	word *const R = result.reg.begin();
	const unsigned int N = modulus.reg.size();
	assert(a.reg.size()<=N);

	CryptoPP::Square(T, T+2*N, a.reg, a.reg.size());
	SetWords(T+2*a.reg.size(), 0, 2*N-2*a.reg.size());
	MontgomeryReduce(R, T+2*N, T, modulus.reg, u.reg, N);
	return result;
}

Integer MontgomeryRepresentation::ConvertOut(const Integer &a) const
{
	word *const T = workspace.begin();
	word *const R = result.reg.begin();
	const unsigned int N = modulus.reg.size();
	assert(a.reg.size()<=N);

	CopyWords(T, a.reg, a.reg.size());
	SetWords(T+a.reg.size(), 0, 2*N-a.reg.size());
	MontgomeryReduce(R, T+2*N, T, modulus.reg, u.reg, N);
	return result;
}

const Integer& MontgomeryRepresentation::MultiplicativeInverse(const Integer &a) const
{
//	  return (EuclideanMultiplicativeInverse(a, modulus)<<(2*WORD_BITS*modulus.reg.size()))%modulus;
	word *const T = workspace.begin();
	word *const R = result.reg.begin();
	const unsigned int N = modulus.reg.size();
	assert(a.reg.size()<=N);

	CopyWords(T, a.reg, a.reg.size());
	SetWords(T+a.reg.size(), 0, 2*N-a.reg.size());
	MontgomeryReduce(R, T+2*N, T, modulus.reg, u.reg, N);
	unsigned k = AlmostInverse(R, T, R, N, modulus.reg, N);

//	cout << "k=" << k << " N*32=" << 32*N << endl;

	if (k>N*WORD_BITS)
		DivideByPower2Mod(R, R, k-N*WORD_BITS, modulus.reg, N);
	else
		MultiplyByPower2Mod(R, R, N*WORD_BITS-k, modulus.reg, N);

	return result;
}

NAMESPACE_END

#endif
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
// algebra.cpp - written and placed in the public domain by Wei Dai



NAMESPACE_BEGIN(CryptoPP)

template <class T> const T& AbstractGroup<T>::Double(const Element &a) const
{
	return Add(a, a);
}

template <class T> const T& AbstractGroup<T>::Subtract(const Element &a, const Element &b) const
{
	// make copy of a in case Inverse() overwrites it
	Element a1(a);
	return Add(a1, Inverse(b));
}

template <class T> T& AbstractGroup<T>::Accumulate(Element &a, const Element &b) const
{
	return a = Add(a, b);
}

template <class T> T& AbstractGroup<T>::Reduce(Element &a, const Element &b) const
{
	return a = Subtract(a, b);
}

template <class T> const T& AbstractRing<T>::Square(const Element &a) const
{
	return Multiply(a, a);
}

template <class T> const T& AbstractRing<T>::Divide(const Element &a, const Element &b) const
{
	// make copy of a in case MultiplicativeInverse() overwrites it
	Element a1(a);
	return Multiply(a1, MultiplicativeInverse(b));
}

template <class T> const T& AbstractEuclideanDomain<T>::Mod(const Element &a, const Element &b) const
{
	Element q;
	DivisionAlgorithm(result, q, a, b);
	return result;
}

template <class T> const T& AbstractEuclideanDomain<T>::Gcd(const Element &a, const Element &b) const
{
	Element g[3]={b, a};
	unsigned int i0=0, i1=1, i2=2;

	while (!Equal(g[i1], this->Identity()))
	{
		g[i2] = Mod(g[i0], g[i1]);
		unsigned int t = i0; i0 = i1; i1 = i2; i2 = t;
	}

	return result = g[i0];
}

template <class T> T AbstractGroup<T>::ScalarMultiply(const Element &base, const Integer &exponent) const
{
	Element result;
	SimultaneousMultiply(&result, base, &exponent, 1);
	return result;
}

template <class T> T AbstractGroup<T>::CascadeScalarMultiply(const Element &x, const Integer &e1, const Element &y, const Integer &e2) const
{
	const unsigned expLen = STDMAX(e1.BitCount(), e2.BitCount());
	if (expLen==0)
		return Identity();

	const unsigned w = (expLen <= 46 ? 1 : (expLen <= 260 ? 2 : 3));
	const unsigned tableSize = 1<<w;
	std::vector<Element> powerTable(tableSize << w);

	powerTable[1] = x;
	powerTable[tableSize] = y;
	if (w==1)
		powerTable[3] = Add(x,y);
	else
	{
		powerTable[2] = Double(x);
		powerTable[2*tableSize] = Double(y);

		unsigned i, j;

		for (i=3; i<tableSize; i+=2)
			powerTable[i] = Add(powerTable[i-2], powerTable[2]);
		for (i=1; i<tableSize; i+=2)
			for (j=i+tableSize; j<(tableSize<<w); j+=tableSize)
				powerTable[j] = Add(powerTable[j-tableSize], y);

		for (i=3*tableSize; i<(tableSize<<w); i+=2*tableSize)
			powerTable[i] = Add(powerTable[i-2*tableSize], powerTable[2*tableSize]);
		for (i=tableSize; i<(tableSize<<w); i+=2*tableSize)
			for (j=i+2; j<i+tableSize; j+=2)
				powerTable[j] = Add(powerTable[j-1], x);
	}

	Element result;
	unsigned power1 = 0, power2 = 0, prevPosition = expLen-1;
	bool firstTime = true;

	for (int i = expLen-1; i>=0; i--)
	{
		power1 = 2*power1 + e1.GetBit(i);
		power2 = 2*power2 + e2.GetBit(i);

		if (i==0 || 2*power1 >= tableSize || 2*power2 >= tableSize)
		{
			unsigned squaresBefore = prevPosition-i;
			unsigned squaresAfter = 0;
			prevPosition = i;
			while ((power1 || power2) && power1%2 == 0 && power2%2==0)
			{
				power1 /= 2;
				power2 /= 2;
				squaresBefore--;
				squaresAfter++;
			}
			if (firstTime)
			{
				result = powerTable[(power2<<w) + power1];
				firstTime = false;
			}
			else
			{
				while (squaresBefore--)
					result = Double(result);
				if (power1 || power2)
					Accumulate(result, powerTable[(power2<<w) + power1]);
			}
			while (squaresAfter--)
				result = Double(result);
			power1 = power2 = 0;
		}
	}
	return result;
}

struct WindowSlider
{
	WindowSlider(const Integer &Exp, bool FastNegate, unsigned int windowSizeIn=0)
		: exp(Exp), windowModulus(Integer::One()), windowSize(windowSizeIn), windowBegin(0), fastNegate(FastNegate), firstTime(true), finished(false)
	{
		if (windowSize == 0)
		{
			unsigned int expLen = exp.BitCount();
			windowSize = expLen <= 17 ? 1 : (expLen <= 24 ? 2 : (expLen <= 70 ? 3 : (expLen <= 197 ? 4 : (expLen <= 539 ? 5 : (expLen <= 1434 ? 6 : 7)))));
		}
		windowModulus <<= windowSize;
	}

	void FindNextWindow()
	{
		unsigned int expLen = exp.WordCount() * WORD_BITS;
		unsigned int skipCount = firstTime ? 0 : windowSize;
		firstTime = false;
		while (!exp.GetBit(skipCount))
		{
			if (skipCount >= expLen)
			{
				finished = true;
				return;
			}
			skipCount++;
		}

		exp >>= skipCount;
		windowBegin += skipCount;
		expWindow = exp % (1 << windowSize);

		if (fastNegate && exp.GetBit(windowSize))
		{
			negateNext = true;
			expWindow = (1 << windowSize) - expWindow;
			exp += windowModulus;
		}
		else
			negateNext = false;
	}

	Integer exp, windowModulus;
	unsigned int windowSize, windowBegin, expWindow;
	bool fastNegate, negateNext, firstTime, finished;
};

template <class T>
void AbstractGroup<T>::SimultaneousMultiply(T *results, const T &base, const Integer *expBegin, unsigned int expCount) const
{
	std::vector<std::vector<Element> > buckets(expCount);
	std::vector<WindowSlider> exponents;
	exponents.reserve(expCount);
	unsigned int i;

	for (i=0; i<expCount; i++)
	{
		assert(expBegin->NotNegative());
		exponents.push_back(WindowSlider(*expBegin++, InversionIsFast(), 0));
		exponents[i].FindNextWindow();
		buckets[i].resize(1<<(exponents[i].windowSize-1), Identity());
	}

	unsigned int expBitPosition = 0;
	Element g = base;
	bool notDone = true;

	while (notDone)
	{
		notDone = false;
		for (i=0; i<expCount; i++)
		{
			if (!exponents[i].finished && expBitPosition == exponents[i].windowBegin)
			{
				Element &bucket = buckets[i][exponents[i].expWindow/2];
				if (exponents[i].negateNext)
					Accumulate(bucket, Inverse(g));
				else
					Accumulate(bucket, g);
				exponents[i].FindNextWindow();
			}
			notDone = notDone || !exponents[i].finished;
		}

		if (notDone)
		{
			g = Double(g);
			expBitPosition++;
		}
	}

	for (i=0; i<expCount; i++)
	{
		Element &r = *results++;
		r = buckets[i][buckets[i].size()-1];
		if (buckets[i].size() > 1)
		{
			for (int j = buckets[i].size()-2; j >= 1; j--)
			{
				Accumulate(buckets[i][j], buckets[i][j+1]);
				Accumulate(r, buckets[i][j]);
			}
			Accumulate(buckets[i][0], buckets[i][1]);
			r = Add(Double(r), buckets[i][0]);
		}
	}
}

template <class T> T AbstractRing<T>::Exponentiate(const Element &base, const Integer &exponent) const
{
	Element result;
	SimultaneousExponentiate(&result, base, &exponent, 1);
	return result;
}

template <class T> T AbstractRing<T>::CascadeExponentiate(const Element &x, const Integer &e1, const Element &y, const Integer &e2) const
{
	return MultiplicativeGroup().AbstractGroup<T>::CascadeScalarMultiply(x, e1, y, e2);
}

template <class T>
void AbstractRing<T>::SimultaneousExponentiate(T *results, const T &base, const Integer *exponents, unsigned int expCount) const
{
	MultiplicativeGroup().AbstractGroup<T>::SimultaneousMultiply(results, base, exponents, expCount);
}

NAMESPACE_END
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
// queue.cpp - written and placed in the public domain by Wei Dai


#ifndef CRYPTOPP_IMPORTS


NAMESPACE_BEGIN(CryptoPP)

static const unsigned int s_maxAutoNodeSize = 16*1024;

// this class for use by ByteQueue only
class ByteQueueNode
{
public:
	ByteQueueNode(unsigned int maxSize)
		: buf(maxSize)
	{
		m_head = m_tail = 0;
		next = 0;
	}

	inline unsigned int MaxSize() const {return buf.size();}

	inline unsigned int CurrentSize() const
	{
		return m_tail-m_head;
	}

	inline bool UsedUp() const
	{
		return (m_head==MaxSize());
	}

	inline void Clear()
	{
		m_head = m_tail = 0;
	}

	inline unsigned int Put(const byte *begin, unsigned int length)
	{
		unsigned int l = STDMIN(length, MaxSize()-m_tail);
		if (buf+m_tail != begin)
			memcpy(buf+m_tail, begin, l);
		m_tail += l;
		return l;
	}

	inline unsigned int Peek(byte &outByte) const
	{
		if (m_tail==m_head)
			return 0;

		outByte=buf[m_head];
		return 1;
	}

	inline unsigned int Peek(byte *target, unsigned int copyMax) const
	{
		unsigned int len = STDMIN(copyMax, m_tail-m_head);
		memcpy(target, buf+m_head, len);
		return len;
	}

	inline unsigned int CopyTo(BufferedTransformation &target, const std::string &channel=BufferedTransformation::NULL_CHANNEL) const
	{
		unsigned int len = m_tail-m_head;
		target.ChannelPut(channel, buf+m_head, len);
		return len;
	}

	inline unsigned int CopyTo(BufferedTransformation &target, unsigned int copyMax, const std::string &channel=BufferedTransformation::NULL_CHANNEL) const
	{
		unsigned int len = STDMIN(copyMax, m_tail-m_head);
		target.ChannelPut(channel, buf+m_head, len);
		return len;
	}

	inline unsigned int Get(byte &outByte)
	{
		unsigned int len = Peek(outByte);
		m_head += len;
		return len;
	}

	inline unsigned int Get(byte *outString, unsigned int getMax)
	{
		unsigned int len = Peek(outString, getMax);
		m_head += len;
		return len;
	}

	inline unsigned int TransferTo(BufferedTransformation &target, const std::string &channel=BufferedTransformation::NULL_CHANNEL)
	{
		unsigned int len = m_tail-m_head;
		target.ChannelPutModifiable(channel, buf+m_head, len);
		m_head = m_tail;
		return len;
	}

	inline unsigned int TransferTo(BufferedTransformation &target, unsigned int transferMax, const std::string &channel=BufferedTransformation::NULL_CHANNEL)
	{
		unsigned int len = STDMIN(transferMax, m_tail-m_head);
		target.ChannelPutModifiable(channel, buf+m_head, len);
		m_head += len;
		return len;
	}

	inline unsigned int Skip(unsigned int skipMax)
	{
		unsigned int len = STDMIN(skipMax, m_tail-m_head);
		m_head += len;
		return len;
	}

	inline byte operator[](unsigned int i) const
	{
		return buf[m_head+i];
	}

	ByteQueueNode *next;

	SecByteBlock buf;
	unsigned int m_head, m_tail;
};

// ********************************************************

ByteQueue::ByteQueue(unsigned int nodeSize)
	: m_lazyLength(0)
{
	SetNodeSize(nodeSize);
	m_head = m_tail = new ByteQueueNode(m_nodeSize);
}

void ByteQueue::SetNodeSize(unsigned int nodeSize)
{
	m_autoNodeSize = !nodeSize;
	m_nodeSize = m_autoNodeSize ? 256 : nodeSize;
}

ByteQueue::ByteQueue(const ByteQueue &copy)
	: Bufferless<BufferedTransformation>()
{
	CopyFrom(copy);
}

void ByteQueue::CopyFrom(const ByteQueue &copy)
{
	m_lazyLength = 0;
	m_autoNodeSize = copy.m_autoNodeSize;
	m_nodeSize = copy.m_nodeSize;
	m_head = m_tail = new ByteQueueNode(*copy.m_head);

	for (ByteQueueNode *current=copy.m_head->next; current; current=current->next)
	{
		m_tail->next = new ByteQueueNode(*current);
		m_tail = m_tail->next;
	}

	m_tail->next = NULL;

	Put(copy.m_lazyString, copy.m_lazyLength);
}

ByteQueue::~ByteQueue()
{
	Destroy();
}

void ByteQueue::Destroy()
{
	for (ByteQueueNode *next, *current=m_head; current; current=next)
	{
		next=current->next;
		delete current;
	}
}

void ByteQueue::IsolatedInitialize(const NameValuePairs &parameters)
{
	m_nodeSize = parameters.GetIntValueWithDefault("NodeSize", 256);
	Clear();
}

unsigned long ByteQueue::CurrentSize() const
{
	unsigned long size=0;

	for (ByteQueueNode *current=m_head; current; current=current->next)
		size += current->CurrentSize();

	return size + m_lazyLength;
}

bool ByteQueue::IsEmpty() const
{
	return m_head==m_tail && m_head->CurrentSize()==0 && m_lazyLength==0;
}

void ByteQueue::Clear()
{
	for (ByteQueueNode *next, *current=m_head->next; current; current=next)
	{
		next=current->next;
		delete current;
	}

	m_tail = m_head;
	m_head->Clear();
	m_head->next = NULL;
	m_lazyLength = 0;
}

unsigned int ByteQueue::Put2(const byte *inString, unsigned int length, int /* messageEnd */, bool /* blocking */)
{
	if (m_lazyLength > 0)
		FinalizeLazyPut();

	unsigned int len;
	while ((len=m_tail->Put(inString, length)) < length)
	{
		inString += len;
		length -= len;
		if (m_autoNodeSize && m_nodeSize < s_maxAutoNodeSize)
			do
			{
				m_nodeSize *= 2;
			}
			while (m_nodeSize < length && m_nodeSize < s_maxAutoNodeSize);
		m_tail->next = new ByteQueueNode(STDMAX(m_nodeSize, length));
		m_tail = m_tail->next;
	}

	return 0;
}

void ByteQueue::CleanupUsedNodes()
{
	while (m_head != m_tail && m_head->UsedUp())
	{
		ByteQueueNode *temp=m_head;
		m_head=m_head->next;
		delete temp;
	}

	if (m_head->CurrentSize() == 0)
		m_head->Clear();
}

void ByteQueue::LazyPut(const byte *inString, unsigned int size)
{
	if (m_lazyLength > 0)
		FinalizeLazyPut();

	if (inString == m_tail->buf+m_tail->m_tail)
		Put(inString, size);
	else
	{
		m_lazyString = const_cast<byte *>(inString);
		m_lazyLength = size;
		m_lazyStringModifiable = false;
	}
}

void ByteQueue::LazyPutModifiable(byte *inString, unsigned int size)
{
	if (m_lazyLength > 0)
		FinalizeLazyPut();
	m_lazyString = inString;
	m_lazyLength = size;
	m_lazyStringModifiable = true;
}

void ByteQueue::UndoLazyPut(unsigned int size)
{
	if (m_lazyLength < size)
		throw InvalidArgument("ByteQueue: size specified for UndoLazyPut is too large");

	m_lazyLength -= size;
}

void ByteQueue::FinalizeLazyPut()
{
	unsigned int len = m_lazyLength;
	m_lazyLength = 0;
	if (len)
		Put(m_lazyString, len);
}

unsigned int ByteQueue::Get(byte &outByte)
{
	if (m_head->Get(outByte))
	{
		if (m_head->UsedUp())
			CleanupUsedNodes();
		return 1;
	}
	else if (m_lazyLength > 0)
	{
		outByte = *m_lazyString++;
		m_lazyLength--;
		return 1;
	}
	else
		return 0;
}

unsigned int ByteQueue::Get(byte *outString, unsigned int getMax)
{
	ArraySink sink(outString, getMax);
	return TransferTo(sink, getMax);
}

unsigned int ByteQueue::Peek(byte &outByte) const
{
	if (m_head->Peek(outByte))
		return 1;
	else if (m_lazyLength > 0)
	{
		outByte = *m_lazyString;
		return 1;
	}
	else
		return 0;
}

unsigned int ByteQueue::Peek(byte *outString, unsigned int peekMax) const
{
	ArraySink sink(outString, peekMax);
	return CopyTo(sink, peekMax);
}

unsigned int ByteQueue::TransferTo2(BufferedTransformation &target, unsigned long &transferBytes, const std::string &channel, bool blocking)
{
	if (blocking)
	{
		unsigned long bytesLeft = transferBytes;
		for (ByteQueueNode *current=m_head; bytesLeft && current; current=current->next)
			bytesLeft -= current->TransferTo(target, bytesLeft, channel);
		CleanupUsedNodes();

		unsigned int len = (unsigned int)STDMIN(bytesLeft, (unsigned long)m_lazyLength);
		if (len)
		{
			if (m_lazyStringModifiable)
				target.ChannelPutModifiable(channel, m_lazyString, len);
			else
				target.ChannelPut(channel, m_lazyString, len);
			m_lazyString += len;
			m_lazyLength -= len;
			bytesLeft -= len;
		}
		transferBytes -= bytesLeft;
		return 0;
	}
	else
	{
		Walker walker(*this);
		unsigned int blockedBytes = walker.TransferTo2(target, transferBytes, channel, blocking);
		Skip(transferBytes);
		return blockedBytes;
	}
}

unsigned int ByteQueue::CopyRangeTo2(BufferedTransformation &target, unsigned long &begin, unsigned long end, const std::string &channel, bool blocking) const
{
	Walker walker(*this);
	walker.Skip(begin);
	unsigned long transferBytes = end-begin;
	unsigned int blockedBytes = walker.TransferTo2(target, transferBytes, channel, blocking);
	begin += transferBytes;
	return blockedBytes;
}

void ByteQueue::Unget(byte inByte)
{
	Unget(&inByte, 1);
}

void ByteQueue::Unget(const byte *inString, unsigned int length)
{
	unsigned int len = STDMIN(length, m_head->m_head);
	length -= len;
	m_head->m_head -= len;
	memcpy(m_head->buf + m_head->m_head, inString + length, len);

	if (length > 0)
	{
		ByteQueueNode *newHead = new ByteQueueNode(length);
		newHead->next = m_head;
		m_head = newHead;
		m_head->Put(inString, length);
	}
}

const byte * ByteQueue::Spy(unsigned int &contiguousSize) const
{
	contiguousSize = m_head->m_tail - m_head->m_head;
	if (contiguousSize == 0 && m_lazyLength > 0)
	{
		contiguousSize = m_lazyLength;
		return m_lazyString;
	}
	else
		return m_head->buf + m_head->m_head;
}

byte * ByteQueue::CreatePutSpace(unsigned int &size)
{
	if (m_lazyLength > 0)
		FinalizeLazyPut();

	if (m_tail->m_tail == m_tail->MaxSize())
	{
		m_tail->next = new ByteQueueNode(STDMAX(m_nodeSize, size));
		m_tail = m_tail->next;
	}

	size = m_tail->MaxSize() - m_tail->m_tail;
	return m_tail->buf + m_tail->m_tail;
}

ByteQueue & ByteQueue::operator=(const ByteQueue &rhs)
{
	Destroy();
	CopyFrom(rhs);
	return *this;
}

bool ByteQueue::operator==(const ByteQueue &rhs) const
{
	const unsigned long currentSize = CurrentSize();

	if (currentSize != rhs.CurrentSize())
		return false;

	Walker walker1(*this), walker2(rhs);
	byte b1, b2;

	while (walker1.Get(b1) && walker2.Get(b2))
		if (b1 != b2)
			return false;

	return true;
}

byte ByteQueue::operator[](unsigned long i) const
{
	for (ByteQueueNode *current=m_head; current; current=current->next)
	{
		if (i < current->CurrentSize())
			return (*current)[i];
		
		i -= current->CurrentSize();
	}

	assert(i < m_lazyLength);
	return m_lazyString[i];
}

void ByteQueue::swap(ByteQueue &rhs)
{
	std::swap(m_autoNodeSize, rhs.m_autoNodeSize);
	std::swap(m_nodeSize, rhs.m_nodeSize);
	std::swap(m_head, rhs.m_head);
	std::swap(m_tail, rhs.m_tail);
	std::swap(m_lazyString, rhs.m_lazyString);
	std::swap(m_lazyLength, rhs.m_lazyLength);
	std::swap(m_lazyStringModifiable, rhs.m_lazyStringModifiable);
}

// ********************************************************

void ByteQueue::Walker::IsolatedInitialize(const NameValuePairs& /* parameters */)
{
	m_node = m_queue.m_head;
	m_position = 0;
	m_offset = 0;
	m_lazyString = m_queue.m_lazyString;
	m_lazyLength = m_queue.m_lazyLength;
}

unsigned int ByteQueue::Walker::Get(byte &outByte)
{
	ArraySink sink(&outByte, 1);
	return TransferTo(sink, 1);
}

unsigned int ByteQueue::Walker::Get(byte *outString, unsigned int getMax)
{
	ArraySink sink(outString, getMax);
	return TransferTo(sink, getMax);
}

unsigned int ByteQueue::Walker::Peek(byte &outByte) const
{
	ArraySink sink(&outByte, 1);
	return CopyTo(sink, 1);
}

unsigned int ByteQueue::Walker::Peek(byte *outString, unsigned int peekMax) const
{
	ArraySink sink(outString, peekMax);
	return CopyTo(sink, peekMax);
}

unsigned int ByteQueue::Walker::TransferTo2(BufferedTransformation &target, unsigned long &transferBytes, const std::string &channel, bool blocking)
{
	unsigned long bytesLeft = transferBytes;
	unsigned int blockedBytes = 0;

	while (m_node)
	{
		unsigned int len = STDMIN(bytesLeft, (unsigned long)m_node->CurrentSize()-m_offset);
		blockedBytes = target.ChannelPut2(channel, m_node->buf+m_node->m_head+m_offset, len, 0, blocking);

		if (blockedBytes)
			goto done;

		m_position += len;
		bytesLeft -= len;

		if (!bytesLeft)
		{
			m_offset += len;
			goto done;
		}

		m_node = m_node->next;
		m_offset = 0;
	}

	if (bytesLeft && m_lazyLength)
	{
		unsigned int len = (unsigned int)STDMIN(bytesLeft, (unsigned long)m_lazyLength);
		unsigned int BlockedBytes = target.ChannelPut2(channel, m_lazyString, len, 0, blocking);
		if (BlockedBytes)
			goto done;

		m_lazyString += len;
		m_lazyLength -= len;
		bytesLeft -= len;
	}

done:
	transferBytes -= bytesLeft;
	return blockedBytes;
}

unsigned int ByteQueue::Walker::CopyRangeTo2(BufferedTransformation &target, unsigned long &begin, unsigned long end, const std::string &channel, bool blocking) const
{
	Walker walker(*this);
	walker.Skip(begin);
	unsigned long transferBytes = end-begin;
	unsigned int blockedBytes = walker.TransferTo2(target, transferBytes, channel, blocking);
	begin += transferBytes;
	return blockedBytes;
}

NAMESPACE_END

#endif
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
// algparam.cpp - written and placed in the public domain by Wei Dai


#ifndef CRYPTOPP_IMPORTS


NAMESPACE_BEGIN(CryptoPP)

bool (*AssignIntToInteger)(const std::type_info &valueType, void *pInteger, const void *pInt) = NULL;

bool CombinedNameValuePairs::GetVoidValue(const char *name, const std::type_info &valueType, void *pValue) const
{
	if (strcmp(name, "ValueNames") == 0)
		return m_pairs1.GetVoidValue(name, valueType, pValue) && m_pairs2.GetVoidValue(name, valueType, pValue);
	else
		return m_pairs1.GetVoidValue(name, valueType, pValue) || m_pairs2.GetVoidValue(name, valueType, pValue);
}

bool AlgorithmParametersBase::GetVoidValue(const char *name, const std::type_info &valueType, void *pValue) const
{
	if (strcmp(name, "ValueNames") == 0)
	{
		ThrowIfTypeMismatch(name, typeid(std::string), valueType);
		GetParent().GetVoidValue(name, valueType, pValue);
		(*reinterpret_cast<std::string *>(pValue) += m_name) += ";";
		return true;
	}
	else if (strcmp(name, m_name) == 0)
	{
		AssignValue(name, valueType, pValue);
		m_used = true;
		return true;
	}
	else
		return GetParent().GetVoidValue(name, valueType, pValue);
}

NAMESPACE_END

#endif
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
#ifndef CRYPTOPP_MQUEUE_H
#define CRYPTOPP_MQUEUE_H

#include <deque>

NAMESPACE_BEGIN(CryptoPP)

//! Message Queue
class CRYPTOPP_DLL MessageQueue : public AutoSignaling<BufferedTransformation>
{
public:
	MessageQueue(unsigned int nodeSize=256);

	void IsolatedInitialize(const NameValuePairs &parameters)
		{m_queue.IsolatedInitialize(parameters); m_lengths.assign(1, 0U); m_messageCounts.assign(1, 0U);}
	unsigned int Put2(const byte *begin, unsigned int length, int messageEnd, bool /* blocking */)
	{
		m_queue.Put(begin, length);
		m_lengths.back() += length;
		if (messageEnd)
		{
			m_lengths.push_back(0);
			m_messageCounts.back()++;
		}
		return 0;
	}
	bool IsolatedFlush(bool /* hardFlush */, bool /* blocking */) {return false;}
	bool IsolatedMessageSeriesEnd(bool /* blocking */)
		{m_messageCounts.push_back(0); return false;}

	unsigned long MaxRetrievable() const
		{return m_lengths.front();}
	bool AnyRetrievable() const
		{return m_lengths.front() > 0;}

	unsigned int TransferTo2(BufferedTransformation &target, unsigned long &transferBytes, const std::string &channel=NULL_CHANNEL, bool blocking=true);
	unsigned int CopyRangeTo2(BufferedTransformation &target, unsigned long &begin, unsigned long end=ULONG_MAX, const std::string &channel=NULL_CHANNEL, bool blocking=true) const;

	unsigned long TotalBytesRetrievable() const
		{return m_queue.MaxRetrievable();}
	unsigned int NumberOfMessages() const
		{return m_lengths.size()-1;}
	bool GetNextMessage();

	unsigned int NumberOfMessagesInThisSeries() const
		{return m_messageCounts[0];}
	unsigned int NumberOfMessageSeries() const
		{return m_messageCounts.size()-1;}

	unsigned int CopyMessagesTo(BufferedTransformation &target, unsigned int count=UINT_MAX, const std::string &channel=NULL_CHANNEL) const;

	const byte * Spy(unsigned int &contiguousSize) const;

	void swap(MessageQueue &rhs);

private:
	ByteQueue m_queue;
	std::deque<unsigned long> m_lengths, m_messageCounts;
};


NAMESPACE_END

NAMESPACE_BEGIN(std)
template<> inline void swap(CryptoPP::MessageQueue &a, CryptoPP::MessageQueue &b)
{
	a.swap(b);
}
NAMESPACE_END

#endif
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
// mqueue.cpp - written and placed in the public domain by Wei Dai


#ifndef CRYPTOPP_IMPORTS


NAMESPACE_BEGIN(CryptoPP)

MessageQueue::MessageQueue(unsigned int nodeSize)
	: m_queue(nodeSize), m_lengths(1, 0U), m_messageCounts(1, 0U)
{
}

unsigned int MessageQueue::CopyRangeTo2(BufferedTransformation &target, unsigned long &begin, unsigned long end, const std::string &channel, bool blocking) const
{
	if (begin >= MaxRetrievable())
		return 0;

	return m_queue.CopyRangeTo2(target, begin, STDMIN(MaxRetrievable(), end), channel, blocking);
}

unsigned int MessageQueue::TransferTo2(BufferedTransformation &target, unsigned long &transferBytes, const std::string &channel, bool blocking)
{
	transferBytes = STDMIN(MaxRetrievable(), transferBytes);
	unsigned int blockedBytes = m_queue.TransferTo2(target, transferBytes, channel, blocking);
	m_lengths.front() -= transferBytes;
	return blockedBytes;
}

bool MessageQueue::GetNextMessage()
{
	if (NumberOfMessages() > 0 && !AnyRetrievable())
	{
		m_lengths.pop_front();
		if (m_messageCounts[0] == 0 && m_messageCounts.size() > 1)
			m_messageCounts.pop_front();
		return true;
	}
	else
		return false;
}

unsigned int MessageQueue::CopyMessagesTo(BufferedTransformation &target, unsigned int count, const std::string &channel) const
{
	ByteQueue::Walker walker(m_queue);
	std::deque<unsigned long>::const_iterator it = m_lengths.begin();
	unsigned int i;
	for (i=0; i<count && it != --m_lengths.end(); ++i, ++it)
	{
		walker.TransferTo(target, *it, channel);
		if (GetAutoSignalPropagation())
			target.ChannelMessageEnd(channel, GetAutoSignalPropagation()-1);
	}
	return i;
}

void MessageQueue::swap(MessageQueue &rhs)
{
	m_queue.swap(rhs.m_queue);
	m_lengths.swap(rhs.m_lengths);
}

const byte * MessageQueue::Spy(unsigned int &contiguousSize) const
{
	const byte *result = m_queue.Spy(contiguousSize);
	contiguousSize = (unsigned int)STDMIN((unsigned long)contiguousSize, MaxRetrievable());
	return result;
}

NAMESPACE_END

#endif
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
// filters.cpp - written and placed in the public domain by Wei Dai


#ifndef CRYPTOPP_IMPORTS


NAMESPACE_BEGIN(CryptoPP)

Filter::Filter(BufferedTransformation *attachment)
	: m_attachment(attachment), m_continueAt(0)
{
}

BufferedTransformation * Filter::NewDefaultAttachment() const
{
	return new MessageQueue;
}

BufferedTransformation * Filter::AttachedTransformation()
{
	if (m_attachment.get() == NULL)
		m_attachment.reset(NewDefaultAttachment());
	return m_attachment.get();
}

const BufferedTransformation *Filter::AttachedTransformation() const
{
	if (m_attachment.get() == NULL)
		const_cast<Filter *>(this)->m_attachment.reset(NewDefaultAttachment());
	return m_attachment.get();
}

void Filter::Detach(BufferedTransformation *newOut)
{
	m_attachment.reset(newOut);
}

void Filter::Insert(Filter *filter)
{
	filter->m_attachment.reset(m_attachment.release());
	m_attachment.reset(filter);
}

unsigned int Filter::CopyRangeTo2(BufferedTransformation &target, unsigned long &begin, unsigned long end, const std::string &channel, bool blocking) const
{
	return AttachedTransformation()->CopyRangeTo2(target, begin, end, channel, blocking);
}

unsigned int Filter::TransferTo2(BufferedTransformation &target, unsigned long &transferBytes, const std::string &channel, bool blocking)
{
	return AttachedTransformation()->TransferTo2(target, transferBytes, channel, blocking);
}

void Filter::Initialize(const NameValuePairs &parameters, int propagation)
{
	m_continueAt = 0;
	IsolatedInitialize(parameters);
	PropagateInitialize(parameters, propagation);
}

bool Filter::Flush(bool hardFlush, int propagation, bool blocking)
{
	switch (m_continueAt)
	{
	case 0:
		if (IsolatedFlush(hardFlush, blocking))
			return true;
	case 1:
		if (OutputFlush(1, hardFlush, propagation, blocking))
			return true;
	}
	return false;
}

bool Filter::MessageSeriesEnd(int propagation, bool blocking)
{
	switch (m_continueAt)
	{
	case 0:
		if (IsolatedMessageSeriesEnd(blocking))
			return true;
	case 1:
		if (ShouldPropagateMessageSeriesEnd() && OutputMessageSeriesEnd(1, propagation, blocking))
			return true;
	}
	return false;
}

void Filter::PropagateInitialize(const NameValuePairs &parameters, int propagation)
{
	if (propagation)
		AttachedTransformation()->Initialize(parameters, propagation-1);
}

unsigned int Filter::OutputModifiable(int outputSite, byte *inString, unsigned int length, int messageEnd, bool blocking, const std::string& /* channel */)
{
	if (messageEnd)
		messageEnd--;
	unsigned int result = AttachedTransformation()->PutModifiable2(inString, length, messageEnd, blocking);
	m_continueAt = result ? outputSite : 0;
	return result;
}

unsigned int Filter::Output(int outputSite, const byte *inString, unsigned int length, int messageEnd, bool blocking, const std::string& /* channel */)
{
	if (messageEnd)
		messageEnd--;
	unsigned int result = AttachedTransformation()->Put2(inString, length, messageEnd, blocking);
	m_continueAt = result ? outputSite : 0;
	return result;
}

bool Filter::OutputFlush(int outputSite, bool hardFlush, int propagation, bool blocking, const std::string &channel)
{
	if (propagation && AttachedTransformation()->ChannelFlush(channel, hardFlush, propagation-1, blocking))
	{
		m_continueAt = outputSite;
		return true;
	}
	m_continueAt = 0;
	return false;
}

bool Filter::OutputMessageSeriesEnd(int outputSite, int propagation, bool blocking, const std::string &channel)
{
	if (propagation && AttachedTransformation()->ChannelMessageSeriesEnd(channel, propagation-1, blocking))
	{
		m_continueAt = outputSite;
		return true;
	}
	m_continueAt = 0;
	return false;
}

// *************************************************************

void FilterWithBufferedInput::BlockQueue::ResetQueue(unsigned int blockSize, unsigned int maxBlocks)
{
	m_buffer.New(blockSize * maxBlocks);
	m_blockSize = blockSize;
	m_maxBlocks = maxBlocks;
	m_size = 0;
	m_begin = m_buffer;
}

byte *FilterWithBufferedInput::BlockQueue::GetBlock()
{
	if (m_size >= m_blockSize)
	{
		byte *ptr = m_begin;
		if ((m_begin+=m_blockSize) == m_buffer.end())
			m_begin = m_buffer;
		m_size -= m_blockSize;
		return ptr;
	}
	else
		return NULL;
}

byte *FilterWithBufferedInput::BlockQueue::GetContigousBlocks(unsigned int &numberOfBytes)
{
	numberOfBytes = STDMIN(numberOfBytes, STDMIN((unsigned int)(m_buffer.end()-m_begin), m_size));
	byte *ptr = m_begin;
	m_begin += numberOfBytes;
	m_size -= numberOfBytes;
	if (m_size == 0 || m_begin == m_buffer.end())
		m_begin = m_buffer;
	return ptr;
}

unsigned int FilterWithBufferedInput::BlockQueue::GetAll(byte *outString)
{
	unsigned int size = m_size;
	unsigned int numberOfBytes = m_maxBlocks*m_blockSize;
	const byte *ptr = GetContigousBlocks(numberOfBytes);
	memcpy(outString, ptr, numberOfBytes);
	memcpy(outString+numberOfBytes, m_begin, m_size);
	m_size = 0;
	return size;
}

void FilterWithBufferedInput::BlockQueue::Put(const byte *inString, unsigned int length)
{
	assert(m_size + length <= m_buffer.size());
	byte *end = (m_size < (unsigned int)(m_buffer.end()-m_begin)) ? m_begin + m_size : m_begin + m_size - m_buffer.size();
	unsigned int len = STDMIN(length, (unsigned int)(m_buffer.end()-end));
	memcpy(end, inString, len);
	if (len < length)
		memcpy(m_buffer, inString+len, length-len);
	m_size += length;
}

FilterWithBufferedInput::FilterWithBufferedInput(BufferedTransformation *attachment)
	: Filter(attachment)
{
}

FilterWithBufferedInput::FilterWithBufferedInput(unsigned int firstSize, unsigned int blockSize, unsigned int lastSize, BufferedTransformation *attachment)
	: Filter(attachment), m_firstSize(firstSize), m_blockSize(blockSize), m_lastSize(lastSize)
	, m_firstInputDone(false)
{
	if (m_blockSize < 1)
		throw InvalidArgument("FilterWithBufferedInput: invalid buffer size");

	m_queue.ResetQueue(1, m_firstSize);
}

void FilterWithBufferedInput::IsolatedInitialize(const NameValuePairs &parameters)
{
	InitializeDerivedAndReturnNewSizes(parameters, m_firstSize, m_blockSize, m_lastSize);
	if (m_blockSize < 1)
		throw InvalidArgument("FilterWithBufferedInput: invalid buffer size");
	m_queue.ResetQueue(1, m_firstSize);
	m_firstInputDone = false;
}

bool FilterWithBufferedInput::IsolatedFlush(bool hardFlush, bool blocking)
{
	if (!blocking)
		throw BlockingInputOnly("FilterWithBufferedInput");
	
	if (hardFlush)
		ForceNextPut();
	FlushDerived();
	
	return false;
}

unsigned int FilterWithBufferedInput::PutMaybeModifiable(byte *inString, unsigned int length, int messageEnd, bool blocking, bool modifiable)
{
	if (!blocking)
		throw BlockingInputOnly("FilterWithBufferedInput");

	if (length != 0)
	{
		unsigned int newLength = m_queue.CurrentSize() + length;

		if (!m_firstInputDone && newLength >= m_firstSize)
		{
			unsigned int len = m_firstSize - m_queue.CurrentSize();
			m_queue.Put(inString, len);
			FirstPut(m_queue.GetContigousBlocks(m_firstSize));
			assert(m_queue.CurrentSize() == 0);
			m_queue.ResetQueue(m_blockSize, (2*m_blockSize+m_lastSize-2)/m_blockSize);

			inString += len;
			newLength -= m_firstSize;
			m_firstInputDone = true;
		}

		if (m_firstInputDone)
		{
			if (m_blockSize == 1)
			{
				while (newLength > m_lastSize && m_queue.CurrentSize() > 0)
				{
					unsigned int len = newLength - m_lastSize;
					byte *ptr = m_queue.GetContigousBlocks(len);
					NextPutModifiable(ptr, len);
					newLength -= len;
				}

				if (newLength > m_lastSize)
				{
					unsigned int len = newLength - m_lastSize;
					NextPutMaybeModifiable(inString, len, modifiable);
					inString += len;
					newLength -= len;
				}
			}
			else
			{
				while (newLength >= m_blockSize + m_lastSize && m_queue.CurrentSize() >= m_blockSize)
				{
					NextPutModifiable(m_queue.GetBlock(), m_blockSize);
					newLength -= m_blockSize;
				}

				if (newLength >= m_blockSize + m_lastSize && m_queue.CurrentSize() > 0)
				{
					assert(m_queue.CurrentSize() < m_blockSize);
					unsigned int len = m_blockSize - m_queue.CurrentSize();
					m_queue.Put(inString, len);
					inString += len;
					NextPutModifiable(m_queue.GetBlock(), m_blockSize);
					newLength -= m_blockSize;
				}

				if (newLength >= m_blockSize + m_lastSize)
				{
					unsigned int len = RoundDownToMultipleOf(newLength - m_lastSize, m_blockSize);
					NextPutMaybeModifiable(inString, len, modifiable);
					inString += len;
					newLength -= len;
				}
			}
		}

		m_queue.Put(inString, newLength - m_queue.CurrentSize());
	}

	if (messageEnd)
	{
		if (!m_firstInputDone && m_firstSize==0)
			FirstPut(NULL);

		SecByteBlock temp(m_queue.CurrentSize());
		m_queue.GetAll(temp);
		LastPut(temp, temp.size());

		m_firstInputDone = false;
		m_queue.ResetQueue(1, m_firstSize);

		Output(1, NULL, 0, messageEnd, blocking);
	}
	return 0;
}

void FilterWithBufferedInput::ForceNextPut()
{
	if (!m_firstInputDone)
		return;
	
	if (m_blockSize > 1)
	{
		while (m_queue.CurrentSize() >= m_blockSize)
			NextPutModifiable(m_queue.GetBlock(), m_blockSize);
	}
	else
	{
		unsigned int len;
		while ((len = m_queue.CurrentSize()) > 0)
			NextPutModifiable(m_queue.GetContigousBlocks(len), len);
	}
}

void FilterWithBufferedInput::NextPutMultiple(const byte *inString, unsigned int length)
{
	assert(m_blockSize > 1);	// m_blockSize = 1 should always override this function
	while (length > 0)
	{
		assert(length >= m_blockSize);
		NextPutSingle(inString);
		inString += m_blockSize;
		length -= m_blockSize;
	}
}

// *************************************************************

ProxyFilter::ProxyFilter(BufferedTransformation *filter, unsigned int firstSize, unsigned int lastSize, BufferedTransformation *attachment)
	: FilterWithBufferedInput(firstSize, 1, lastSize, attachment), m_filter(filter)
{
	if (m_filter.get())
		m_filter->Attach(new OutputProxy(*this, false));
}

bool ProxyFilter::IsolatedFlush(bool hardFlush, bool blocking)
{
	return m_filter.get() ? m_filter->Flush(hardFlush, -1, blocking) : false;
}

void ProxyFilter::SetFilter(Filter *filter)
{
	m_filter.reset(filter);
	if (filter)
	{
		OutputProxy *proxy;
		std::auto_ptr<OutputProxy> temp(proxy = new OutputProxy(*this, false));
		m_filter->TransferAllTo(*proxy);
		m_filter->Attach(temp.release());
	}
}

void ProxyFilter::NextPutMultiple(const byte *s, unsigned int len)
{
	if (m_filter.get())
		m_filter->Put(s, len);
}

void ProxyFilter::NextPutModifiable(byte *s, unsigned int len)
{
	if (m_filter.get())
		m_filter->PutModifiable(s, len);
}

// *************************************************************

unsigned int ArraySink::Put2(const byte *begin, unsigned int length, int /* messageEnd */, bool /* blocking */)
{
	memcpy(m_buf+m_total, begin, STDMIN(length, SaturatingSubtract(m_size, m_total)));
	m_total += length;
	return 0;
}

byte * ArraySink::CreatePutSpace(unsigned int &size)
{
	size = m_size - m_total;
	return m_buf + m_total;
}

void ArraySink::IsolatedInitialize(const NameValuePairs &parameters)
{
	ByteArrayParameter array;
	if (!parameters.GetValue(Name::OutputBuffer(), array))
		throw InvalidArgument("ArraySink: missing OutputBuffer argument");
	m_buf = array.begin();
	m_size = array.size();
	m_total = 0;
}

unsigned int ArrayXorSink::Put2(const byte *begin, unsigned int length, int /* messageEnd */, bool /* blocking */)
{
	xorbuf(m_buf+m_total, begin, STDMIN(length, SaturatingSubtract(m_size, m_total)));
	m_total += length;
	return 0;
}

// *************************************************************

void HashFilter::IsolatedInitialize(const NameValuePairs &parameters)
{
	m_putMessage = parameters.GetValueWithDefault(Name::PutMessage(), false);
	m_hashModule.Restart();
}

unsigned int HashFilter::Put2(const byte *inString, unsigned int length, int messageEnd, bool blocking)
{
	FILTER_BEGIN;
	m_hashModule.Update(inString, length);
	if (m_putMessage)
		FILTER_OUTPUT(1, inString, length, 0);
	if (messageEnd)
	{
		{
			unsigned int size, digestSize = m_hashModule.DigestSize();
			m_space = HelpCreatePutSpace(*AttachedTransformation(), NULL_CHANNEL, digestSize, digestSize, size = digestSize);
			m_hashModule.Final(m_space);
		}
		FILTER_OUTPUT(2, m_space, m_hashModule.DigestSize(), messageEnd);
	}
	FILTER_END_NO_MESSAGE_END;
}

// *************************************************************

unsigned int Source::PumpAll2(bool blocking)
{
	// TODO: switch length type
	unsigned long i = UINT_MAX;
	RETURN_IF_NONZERO(Pump2(i, blocking));
	unsigned int j = UINT_MAX;
	return PumpMessages2(j, blocking);
}

bool Store::GetNextMessage()
{
	if (!m_messageEnd && !AnyRetrievable())
	{
		m_messageEnd=true;
		return true;
	}
	else
		return false;
}

unsigned int Store::CopyMessagesTo(BufferedTransformation &target, unsigned int count, const std::string &channel) const
{
	if (m_messageEnd || count == 0)
		return 0;
	else
	{
		CopyTo(target, ULONG_MAX, channel);
		if (GetAutoSignalPropagation())
			target.ChannelMessageEnd(channel, GetAutoSignalPropagation()-1);
		return 1;
	}
}

void StringStore::StoreInitialize(const NameValuePairs &parameters)
{
	ConstByteArrayParameter array;
	if (!parameters.GetValue(Name::InputBuffer(), array))
		throw InvalidArgument("StringStore: missing InputBuffer argument");
	m_store = array.begin();
	m_length = array.size();
	m_count = 0;
}

unsigned int StringStore::TransferTo2(BufferedTransformation &target, unsigned long &transferBytes, const std::string &channel, bool blocking)
{
	unsigned long position = 0;
	unsigned int blockedBytes = CopyRangeTo2(target, position, transferBytes, channel, blocking);
	m_count += position;
	transferBytes = position;
	return blockedBytes;
}

unsigned int StringStore::CopyRangeTo2(BufferedTransformation &target, unsigned long &begin, unsigned long end, const std::string &channel, bool blocking) const
{
	unsigned int i = (unsigned int)STDMIN((unsigned long)m_count+begin, (unsigned long)m_length);
	unsigned int len = (unsigned int)STDMIN((unsigned long)m_length-i, end-begin);
	unsigned int blockedBytes = target.ChannelPut2(channel, m_store+i, len, 0, blocking);
	if (!blockedBytes)
		begin += len;
	return blockedBytes;
}


NAMESPACE_END

#endif
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
// pubkey.cpp - written and placed in the public domain by Wei Dai


#ifndef CRYPTOPP_IMPORTS


NAMESPACE_BEGIN(CryptoPP)

void P1363_MGF1KDF2_Common(HashTransformation &hash, byte *output, unsigned int outputLength, const byte *input, unsigned int inputLength, const byte *derivationParams, unsigned int derivationParamsLength, bool mask, unsigned int counterStart)
{
	ArraySink *sink;
	HashFilter filter(hash, sink = mask ? new ArrayXorSink(output, outputLength) : new ArraySink(output, outputLength));
	word32 counter = counterStart;
	while (sink->AvailableSize() > 0)
	{
		filter.Put(input, inputLength);
		filter.PutWord32(counter++);
		filter.Put(derivationParams, derivationParamsLength);
		filter.MessageEnd();
	}
}

bool PK_DeterministicSignatureMessageEncodingMethod::VerifyMessageRepresentative(
	HashTransformation &hash, HashIdentifier hashIdentifier, bool messageEmpty,
	byte *representative, unsigned int representativeBitLength) const
{
	SecByteBlock computedRepresentative(BitsToBytes(representativeBitLength));
	ComputeMessageRepresentative(NullRNG(), NULL, 0, hash, hashIdentifier, messageEmpty, computedRepresentative, representativeBitLength);
	return memcmp(representative, computedRepresentative, computedRepresentative.size()) == 0;
}

void TF_SignerBase::InputRecoverableMessage(PK_MessageAccumulator &messageAccumulator, const byte *recoverableMessage, unsigned int recoverableMessageLength) const
{
	PK_MessageAccumulatorBase &ma = static_cast<PK_MessageAccumulatorBase &>(messageAccumulator);
	const MessageEncodingInterface &mei = GetMessageEncodingInterface();
	unsigned int maxRecoverableLength = mei.MaxRecoverableLength(MessageRepresentativeBitLength(), GetHashIdentifier().second, ma.AccessHash().DigestSize());

	if (maxRecoverableLength == 0)
		{throw NotImplemented("TF_SignerBase: this algorithm does not support messsage recovery or the key is too short");}
	if (recoverableMessageLength > maxRecoverableLength)
		throw InvalidArgument("TF_SignerBase: the recoverable message part is too long for the given key and algorithm");

	ma.m_recoverableMessage.Assign(recoverableMessage, recoverableMessageLength);
	mei.ProcessRecoverableMessage(
		ma.AccessHash(), 
		recoverableMessage, recoverableMessageLength,
		NULL, 0, ma.m_semisignature);
}

unsigned int TF_SignerBase::SignAndRestart(RandomNumberGenerator &rng, PK_MessageAccumulator &messageAccumulator, byte *signature, bool /* restart */) const
{
	PK_MessageAccumulatorBase &ma = static_cast<PK_MessageAccumulatorBase &>(messageAccumulator);
	SecByteBlock representative(MessageRepresentativeLength());
	GetMessageEncodingInterface().ComputeMessageRepresentative(rng, 
		ma.m_recoverableMessage, ma.m_recoverableMessage.size(), 
		ma.AccessHash(), GetHashIdentifier(), ma.m_empty,
		representative, MessageRepresentativeBitLength());
	ma.m_empty = true;

	Integer r(representative, representative.size());
	unsigned int signatureLength = SignatureLength();
	GetTrapdoorFunctionInterface().CalculateRandomizedInverse(rng, r).Encode(signature, signatureLength);
	return signatureLength;
}

void TF_VerifierBase::InputSignature(PK_MessageAccumulator &messageAccumulator, const byte *signature, unsigned int signatureLength) const
{
	PK_MessageAccumulatorBase &ma = static_cast<PK_MessageAccumulatorBase &>(messageAccumulator);
	ma.m_representative.New(MessageRepresentativeLength());
	Integer x = GetTrapdoorFunctionInterface().ApplyFunction(Integer(signature, signatureLength));
	if (x.BitCount() > MessageRepresentativeBitLength())
		x = Integer::Zero();	// don't return false here to prevent timing attack
	x.Encode(ma.m_representative, ma.m_representative.size());
}

bool TF_VerifierBase::VerifyAndRestart(PK_MessageAccumulator &messageAccumulator) const
{
	PK_MessageAccumulatorBase &ma = static_cast<PK_MessageAccumulatorBase &>(messageAccumulator);
	bool result = GetMessageEncodingInterface().VerifyMessageRepresentative(
		ma.AccessHash(), GetHashIdentifier(), ma.m_empty, ma.m_representative, MessageRepresentativeBitLength());
	ma.m_empty = true;
	return result;
}

DecodingResult TF_VerifierBase::RecoverAndRestart(byte *recoveredMessage, PK_MessageAccumulator &messageAccumulator) const
{
	PK_MessageAccumulatorBase &ma = static_cast<PK_MessageAccumulatorBase &>(messageAccumulator);
	DecodingResult result = GetMessageEncodingInterface().RecoverMessageFromRepresentative(
		ma.AccessHash(), GetHashIdentifier(), ma.m_empty, ma.m_representative, MessageRepresentativeBitLength(), recoveredMessage);
	ma.m_empty = true;
	return result;
}

NAMESPACE_END

#endif
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
// iterhash.cpp - written and placed in the public domain by Wei Dai


NAMESPACE_BEGIN(CryptoPP)

template <class T, class BASE> void IteratedHashBase<T, BASE>::Update(const byte *input, unsigned int len)
{
	HashWordType tmp = m_countLo;
	if ((m_countLo = tmp + len) < tmp)
		m_countHi++;             // carry from low to high
	m_countHi += SafeRightShift<8*sizeof(HashWordType)>(len);

	unsigned int blockSize = BlockSize();
	unsigned int num = ModPowerOf2(tmp, blockSize);

	if (num != 0)	// process left over data
	{
		if ((num+len) >= blockSize)
		{
			memcpy((byte *)m_data.begin()+num, input, blockSize-num);
			HashBlock(m_data);
			input += (blockSize-num);
			len-=(blockSize - num);
			num=0;
			// drop through and do the rest
		}
		else
		{
			memcpy((byte *)m_data.begin()+num, input, len);
			return;
		}
	}

	// now process the input data in blocks of blockSize bytes and save the leftovers to m_data
	if (len >= blockSize)
	{
		if (input == (byte *)m_data.begin())
		{
			assert(len == blockSize);
			HashBlock(m_data);
			return;
		}
		else if (IsAligned<T>(input))
		{
			unsigned int leftOver = HashMultipleBlocks((T *)input, len);
			input += (len - leftOver);
			len = leftOver;
		}
		else
			do
			{   // copy input first if it's not aligned correctly
				memcpy(m_data, input, blockSize);
				HashBlock(m_data);
				input+=blockSize;
				len-=blockSize;
			} while (len >= blockSize);
	}

	memcpy(m_data, input, len);
}

template <class T, class BASE> byte * IteratedHashBase<T, BASE>::CreateUpdateSpace(unsigned int &size)
{
	unsigned int blockSize = BlockSize();
	unsigned int num = ModPowerOf2(m_countLo, blockSize);
	size = blockSize - num;
	return (byte *)m_data.begin() + num;
}

template <class T, class BASE> unsigned int IteratedHashBase<T, BASE>::HashMultipleBlocks(const T *input, unsigned int length)
{
	unsigned int blockSize = BlockSize();
	do
	{
		HashBlock(input);
		input += blockSize/sizeof(T);
		length -= blockSize;
	}
	while (length >= blockSize);
	return length;
}

template <class T, class BASE> void IteratedHashBase<T, BASE>::PadLastBlock(unsigned int lastBlockSize, byte padFirst)
{
	unsigned int blockSize = BlockSize();
	unsigned int num = ModPowerOf2(m_countLo, blockSize);
	((byte *)m_data.begin())[num++]=padFirst;
	if (num <= lastBlockSize)
		memset((byte *)m_data.begin()+num, 0, lastBlockSize-num);
	else
	{
		memset((byte *)m_data.begin()+num, 0, blockSize-num);
		HashBlock(m_data);
		memset(m_data, 0, lastBlockSize);
	}
}

template <class T, class BASE> void IteratedHashBase<T, BASE>::Restart()
{
	m_countLo = m_countHi = 0;
	Init();
}

NAMESPACE_END
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
// sha.cpp - modified by Wei Dai from Steve Reid's public domain sha1.c

// Steve Reid implemented SHA-1. Wei Dai implemented SHA-2.
// Both are in the public domain.


NAMESPACE_BEGIN(CryptoPP)

// start of Steve Reid's code

#define blk0(i) (W[i] = data[i])
#define blk1(i) (W[i&15] = rotlFixed(W[(i+13)&15]^W[(i+8)&15]^W[(i+2)&15]^W[i&15],1))

#ifndef CRYPTOPP_IMPORTS

void SHA::InitState(HashWordType *state)
{
	state[0] = 0x67452301L;
	state[1] = 0xEFCDAB89L;
	state[2] = 0x98BADCFEL;
	state[3] = 0x10325476L;
	state[4] = 0xC3D2E1F0L;
}

#define f1(x,y,z) (z^(x&(y^z)))
#define f2(x,y,z) (x^y^z)
#define f3(x,y,z) ((x&y)|(z&(x|y)))
#define f4(x,y,z) (x^y^z)

/* (R0+R1), R2, R3, R4 are the different operations used in SHA1 */
#define R0(v,w,x,y,z,i) z+=f1(w,x,y)+blk0(i)+0x5A827999+rotlFixed(v,5);w=rotlFixed(w,30);
#define R1(v,w,x,y,z,i) z+=f1(w,x,y)+blk1(i)+0x5A827999+rotlFixed(v,5);w=rotlFixed(w,30);
#define R2(v,w,x,y,z,i) z+=f2(w,x,y)+blk1(i)+0x6ED9EBA1+rotlFixed(v,5);w=rotlFixed(w,30);
#define R3(v,w,x,y,z,i) z+=f3(w,x,y)+blk1(i)+0x8F1BBCDC+rotlFixed(v,5);w=rotlFixed(w,30);
#define R4(v,w,x,y,z,i) z+=f4(w,x,y)+blk1(i)+0xCA62C1D6+rotlFixed(v,5);w=rotlFixed(w,30);

void SHA::Transform(word32 *state, const word32 *data)
{
	word32 W[16];
    /* Copy context->state[] to working vars */
    word32 a = state[0];
    word32 b = state[1];
    word32 c = state[2];
    word32 d = state[3];
    word32 e = state[4];
    /* 4 rounds of 20 operations each. Loop unrolled. */
    R0(a,b,c,d,e, 0); R0(e,a,b,c,d, 1); R0(d,e,a,b,c, 2); R0(c,d,e,a,b, 3);
    R0(b,c,d,e,a, 4); R0(a,b,c,d,e, 5); R0(e,a,b,c,d, 6); R0(d,e,a,b,c, 7);
    R0(c,d,e,a,b, 8); R0(b,c,d,e,a, 9); R0(a,b,c,d,e,10); R0(e,a,b,c,d,11);
    R0(d,e,a,b,c,12); R0(c,d,e,a,b,13); R0(b,c,d,e,a,14); R0(a,b,c,d,e,15);
    R1(e,a,b,c,d,16); R1(d,e,a,b,c,17); R1(c,d,e,a,b,18); R1(b,c,d,e,a,19);
    R2(a,b,c,d,e,20); R2(e,a,b,c,d,21); R2(d,e,a,b,c,22); R2(c,d,e,a,b,23);
    R2(b,c,d,e,a,24); R2(a,b,c,d,e,25); R2(e,a,b,c,d,26); R2(d,e,a,b,c,27);
    R2(c,d,e,a,b,28); R2(b,c,d,e,a,29); R2(a,b,c,d,e,30); R2(e,a,b,c,d,31);
    R2(d,e,a,b,c,32); R2(c,d,e,a,b,33); R2(b,c,d,e,a,34); R2(a,b,c,d,e,35);
    R2(e,a,b,c,d,36); R2(d,e,a,b,c,37); R2(c,d,e,a,b,38); R2(b,c,d,e,a,39);
    R3(a,b,c,d,e,40); R3(e,a,b,c,d,41); R3(d,e,a,b,c,42); R3(c,d,e,a,b,43);
    R3(b,c,d,e,a,44); R3(a,b,c,d,e,45); R3(e,a,b,c,d,46); R3(d,e,a,b,c,47);
    R3(c,d,e,a,b,48); R3(b,c,d,e,a,49); R3(a,b,c,d,e,50); R3(e,a,b,c,d,51);
    R3(d,e,a,b,c,52); R3(c,d,e,a,b,53); R3(b,c,d,e,a,54); R3(a,b,c,d,e,55);
    R3(e,a,b,c,d,56); R3(d,e,a,b,c,57); R3(c,d,e,a,b,58); R3(b,c,d,e,a,59);
    R4(a,b,c,d,e,60); R4(e,a,b,c,d,61); R4(d,e,a,b,c,62); R4(c,d,e,a,b,63);
    R4(b,c,d,e,a,64); R4(a,b,c,d,e,65); R4(e,a,b,c,d,66); R4(d,e,a,b,c,67);
    R4(c,d,e,a,b,68); R4(b,c,d,e,a,69); R4(a,b,c,d,e,70); R4(e,a,b,c,d,71);
    R4(d,e,a,b,c,72); R4(c,d,e,a,b,73); R4(b,c,d,e,a,74); R4(a,b,c,d,e,75);
    R4(e,a,b,c,d,76); R4(d,e,a,b,c,77); R4(c,d,e,a,b,78); R4(b,c,d,e,a,79);
    /* Add the working vars back into context.state[] */
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
    /* Wipe variables */
    a = b = c = d = e = 0;
	memset(W, 0, sizeof(W));
}

#endif	// #ifndef CRYPTOPP_IMPORTS

// end of Steve Reid's code

// *************************************************************

NAMESPACE_END
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
// pkcspad.cpp - written and placed in the public domain by Wei Dai



NAMESPACE_BEGIN(CryptoPP)

template<> const byte PKCS_DigestDecoration<SHA>::decoration[] = {0x30,0x21,0x30,0x09,0x06,0x05,0x2B,0x0E,0x03,0x02,0x1A,0x05,0x00,0x04,0x14};
template<> const unsigned int PKCS_DigestDecoration<SHA>::length = sizeof(PKCS_DigestDecoration<SHA>::decoration);

unsigned int PKCS_EncryptionPaddingScheme::MaxUnpaddedLength(unsigned int paddedLength) const
{
	return SaturatingSubtract(paddedLength/8, 10U);
}

void PKCS_EncryptionPaddingScheme::Pad(RandomNumberGenerator &rng, const byte *input, unsigned int inputLen, byte *pkcsBlock, unsigned int pkcsBlockLen, const NameValuePairs& /* parameters */) const
{
	assert (inputLen <= MaxUnpaddedLength(pkcsBlockLen));	// this should be checked by caller

	// convert from bit length to byte length
	if (pkcsBlockLen % 8 != 0)
	{
		pkcsBlock[0] = 0;
		pkcsBlock++;
	}
	pkcsBlockLen /= 8;

	pkcsBlock[0] = 2;  // block type 2

	// pad with non-zero random bytes
	for (unsigned i = 1; i < pkcsBlockLen-inputLen-1; i++)
		pkcsBlock[i] = (byte)rng.GenerateWord32(1, 0xff);

	pkcsBlock[pkcsBlockLen-inputLen-1] = 0;     // separator
	memcpy(pkcsBlock+pkcsBlockLen-inputLen, input, inputLen);
}

DecodingResult PKCS_EncryptionPaddingScheme::Unpad(const byte *pkcsBlock, unsigned int pkcsBlockLen, byte *output, const NameValuePairs& /* parameters */) const
{
	bool invalid = false;
	unsigned int maxOutputLen = MaxUnpaddedLength(pkcsBlockLen);

	// convert from bit length to byte length
	if (pkcsBlockLen % 8 != 0)
	{
		invalid = (pkcsBlock[0] != 0) || invalid;
		pkcsBlock++;
	}
	pkcsBlockLen /= 8;

	// Require block type 2.
	invalid = (pkcsBlock[0] != 2) || invalid;

	// skip past the padding until we find the separator
	unsigned i=1;
	while (i<pkcsBlockLen && pkcsBlock[i++]) { // null body
		}
	assert(i==pkcsBlockLen || pkcsBlock[i-1]==0);

	unsigned int outputLen = pkcsBlockLen - i;
	invalid = (outputLen > maxOutputLen) || invalid;

	if (invalid)
		return DecodingResult();

	memcpy (output, pkcsBlock+i, outputLen);
	return DecodingResult(outputLen);
}

// ********************************************************

#ifndef CRYPTOPP_IMPORTS

void PKCS1v15_SignatureMessageEncodingMethod::ComputeMessageRepresentative(RandomNumberGenerator& /* rng */, 
	const byte* /* recoverableMessage */, unsigned int /* recoverableMessageLength */,
	HashTransformation &hash, HashIdentifier hashIdentifier, bool /* messageEmpty */,
	byte *representative, unsigned int representativeBitLength) const
{
	unsigned int digestSize = hash.DigestSize();
	if (digestSize + hashIdentifier.second + 10 > representativeBitLength/8)
		throw PK_Signer::KeyTooShort();

	unsigned int pkcsBlockLen = representativeBitLength;
	// convert from bit length to byte length
	if (pkcsBlockLen % 8 != 0)
	{
		representative[0] = 0;
		representative++;
	}
	pkcsBlockLen /= 8;

	representative[0] = 1;   // block type 1

	byte *pPadding = representative + 1;
	byte *pDigest = representative + pkcsBlockLen - digestSize;
	byte *pHashId = pDigest - hashIdentifier.second;
	byte *pSeparator = pHashId - 1;

	// pad with 0xff
	memset(pPadding, 0xff, pSeparator-pPadding);
	*pSeparator = 0;
	memcpy(pHashId, hashIdentifier.first, hashIdentifier.second);
	hash.Final(pDigest);
}

#endif

NAMESPACE_END
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
// asn.cpp - written and placed in the public domain by Wei Dai


#ifndef CRYPTOPP_IMPORTS



NAMESPACE_BEGIN(CryptoPP)
USING_NAMESPACE(std)

/// DER Length
unsigned int DERLengthEncode(BufferedTransformation &bt, unsigned int length)
{
	unsigned int i=0;
	if (length <= 0x7f)
	{
		bt.Put(byte(length));
		i++;
	}
	else
	{
		bt.Put(byte(BytePrecision(length) | 0x80));
		i++;
		for (int j=BytePrecision(length); j; --j)
		{
			bt.Put(byte(length >> (j-1)*8));
			i++;
		}
	}
	return i;
}

bool BERLengthDecode(BufferedTransformation &bt, unsigned int &length, bool &definiteLength)
{
	byte b;

	if (!bt.Get(b))
		return false;

	if (!(b & 0x80))
	{
		definiteLength = true;
		length = b;
	}
	else
	{
		unsigned int lengthBytes = b & 0x7f;

		if (lengthBytes == 0)
		{
			definiteLength = false;
			return true;
		}

		definiteLength = true;
		length = 0;
		while (lengthBytes--)
		{
			if (length >> (8*(sizeof(length)-1)))
				BERDecodeError();	// length about to overflow

			if (!bt.Get(b))
				return false;

			length = (length << 8) | b;
		}
	}
	return true;
}

bool BERLengthDecode(BufferedTransformation &bt, unsigned int &length)
{
	bool definiteLength;
	if (!BERLengthDecode(bt, length, definiteLength))
		BERDecodeError();
	return definiteLength;
}

void DEREncodeNull(BufferedTransformation &out)
{
	out.Put(TAG_NULL);
	out.Put(0);
}

void BERDecodeNull(BufferedTransformation &in)
{
	byte b;
	if (!in.Get(b) || b != TAG_NULL)
		BERDecodeError();
	unsigned int length;
	if (!BERLengthDecode(in, length) || length != 0)
		BERDecodeError();
}

/// ASN Strings
unsigned int DEREncodeOctetString(BufferedTransformation &bt, const byte *str, unsigned int strLen)
{
	bt.Put(OCTET_STRING);
	unsigned int lengthBytes = DERLengthEncode(bt, strLen);
	bt.Put(str, strLen);
	return 1+lengthBytes+strLen;
}

unsigned int DEREncodeOctetString(BufferedTransformation &bt, const SecByteBlock &str)
{
	return DEREncodeOctetString(bt, str.begin(), str.size());
}

void DERReencode(BufferedTransformation &source, BufferedTransformation &dest)
{
	byte tag;
	source.Peek(tag);
	BERGeneralDecoder decoder(source, tag);
	DERGeneralEncoder encoder(dest, tag);
	if (decoder.IsDefiniteLength())
		decoder.TransferTo(encoder, decoder.RemainingLength());
	else
	{
		while (!decoder.EndReached())
			DERReencode(decoder, encoder);
	}
	decoder.MessageEnd();
	encoder.MessageEnd();
}

void OID::EncodeValue(BufferedTransformation &bt, unsigned long v)
{
	for (unsigned int i=RoundUpToMultipleOf(STDMAX(7U,BitPrecision(v)), 7U)-7; i != 0; i-=7)
		bt.Put((byte)(0x80 | ((v >> i) & 0x7f)));
	bt.Put((byte)(v & 0x7f));
}

unsigned int OID::DecodeValue(BufferedTransformation &bt, unsigned long &v)
{
	byte b;
	unsigned int i=0;
	v = 0;
	while (true)
	{
		if (!bt.Get(b))
			BERDecodeError();
		i++;
		v <<= 7;
		v += b & 0x7f;
		if (!(b & 0x80))
			return i;
	}
}

void OID::DEREncode(BufferedTransformation &bt) const
{
	assert(m_values.size() >= 2);
	ByteQueue temp;
	temp.Put(byte(m_values[0] * 40 + m_values[1]));
	for (unsigned int i=2; i<m_values.size(); i++)
		EncodeValue(temp, m_values[i]);
	bt.Put(OBJECT_IDENTIFIER);
	DERLengthEncode(bt, temp.CurrentSize());
	temp.TransferTo(bt);
}

void OID::BERDecode(BufferedTransformation &bt)
{
	byte b;
	if (!bt.Get(b) || b != OBJECT_IDENTIFIER)
		BERDecodeError();

	unsigned int length;
	if (!BERLengthDecode(bt, length) || length < 1)
		BERDecodeError();

	if (!bt.Get(b))
		BERDecodeError();
	
	length--;
	m_values.resize(2);
	m_values[0] = b / 40;
	m_values[1] = b % 40;

	while (length > 0)
	{
		unsigned long v;
		unsigned int valueLen = DecodeValue(bt, v);
		if (valueLen > length)
			BERDecodeError();
		m_values.push_back(v);
		length -= valueLen;
	}
}

void OID::BERDecodeAndCheck(BufferedTransformation &bt) const
{
	OID oid(bt);
	if (*this != oid)
		BERDecodeError();
}

BERGeneralDecoder::BERGeneralDecoder(BufferedTransformation &inQueue, byte asnTag)
	: m_inQueue(inQueue), m_finished(false)
{
	Init(asnTag);
}

BERGeneralDecoder::BERGeneralDecoder(BERGeneralDecoder &inQueue, byte asnTag)
	: m_inQueue(inQueue), m_finished(false)
{
	Init(asnTag);
}

void BERGeneralDecoder::Init(byte asnTag)
{
	byte b;
	if (!m_inQueue.Get(b) || b != asnTag)
		BERDecodeError();

	m_definiteLength = BERLengthDecode(m_inQueue, m_length);
	if (!m_definiteLength && !(asnTag & CONSTRUCTED))
		BERDecodeError();	// cannot be primitive and have indefinite length
}

BERGeneralDecoder::~BERGeneralDecoder()
{
	try	// avoid throwing in constructor
	{
		if (!m_finished)
			MessageEnd();
	}
	catch (...)
	{
	}
}

bool BERGeneralDecoder::EndReached() const
{
	if (m_definiteLength)
		return m_length == 0;
	else
	{	// check end-of-content octets
		word16 i;
		return (m_inQueue.PeekWord16(i)==2 && i==0);
	}
}

byte BERGeneralDecoder::PeekByte() const
{
	byte b;
	if (!Peek(b))
		BERDecodeError();
	return b;
}

void BERGeneralDecoder::CheckByte(byte check)
{
	byte b;
	if (!Get(b) || b != check)
		BERDecodeError();
}

void BERGeneralDecoder::MessageEnd()
{
	m_finished = true;
	if (m_definiteLength)
	{
		if (m_length != 0)
			BERDecodeError();
	}
	else
	{	// remove end-of-content octets
		word16 i;
		if (m_inQueue.GetWord16(i) != 2 || i != 0)
			BERDecodeError();
	}
}

unsigned int BERGeneralDecoder::TransferTo2(BufferedTransformation &target, unsigned long &transferBytes, const std::string &channel, bool blocking)
{
	if (m_definiteLength && transferBytes > m_length)
		transferBytes = m_length;
	unsigned int blockedBytes = m_inQueue.TransferTo2(target, transferBytes, channel, blocking);
	ReduceLength(transferBytes);
	return blockedBytes;
}

unsigned int BERGeneralDecoder::CopyRangeTo2(BufferedTransformation &target, unsigned long &begin, unsigned long end, const std::string &channel, bool blocking) const
{
	if (m_definiteLength)
		end = STDMIN((unsigned long)m_length, end);
	return m_inQueue.CopyRangeTo2(target, begin, end, channel, blocking);
}

unsigned int BERGeneralDecoder::ReduceLength(unsigned int delta)
{
	if (m_definiteLength)
	{
		if (m_length < delta)
			BERDecodeError();
		m_length -= delta;
	}
	return delta;
}

DERGeneralEncoder::DERGeneralEncoder(BufferedTransformation &outQueue, byte asnTag)
	: m_outQueue(outQueue), m_finished(false), m_asnTag(asnTag)
{
}

DERGeneralEncoder::DERGeneralEncoder(DERGeneralEncoder &outQueue, byte asnTag)
	: ByteQueue(), m_outQueue(outQueue), m_finished(false), m_asnTag(asnTag)
{
}

DERGeneralEncoder::~DERGeneralEncoder()
{
	try	// avoid throwing in constructor
	{
		if (!m_finished)
			MessageEnd();
	}
	catch (...)
	{
	}
}

void DERGeneralEncoder::MessageEnd()
{
	m_finished = true;
	unsigned int length = (unsigned int)CurrentSize();
	m_outQueue.Put(m_asnTag);
	DERLengthEncode(m_outQueue, length);
	TransferTo(m_outQueue);
}

// *************************************************************

void X509PublicKey::BERDecode(BufferedTransformation &bt)
{
	BERSequenceDecoder subjectPublicKeyInfo(bt);
		BERSequenceDecoder algorithm(subjectPublicKeyInfo);
			GetAlgorithmID().BERDecodeAndCheck(algorithm);
			bool parametersPresent = algorithm.EndReached() ? false : BERDecodeAlgorithmParameters(algorithm);
		algorithm.MessageEnd();

		BERGeneralDecoder subjectPublicKey(subjectPublicKeyInfo, BIT_STRING);
			subjectPublicKey.CheckByte(0);	// unused bits
			BERDecodeKey2(subjectPublicKey, parametersPresent, subjectPublicKey.RemainingLength());
		subjectPublicKey.MessageEnd();
	subjectPublicKeyInfo.MessageEnd();
}

void X509PublicKey::DEREncode(BufferedTransformation &bt) const
{
	DERSequenceEncoder subjectPublicKeyInfo(bt);

		DERSequenceEncoder algorithm(subjectPublicKeyInfo);
			GetAlgorithmID().DEREncode(algorithm);
			DEREncodeAlgorithmParameters(algorithm);
		algorithm.MessageEnd();

		DERGeneralEncoder subjectPublicKey(subjectPublicKeyInfo, BIT_STRING);
			subjectPublicKey.Put(0);	// unused bits
			DEREncodeKey(subjectPublicKey);
		subjectPublicKey.MessageEnd();

	subjectPublicKeyInfo.MessageEnd();
}

void PKCS8PrivateKey::BERDecode(BufferedTransformation &bt)
{
	BERSequenceDecoder privateKeyInfo(bt);
		word32 version;
		BERDecodeUnsigned<word32>(privateKeyInfo, version, INTEGER, 0, 0);	// check version

		BERSequenceDecoder algorithm(privateKeyInfo);
			GetAlgorithmID().BERDecodeAndCheck(algorithm);
			bool parametersPresent = BERDecodeAlgorithmParameters(algorithm);
		algorithm.MessageEnd();

		BERGeneralDecoder octetString(privateKeyInfo, OCTET_STRING);
			BERDecodeKey2(octetString, parametersPresent, privateKeyInfo.RemainingLength());
		octetString.MessageEnd();

		if (!privateKeyInfo.EndReached())
			BERDecodeOptionalAttributes(privateKeyInfo);
	privateKeyInfo.MessageEnd();
}

void PKCS8PrivateKey::DEREncode(BufferedTransformation &bt) const
{
	DERSequenceEncoder privateKeyInfo(bt);
		DEREncodeUnsigned<word32>(privateKeyInfo, 0);	// version

		DERSequenceEncoder algorithm(privateKeyInfo);
			GetAlgorithmID().DEREncode(algorithm);
			DEREncodeAlgorithmParameters(algorithm);
		algorithm.MessageEnd();

		DERGeneralEncoder octetString(privateKeyInfo, OCTET_STRING);
			DEREncodeKey(octetString);
		octetString.MessageEnd();

		DEREncodeOptionalAttributes(privateKeyInfo);
	privateKeyInfo.MessageEnd();
}

void PKCS8PrivateKey::BERDecodeOptionalAttributes(BufferedTransformation &bt)
{
	DERReencode(bt, m_optionalAttributes);
}

void PKCS8PrivateKey::DEREncodeOptionalAttributes(BufferedTransformation &bt) const
{
	m_optionalAttributes.CopyTo(bt);
}

NAMESPACE_END

#endif
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
// rsa.cpp - written and placed in the public domain by Wei Dai


#ifndef CRYPTOPP_IMPORTS

NAMESPACE_BEGIN(CryptoPP)

OID RSAFunction::GetAlgorithmID() const
{
	return ASN1::rsaEncryption();
}

void RSAFunction::BERDecodeKey(BufferedTransformation &bt)
{
	BERSequenceDecoder seq(bt);
		m_n.BERDecode(seq);
		m_e.BERDecode(seq);
	seq.MessageEnd();
}

void RSAFunction::DEREncodeKey(BufferedTransformation &bt) const
{
	DERSequenceEncoder seq(bt);
		m_n.DEREncode(seq);
		m_e.DEREncode(seq);
	seq.MessageEnd();
}

Integer RSAFunction::ApplyFunction(const Integer &x) const
{
	DoQuickSanityCheck();
	return a_exp_b_mod_c(x, m_e, m_n);
}

bool RSAFunction::Validate(RandomNumberGenerator& /* rng */, unsigned int /* level */) const
{
	bool pass = true;
	pass = pass && m_n > Integer::One() && m_n.IsOdd();
	pass = pass && m_e > Integer::One() && m_e.IsOdd() && m_e < m_n;
	return pass;
}

bool RSAFunction::GetVoidValue(const char *name, const std::type_info &valueType, void *pValue) const
{
	return GetValueHelper(this, name, valueType, pValue).Assignable()
		CRYPTOPP_GET_FUNCTION_ENTRY(Modulus)
		CRYPTOPP_GET_FUNCTION_ENTRY(PublicExponent)
		;
}

void RSAFunction::AssignFrom(const NameValuePairs &source)
{
	AssignFromHelper(this, source)
		CRYPTOPP_SET_FUNCTION_ENTRY(Modulus)
		CRYPTOPP_SET_FUNCTION_ENTRY(PublicExponent)
		;
}

// *****************************************************************************

class RSAPrimeSelector : public PrimeSelector
{
public:
	RSAPrimeSelector(const Integer &e) : m_e(e) {}
	virtual ~RSAPrimeSelector() {};
	
	bool IsAcceptable(const Integer &candidate) const {return RelativelyPrime(m_e, candidate-Integer::One());}
	Integer m_e;
};

void InvertibleRSAFunction::GenerateRandom(RandomNumberGenerator &rng, const NameValuePairs &alg)
{
	int modulusSize = 2048;
	alg.GetIntValue(Name::ModulusSize(), modulusSize) || alg.GetIntValue(Name::KeySize(), modulusSize);

	if (modulusSize < 16)
		throw InvalidArgument("InvertibleRSAFunction: specified modulus size is too small");

	m_e = alg.GetValueWithDefault(Name::PublicExponent(), Integer(17));

	if (m_e < 3 || m_e.IsEven())
		throw InvalidArgument("InvertibleRSAFunction: invalid public exponent");

	RSAPrimeSelector selector(m_e);
	const NameValuePairs &primeParam = MakeParametersForTwoPrimesOfEqualSize(modulusSize)
		(Name::PointerToPrimeSelector(), selector.GetSelectorPointer());
	m_p.GenerateRandom(rng, primeParam);
	m_q.GenerateRandom(rng, primeParam);

	m_d = EuclideanMultiplicativeInverse(m_e, LCM(m_p-1, m_q-1));
	assert(m_d.IsPositive());

	m_dp = m_d % (m_p-1);
	m_dq = m_d % (m_q-1);
	m_n = m_p * m_q;
	m_u = m_q.InverseMod(m_p);

}

void InvertibleRSAFunction::Initialize(RandomNumberGenerator &rng, unsigned int keybits, const Integer &e)
{
	GenerateRandom(rng, MakeParameters(Name::ModulusSize(), (int)keybits)(Name::PublicExponent(), e+e.IsEven()));
}

void InvertibleRSAFunction::Initialize(const Integer &n, const Integer &e, const Integer &d)
{
	if (n.IsEven() || e.IsEven() | d.IsEven())
		throw InvalidArgument("InvertibleRSAFunction: input is not a valid RSA private key");

	m_n = n;
	m_e = e;
	m_d = d;

	Integer r = --(d*e);
	unsigned int s = 0;
	while (r.IsEven())
	{
		r >>= 1;
		s++;
	}

	ModularArithmetic modn(n);
	for (Integer i = 2; ; ++i)
	{
		Integer a = modn.Exponentiate(i, r);
		if (a == 1)
			continue;
		Integer b;
		unsigned int j = 0;
		while (a != n-1)
		{
			b = modn.Square(a);
			if (b == 1)
			{
				m_p = GCD(a-1, n);
				m_q = n/m_p;
				m_dp = m_d % (m_p-1);
				m_dq = m_d % (m_q-1);
				m_u = m_q.InverseMod(m_p);
				return;
			}
			if (++j == s)
				throw InvalidArgument("InvertibleRSAFunction: input is not a valid RSA private key");
			a = b;
		}
	}
}

void InvertibleRSAFunction::BERDecodeKey(BufferedTransformation &bt)
{
	BERSequenceDecoder privateKey(bt);
		word32 version;
		BERDecodeUnsigned<word32>(privateKey, version, INTEGER, 0, 0);	// check version
		m_n.BERDecode(privateKey);
		m_e.BERDecode(privateKey);
		m_d.BERDecode(privateKey);
		m_p.BERDecode(privateKey);
		m_q.BERDecode(privateKey);
		m_dp.BERDecode(privateKey);
		m_dq.BERDecode(privateKey);
		m_u.BERDecode(privateKey);
	privateKey.MessageEnd();
}

void InvertibleRSAFunction::DEREncodeKey(BufferedTransformation &bt) const
{
	DERSequenceEncoder privateKey(bt);
		DEREncodeUnsigned<word32>(privateKey, 0);	// version
		m_n.DEREncode(privateKey);
		m_e.DEREncode(privateKey);
		m_d.DEREncode(privateKey);
		m_p.DEREncode(privateKey);
		m_q.DEREncode(privateKey);
		m_dp.DEREncode(privateKey);
		m_dq.DEREncode(privateKey);
		m_u.DEREncode(privateKey);
	privateKey.MessageEnd();
}

Integer InvertibleRSAFunction::CalculateInverse(RandomNumberGenerator &rng, const Integer &x) const 
{
	DoQuickSanityCheck();
	ModularArithmetic modn(m_n);
	Integer r, rInv;
	do {	// do this loop for people using small numbers for testing
		r.Randomize(rng, Integer::One(), m_n - Integer::One());
		rInv = modn.MultiplicativeInverse(r);
	} while (rInv.IsZero());
	Integer re = modn.Exponentiate(r, m_e);
	re = modn.Multiply(re, x);			// blind
	// here we follow the notation of PKCS #1 and let u=q inverse mod p
	// but in ModRoot, u=p inverse mod q, so we reverse the order of p and q
	Integer y = ModularRoot(re, m_dq, m_dp, m_q, m_p, m_u);
	y = modn.Multiply(y, rInv);				// unblind
	if (modn.Exponentiate(y, m_e) != x)		// check
		throw Exception(Exception::OTHER_ERROR, "InvertibleRSAFunction: computational error during private key operation");
	return y;
}

bool InvertibleRSAFunction::Validate(RandomNumberGenerator &rng, unsigned int level) const
{
	bool pass = RSAFunction::Validate(rng, level);
	pass = pass && m_p > Integer::One() && m_p.IsOdd() && m_p < m_n;
	pass = pass && m_q > Integer::One() && m_q.IsOdd() && m_q < m_n;
	pass = pass && m_d > Integer::One() && m_d.IsOdd() && m_d < m_n;
	pass = pass && m_dp > Integer::One() && m_dp.IsOdd() && m_dp < m_p;
	pass = pass && m_dq > Integer::One() && m_dq.IsOdd() && m_dq < m_q;
	pass = pass && m_u.IsPositive() && m_u < m_p;
	if (level >= 1)
	{
		pass = pass && m_p * m_q == m_n;
		pass = pass && m_e*m_d % LCM(m_p-1, m_q-1) == 1;
		pass = pass && m_dp == m_d%(m_p-1) && m_dq == m_d%(m_q-1);
		pass = pass && m_u * m_q % m_p == 1;
	}
	if (level >= 2)
		pass = pass && VerifyPrime(rng, m_p, level-2) && VerifyPrime(rng, m_q, level-2);
	return pass;
}

bool InvertibleRSAFunction::GetVoidValue(const char *name, const std::type_info &valueType, void *pValue) const
{
	return GetValueHelper<RSAFunction>(this, name, valueType, pValue).Assignable()
		CRYPTOPP_GET_FUNCTION_ENTRY(Prime1)
		CRYPTOPP_GET_FUNCTION_ENTRY(Prime2)
		CRYPTOPP_GET_FUNCTION_ENTRY(PrivateExponent)
		CRYPTOPP_GET_FUNCTION_ENTRY(ModPrime1PrivateExponent)
		CRYPTOPP_GET_FUNCTION_ENTRY(ModPrime2PrivateExponent)
		CRYPTOPP_GET_FUNCTION_ENTRY(MultiplicativeInverseOfPrime2ModPrime1)
		;
}

void InvertibleRSAFunction::AssignFrom(const NameValuePairs &source)
{
	AssignFromHelper<RSAFunction>(this, source)
		CRYPTOPP_SET_FUNCTION_ENTRY(Prime1)
		CRYPTOPP_SET_FUNCTION_ENTRY(Prime2)
		CRYPTOPP_SET_FUNCTION_ENTRY(PrivateExponent)
		CRYPTOPP_SET_FUNCTION_ENTRY(ModPrime1PrivateExponent)
		CRYPTOPP_SET_FUNCTION_ENTRY(ModPrime2PrivateExponent)
		CRYPTOPP_SET_FUNCTION_ENTRY(MultiplicativeInverseOfPrime2ModPrime1)
		;
}

NAMESPACE_END

#endif
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
// basecode.cpp - written and placed in the public domain by Wei Dai


#ifndef CRYPTOPP_IMPORTS


NAMESPACE_BEGIN(CryptoPP)

void BaseN_Encoder::IsolatedInitialize(const NameValuePairs &parameters)
{
	parameters.GetRequiredParameter("BaseN_Encoder", Name::EncodingLookupArray(), m_alphabet);

	parameters.GetRequiredIntParameter("BaseN_Encoder", Name::Log2Base(), m_bitsPerChar);
	if (m_bitsPerChar <= 0 || m_bitsPerChar >= 8)
		throw InvalidArgument("BaseN_Encoder: Log2Base must be between 1 and 7 inclusive");

	byte padding;
	bool pad;
	if (parameters.GetValue(Name::PaddingByte(), padding))
		pad = parameters.GetValueWithDefault(Name::Pad(), true);
	else
		pad = false;
	m_padding = pad ? padding : -1;

	m_bytePos = m_bitPos = 0;

	int i = 8;
	while (i%m_bitsPerChar != 0)
		i += 8;
	m_outputBlockSize = i/m_bitsPerChar;

	m_outBuf.New(m_outputBlockSize);
}

unsigned int BaseN_Encoder::Put2(const byte *begin, unsigned int length, int messageEnd, bool blocking)
{
	FILTER_BEGIN;
	while (m_inputPosition < length)
	{
		if (m_bytePos == 0)
			memset(m_outBuf, 0, m_outputBlockSize);

		{
		unsigned int b = begin[m_inputPosition++], bitsLeftInSource = 8;
		while (true)
		{
			assert(m_bitPos < m_bitsPerChar);
			unsigned int bitsLeftInTarget = m_bitsPerChar-m_bitPos;
			m_outBuf[m_bytePos] |= b >> (8-bitsLeftInTarget);
			if (bitsLeftInSource >= bitsLeftInTarget)
			{
				m_bitPos = 0;
				++m_bytePos;
				bitsLeftInSource -= bitsLeftInTarget;
				if (bitsLeftInSource == 0)
					break;
				b <<= bitsLeftInTarget;
				b &= 0xff;
			}
			else
			{
				m_bitPos += bitsLeftInSource;
				break;
			}
		}
		}

		assert(m_bytePos <= m_outputBlockSize);
		if (m_bytePos == m_outputBlockSize)
		{
			int i;
			for (i=0; i<m_bytePos; i++)
			{
				assert(m_outBuf[i] < (1 << m_bitsPerChar));
				m_outBuf[i] = m_alphabet[m_outBuf[i]];
			}
			FILTER_OUTPUT(1, m_outBuf, m_outputBlockSize, 0);
			
			m_bytePos = m_bitPos = 0;
		}
	}
	if (messageEnd)
	{
		if (m_bitPos > 0)
			++m_bytePos;

		int i;
		for (i=0; i<m_bytePos; i++)
			m_outBuf[i] = m_alphabet[m_outBuf[i]];

		if (m_padding != -1 && m_bytePos > 0)
		{
			memset(m_outBuf+m_bytePos, m_padding, m_outputBlockSize-m_bytePos);
			m_bytePos = m_outputBlockSize;
		}
		FILTER_OUTPUT(2, m_outBuf, m_bytePos, messageEnd);
		m_bytePos = m_bitPos = 0;
	}
	FILTER_END_NO_MESSAGE_END;
}

void BaseN_Decoder::IsolatedInitialize(const NameValuePairs &parameters)
{
	parameters.GetRequiredParameter("BaseN_Decoder", Name::DecodingLookupArray(), m_lookup);

	parameters.GetRequiredIntParameter("BaseN_Decoder", Name::Log2Base(), m_bitsPerChar);
	if (m_bitsPerChar <= 0 || m_bitsPerChar >= 8)
		throw InvalidArgument("BaseN_Decoder: Log2Base must be between 1 and 7 inclusive");

	m_bytePos = m_bitPos = 0;

	int i = m_bitsPerChar;
	while (i%8 != 0)
		i += m_bitsPerChar;
	m_outputBlockSize = i/8;

	m_outBuf.New(m_outputBlockSize);
}

unsigned int BaseN_Decoder::Put2(const byte *begin, unsigned int length, int messageEnd, bool blocking)
{
	FILTER_BEGIN;
	while (m_inputPosition < length)
	{
		unsigned int value;
		value = m_lookup[begin[m_inputPosition++]];
		if (value >= 256)
			continue;

		if (m_bytePos == 0 && m_bitPos == 0)
			memset(m_outBuf, 0, m_outputBlockSize);

		{
			int newBitPos = m_bitPos + m_bitsPerChar;
			if (newBitPos <= 8)
				m_outBuf[m_bytePos] |= value << (8-newBitPos);
			else
			{
				m_outBuf[m_bytePos] |= value >> (newBitPos-8);
				m_outBuf[m_bytePos+1] |= value << (16-newBitPos);
			}

			m_bitPos = newBitPos;
			while (m_bitPos >= 8)
			{
				m_bitPos -= 8;
				++m_bytePos;
			}
		}

		if (m_bytePos == m_outputBlockSize)
		{
			FILTER_OUTPUT(1, m_outBuf, m_outputBlockSize, 0);
			m_bytePos = m_bitPos = 0;
		}
	}
	if (messageEnd)
	{
		FILTER_OUTPUT(2, m_outBuf, m_bytePos, messageEnd);
		m_bytePos = m_bitPos = 0;
	}
	FILTER_END_NO_MESSAGE_END;
}

void BaseN_Decoder::InitializeDecodingLookupArray(int *lookup, const byte *alphabet, unsigned int base, bool caseInsensitive)
{
	std::fill(lookup, lookup+256, -1);

	for (unsigned int i=0; i<base; i++)
	{
		if (caseInsensitive && isalpha(alphabet[i]))
		{
			assert(lookup[toupper(alphabet[i])] == -1);
			lookup[toupper(alphabet[i])] = i;
			assert(lookup[tolower(alphabet[i])] == -1);
			lookup[tolower(alphabet[i])] = i;
		}
		else
		{
			assert(lookup[alphabet[i]] == -1);
			lookup[alphabet[i]] = i;
		}
	}
}

void Grouper::IsolatedInitialize(const NameValuePairs &parameters)
{
	m_groupSize = parameters.GetIntValueWithDefault(Name::GroupSize(), 0);
	ConstByteArrayParameter separator, terminator;
	if (m_groupSize)
		parameters.GetRequiredParameter("Grouper", Name::Separator(), separator);
	else
		parameters.GetValue(Name::Separator(), separator);
	parameters.GetValue(Name::Terminator(), terminator);

	m_separator.Assign(separator.begin(), separator.size());
	m_terminator.Assign(terminator.begin(), terminator.size());
	m_counter = 0;
}

unsigned int Grouper::Put2(const byte *begin, unsigned int length, int messageEnd, bool blocking)
{
	FILTER_BEGIN;
	if (m_groupSize)
	{
		while (m_inputPosition < length)
		{
			if (m_counter == m_groupSize)
			{
				FILTER_OUTPUT(1, m_separator, m_separator.size(), 0);
				m_counter = 0;
			}

			unsigned int len;
			FILTER_OUTPUT2(2, len = STDMIN(length-m_inputPosition, m_groupSize-m_counter),
				begin+m_inputPosition, len, 0);
			m_inputPosition += len;
			m_counter += len;
		}
	}
	else
		FILTER_OUTPUT(3, begin, length, 0);

	if (messageEnd)
	{
		FILTER_OUTPUT(4, m_terminator, m_terminator.size(), messageEnd);
		m_counter = 0;
	}
	FILTER_END_NO_MESSAGE_END
}

NAMESPACE_END

#endif
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
// base64.cpp - written and placed in the public domain by Wei Dai


NAMESPACE_BEGIN(CryptoPP)

static const byte s_vec[] =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const byte s_padding = '=';

void Base64Encoder::IsolatedInitialize(const NameValuePairs &parameters)
{
	bool insertLineBreaks = parameters.GetValueWithDefault(Name::InsertLineBreaks(), true);
	int maxLineLength = parameters.GetIntValueWithDefault(Name::MaxLineLength(), 72);

	const char *lineBreak = insertLineBreaks ? "\n" : "";
	
	m_filter->Initialize(CombinedNameValuePairs(
		parameters,
		MakeParameters(Name::EncodingLookupArray(), &s_vec[0], false)
			(Name::PaddingByte(), s_padding)
			(Name::GroupSize(), insertLineBreaks ? maxLineLength : 0)
			(Name::Separator(), ConstByteArrayParameter(lineBreak))
			(Name::Terminator(), ConstByteArrayParameter(lineBreak))
			(Name::Log2Base(), 6, true)));
}

const int *Base64Decoder::GetDecodingLookupArray()
{
	static bool s_initialized = false;
	static int s_array[256];

	if (!s_initialized)
	{
		InitializeDecodingLookupArray(s_array, s_vec, 64, false);
		s_initialized = true;
	}
	return s_array;
}

NAMESPACE_END
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
// files.cpp - written and placed in the public domain by Wei Dai


#ifndef CRYPTOPP_IMPORTS


NAMESPACE_BEGIN(CryptoPP)

using namespace std;

void FileStore::StoreInitialize(const NameValuePairs &parameters)
{
	m_file.reset(new std::ifstream);
	const char *fileName;
	if (parameters.GetValue(Name::InputFileName(), fileName))
	{
		ios::openmode binary = parameters.GetValueWithDefault(Name::InputBinaryMode(), true) ? ios::binary : ios::openmode(0);
		m_file->open(fileName, ios::in | binary);
		if (!*m_file)
			throw OpenErr(fileName);
		m_stream = m_file.get();
	}
	else
	{
		m_stream = NULL;
		parameters.GetValue(Name::InputStreamPointer(), m_stream);
	}
	m_waiting = false;
}

unsigned long FileStore::MaxRetrievable() const
{
	if (!m_stream)
		return 0;

	streampos current = m_stream->tellg();
	streampos end = m_stream->seekg(0, ios::end).tellg();
	m_stream->seekg(current);
	return end-current;
}

unsigned int FileStore::TransferTo2(BufferedTransformation &target, unsigned long &transferBytes, const std::string &channel, bool blocking)
{
	if (!m_stream)
	{
		transferBytes = 0;
		return 0;
	}

	unsigned long size=transferBytes;
	transferBytes = 0;

	if (m_waiting)
		goto output;

	while (size && m_stream->good())
	{
		{
		unsigned int spaceSize = 1024;
		m_space = HelpCreatePutSpace(target, channel, 1, (unsigned int)STDMIN(size, (unsigned long)UINT_MAX), spaceSize);

		m_stream->read((char *)m_space, STDMIN(size, (unsigned long)spaceSize));
		}
		m_len = m_stream->gcount();
		unsigned int blockedBytes;
output:
		blockedBytes = target.ChannelPutModifiable2(channel, m_space, m_len, 0, blocking);
		m_waiting = blockedBytes > 0;
		if (m_waiting)
			return blockedBytes;
		size -= m_len;
		transferBytes += m_len;
	}

	if (!m_stream->good() && !m_stream->eof())
		throw ReadErr();

	return 0;
}

unsigned int FileStore::CopyRangeTo2(BufferedTransformation &target, unsigned long &begin, unsigned long end, const std::string &channel, bool blocking) const
{
	if (!m_stream)
		return 0;

	if (begin == 0 && end == 1)
	{
		int result = m_stream->peek();
		if (result == EOF)	// GCC workaround: 2.95.2 doesn't have char_traits<char>::eof()
			return 0;
		else
		{
			unsigned int blockedBytes = target.ChannelPut(channel, byte(result), blocking);
			begin += 1-blockedBytes;
			return blockedBytes;
		}
	}

	// TODO: figure out what happens on cin
	streampos current = m_stream->tellg();
	streampos endPosition = m_stream->seekg(0, ios::end).tellg();
	streampos newPosition = current + (streamoff)begin;

	if (newPosition >= endPosition)
	{
		m_stream->seekg(current);
		return 0;	// don't try to seek beyond the end of file
	}
	m_stream->seekg(newPosition);
	try
	{
		assert(!m_waiting);
		unsigned long copyMax = end-begin;
		unsigned int blockedBytes = const_cast<FileStore *>(this)->TransferTo2(target, copyMax, channel, blocking);
		begin += copyMax;
		if (blockedBytes)
		{
			const_cast<FileStore *>(this)->m_waiting = false;
			return blockedBytes;
		}
	}
	catch(...)
	{
		m_stream->clear();
		m_stream->seekg(current);
		throw;
	}
	m_stream->clear();
	m_stream->seekg(current);

	return 0;
}

unsigned long FileStore::Skip(unsigned long skipMax)
{
	unsigned long oldPos = m_stream->tellg();
	m_stream->seekg(skipMax, ios::cur);
	return (unsigned long)m_stream->tellg() - oldPos;
}

void FileSink::IsolatedInitialize(const NameValuePairs &parameters)
{
	m_file.reset(new std::ofstream);
	const char *fileName;
	if (parameters.GetValue(Name::OutputFileName(), fileName))
	{
		ios::openmode binary = parameters.GetValueWithDefault(Name::OutputBinaryMode(), true) ? ios::binary : ios::openmode(0);
		m_file->open(fileName, ios::out | ios::trunc | binary);
		if (!*m_file)
			throw OpenErr(fileName);
		m_stream = m_file.get();
	}
	else
	{
		m_stream = NULL;
		parameters.GetValue(Name::OutputStreamPointer(), m_stream);
	}
}

bool FileSink::IsolatedFlush(bool /* hardFlush */, bool /* blocking */)
{
	if (!m_stream)
		throw Err("FileSink: output stream not opened");

	m_stream->flush();
	if (!m_stream->good())
		throw WriteErr();

	return false;
}

unsigned int FileSink::Put2(const byte *inString, unsigned int length, int messageEnd, bool /* blocking */)
{
	if (!m_stream)
		throw Err("FileSink: output stream not opened");

	m_stream->write((const char *)inString, length);

	if (messageEnd)
		m_stream->flush();

	if (!m_stream->good())
		throw WriteErr();

	return 0;
}

NAMESPACE_END

#endif
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
 // mdc.h - written and placed in the public domain by Wei Dai

#ifndef CRYPTOPP_MDC_H
#define CRYPTOPP_MDC_H

/** \file
*/


NAMESPACE_BEGIN(CryptoPP)

//! _
template <class T>
struct MDC_Info : public FixedBlockSize<T::DIGESTSIZE>, public FixedKeyLength<T::BLOCKSIZE>
{
	static std::string StaticAlgorithmName() {return std::string("MDC/")+T::StaticAlgorithmName();}
};

//! <a href="http://www.weidai.com/scan-mirror/cs.html#MDC">MDC</a>
/*! a construction by Peter Gutmann to turn an iterated hash function into a PRF */
template <class T>
class MDC : public MDC_Info<T>
{
	class CRYPTOPP_NO_VTABLE Enc : public BlockCipherImpl<MDC_Info<T> >
	{
		typedef typename T::HashWordType HashWordType;

	public:
		void UncheckedSetKey(CipherDir direction, const byte *userKey, unsigned int length)
		{
			assert(direction == ENCRYPTION);
			this->AssertValidKeyLength(length);
			memcpy(Key(), userKey, this->KEYLENGTH);
			T::CorrectEndianess(Key(), Key(), this->KEYLENGTH);
		}

		void ProcessAndXorBlock(const byte *inBlock, const byte *xorBlock, byte *outBlock) const
		{
			T::CorrectEndianess(Buffer(), (HashWordType *)inBlock, this->BLOCKSIZE);
			T::Transform(Buffer(), Key());
			if (xorBlock)
			{
				T::CorrectEndianess(Buffer(), Buffer(), this->BLOCKSIZE);
				xorbuf(outBlock, xorBlock, m_buffer, this->BLOCKSIZE);
			}
			else
				T::CorrectEndianess((HashWordType *)outBlock, Buffer(), this->BLOCKSIZE);
		}

		bool IsPermutation() const {return false;}

		unsigned int GetAlignment() const {return sizeof(HashWordType);}

	private:
		HashWordType *Key() {return (HashWordType *)m_key.data();}
		const HashWordType *Key() const {return (const HashWordType *)m_key.data();}
		HashWordType *Buffer() const {return (HashWordType *)m_buffer.data();}

		// VC60 workaround: bug triggered if using FixedSizeAllocatorWithCleanup
		FixedSizeSecBlock<byte, MDC_Info<T>::KEYLENGTH, AllocatorWithCleanup<byte> > m_key;
		mutable FixedSizeSecBlock<byte, MDC_Info<T>::BLOCKSIZE, AllocatorWithCleanup<byte> > m_buffer;
	};

public:
	//! use BlockCipher interface
	typedef BlockCipherFinal<ENCRYPTION, Enc> Encryption;
};

NAMESPACE_END

#endif
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
/*! \file
 	This file contains helper classes for implementing stream ciphers.

	All this infrastructure may look very complex compared to what's in Crypto++ 4.x,
	but stream ciphers implementations now support a lot of new functionality,
	including better performance (minimizing copying), resetting of keys and IVs, and methods to
	query which features are supported by a cipher.

	Here's an explanation of these classes. The word "policy" is used here to mean a class with a
	set of methods that must be implemented by individual stream cipher implementations.
	This is usually much simpler than the full stream cipher API, which is implemented by
	either AdditiveCipherTemplate or CFB_CipherTemplate using the policy. So for example, an
	implementation of SEAL only needs to implement the AdditiveCipherAbstractPolicy interface
	(since it's an additive cipher, i.e., it xors a keystream into the plaintext).
	See this line in seal.h:

	typedef SymmetricCipherFinal\<ConcretePolicyHolder\<SEAL_Policy\<B\>, AdditiveCipherTemplate\<\> \> \> Encryption;

	AdditiveCipherTemplate and CFB_CipherTemplate are designed so that they don't need
	to take a policy class as a template parameter (although this is allowed), so that
	their code is not duplicated for each new cipher. Instead they each
	get a reference to an abstract policy interface by calling AccessPolicy() on itself, so
	AccessPolicy() must be overriden to return the actual policy reference. This is done
	by the ConceretePolicyHolder class. Finally, SymmetricCipherFinal implements the constructors and
	other functions that must be implemented by the most derived class.
*/

#ifndef CRYPTOPP_STRCIPHR_H
#define CRYPTOPP_STRCIPHR_H


NAMESPACE_BEGIN(CryptoPP)

template <class POLICY_INTERFACE, class BASE = Empty>
class CRYPTOPP_NO_VTABLE AbstractPolicyHolder : public BASE
{
public:
	typedef POLICY_INTERFACE PolicyInterface;

protected:
	virtual const POLICY_INTERFACE & GetPolicy() const =0;
	virtual POLICY_INTERFACE & AccessPolicy() =0;
};

template <class POLICY, class BASE, class POLICY_INTERFACE = CPP_TYPENAME BASE::PolicyInterface>
class ConcretePolicyHolder : public BASE, protected POLICY
{
protected:
	const POLICY_INTERFACE & GetPolicy() const {return *this;}
	POLICY_INTERFACE & AccessPolicy() {return *this;}
};

enum KeystreamOperation {WRITE_KEYSTREAM, XOR_KEYSTREAM, XOR_KEYSTREAM_INPLACE};

class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE CFB_CipherAbstractPolicy
{
public:
	virtual ~CFB_CipherAbstractPolicy() {};
	virtual unsigned int GetAlignment() const =0;
	virtual unsigned int GetBytesPerIteration() const =0;
	virtual byte * GetRegisterBegin() =0;
	virtual void TransformRegister() =0;
	virtual bool CanIterate() const {return false;}
	virtual void Iterate(byte* /* output */, const byte* /* input */, CipherDir /* dir */, unsigned int /* iterationCount */) {assert(false);}
	virtual void CipherSetKey(const NameValuePairs &params, const byte *key, unsigned int length) =0;
	virtual void CipherResynchronize(const byte* /* iv */) {throw NotImplemented("StreamTransformation: this object doesn't support resynchronization");}
};

template <class BASE>
class CRYPTOPP_NO_VTABLE CFB_CipherTemplate : public BASE
{
public:
	void ProcessData(byte *outString, const byte *inString, unsigned int length);
	void Resynchronize(const byte *iv);
	unsigned int OptimalBlockSize() const {return this->GetPolicy().GetBytesPerIteration();}
	unsigned int GetOptimalNextBlockSize() const {return m_leftOver;}
	unsigned int OptimalDataAlignment() const {return this->GetPolicy().GetAlignment();}
	bool IsRandomAccess() const {return false;}
	bool IsSelfInverting() const {return false;}

	typedef typename BASE::PolicyInterface PolicyInterface;

protected:
	virtual void CombineMessageAndShiftRegister(byte *output, byte *reg, const byte *message, unsigned int length) =0;

	void UncheckedSetKey(const NameValuePairs &params, const byte *key, unsigned int length, const byte *iv);

	unsigned int m_leftOver;
};

template <class BASE = AbstractPolicyHolder<CFB_CipherAbstractPolicy, SymmetricCipher> >
class CRYPTOPP_NO_VTABLE CFB_EncryptionTemplate : public CFB_CipherTemplate<BASE>
{
	bool IsForwardTransformation() const {return true;}
	void CombineMessageAndShiftRegister(byte *output, byte *reg, const byte *message, unsigned int length);
};

template <class BASE = AbstractPolicyHolder<CFB_CipherAbstractPolicy, SymmetricCipher> >
class CRYPTOPP_NO_VTABLE CFB_DecryptionTemplate : public CFB_CipherTemplate<BASE>
{
	bool IsForwardTransformation() const {return false;}
	void CombineMessageAndShiftRegister(byte *output, byte *reg, const byte *message, unsigned int length);
};

template <class BASE>
void CFB_CipherTemplate<BASE>::UncheckedSetKey(const NameValuePairs &params, const byte *key, unsigned int length, const byte *iv)
{
	PolicyInterface &policy = this->AccessPolicy();
	policy.CipherSetKey(params, key, length);

	if (this->IsResynchronizable())
		policy.CipherResynchronize(iv);

	m_leftOver = policy.GetBytesPerIteration();
}

NAMESPACE_END

#endif
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
// strciphr.cpp - written and placed in the public domain by Wei Dai


#ifndef CRYPTOPP_IMPORTS


NAMESPACE_BEGIN(CryptoPP)

template <class BASE>
void CFB_CipherTemplate<BASE>::Resynchronize(const byte *iv)
{
	PolicyInterface &policy = this->AccessPolicy();
	policy.CipherResynchronize(iv);
	m_leftOver = policy.GetBytesPerIteration();
}

template <class BASE>
void CFB_CipherTemplate<BASE>::ProcessData(byte *outString, const byte *inString, unsigned int length)
{
	assert(length % this->MandatoryBlockSize() == 0);

	PolicyInterface &policy = this->AccessPolicy();
	unsigned int bytesPerIteration = policy.GetBytesPerIteration();
	unsigned int alignment = policy.GetAlignment();
	byte *reg = policy.GetRegisterBegin();

	if (m_leftOver)
	{
		unsigned int len = STDMIN(m_leftOver, length);
		CombineMessageAndShiftRegister(outString, reg + bytesPerIteration - m_leftOver, inString, len);
		m_leftOver -= len;
		length -= len;
		inString += len;
		outString += len;
	}

	if (!length)
		return;

	assert(m_leftOver == 0);

	if (policy.CanIterate() && length >= bytesPerIteration && IsAlignedOn(outString, alignment))
	{
		if (IsAlignedOn(inString, alignment))
			policy.Iterate(outString, inString, GetCipherDir(*this), length / bytesPerIteration);
		else
		{
			memcpy(outString, inString, length);
			policy.Iterate(outString, outString, GetCipherDir(*this), length / bytesPerIteration);
		}
		inString += length - length % bytesPerIteration;
		outString += length - length % bytesPerIteration;
		length %= bytesPerIteration;
	}

	while (length >= bytesPerIteration)
	{
		policy.TransformRegister();
		CombineMessageAndShiftRegister(outString, reg, inString, bytesPerIteration);
		length -= bytesPerIteration;
		inString += bytesPerIteration;
		outString += bytesPerIteration;
	}

	if (length > 0)
	{
		policy.TransformRegister();
		CombineMessageAndShiftRegister(outString, reg, inString, length);
		m_leftOver = bytesPerIteration - length;
	}
}

template <class BASE>
void CFB_EncryptionTemplate<BASE>::CombineMessageAndShiftRegister(byte *output, byte *reg, const byte *message, unsigned int length)
{
	xorbuf(reg, message, length);
	memcpy(output, reg, length);
}

template <class BASE>
void CFB_DecryptionTemplate<BASE>::CombineMessageAndShiftRegister(byte *output, byte *reg, const byte *message, unsigned int length)
{
	for (unsigned int i=0; i<length; i++)
	{
		byte b = message[i];
		output[i] = reg[i] ^ b;
		reg[i] = b;
	}
}

NAMESPACE_END

#endif
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
#ifndef CRYPTOPP_MODES_H
#define CRYPTOPP_MODES_H

/*! \file
*/


NAMESPACE_BEGIN(CryptoPP)

//! Cipher mode documentation. See NIST SP 800-38A for definitions of these modes.

/*! Each class derived from this one defines two types, Encryption and Decryption, 
	both of which implement the SymmetricCipher interface.
	For each mode there are two classes, one of which is a template class,
	and the other one has a name that ends in "_ExternalCipher".
	The "external cipher" mode objects hold a reference to the underlying block cipher,
	instead of holding an instance of it. The reference must be passed in to the constructor.
	For the "cipher holder" classes, the CIPHER template parameter should be a class
	derived from BlockCipherDocumentation, for example DES or AES.
*/
struct CipherModeDocumentation : public SymmetricCipherDocumentation
{
};

class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE CipherModeBase : public SymmetricCipher
{
public:
	unsigned int MinKeyLength() const {return m_cipher->MinKeyLength();}
	unsigned int MaxKeyLength() const {return m_cipher->MaxKeyLength();}
	unsigned int DefaultKeyLength() const {return m_cipher->DefaultKeyLength();}
	unsigned int GetValidKeyLength(unsigned int n) const {return m_cipher->GetValidKeyLength(n);}
	bool IsValidKeyLength(unsigned int n) const {return m_cipher->IsValidKeyLength(n);}

	void SetKey(const byte *key, unsigned int length, const NameValuePairs &params = g_nullNameValuePairs);

	unsigned int OptimalDataAlignment() const {return BlockSize();}

	unsigned int IVSize() const {return BlockSize();}
	void GetNextIV(byte *IV);
	virtual IV_Requirement IVRequirement() const =0;

protected:
	inline unsigned int BlockSize() const {assert(m_register.size() > 0); return m_register.size();}
	virtual void SetFeedbackSize(unsigned int feedbackSize)
	{
		if (!(feedbackSize == 0 || feedbackSize == BlockSize()))
			throw InvalidArgument("CipherModeBase: feedback size cannot be specified for this cipher mode");
	}
	virtual void ResizeBuffers()
	{
		m_register.New(m_cipher->BlockSize());
	}
	virtual void UncheckedSetKey(const NameValuePairs &params, const byte *key, unsigned int length, const byte *iv) =0;

	BlockCipher *m_cipher;
	SecByteBlock m_register;
};

template <class POLICY_INTERFACE>
class CRYPTOPP_NO_VTABLE ModePolicyCommonTemplate : public CipherModeBase, public POLICY_INTERFACE
{
	unsigned int GetAlignment() const {return m_cipher->BlockAlignment();}
	void CipherSetKey(const NameValuePairs &params, const byte *key, unsigned int length);
};

template <class POLICY_INTERFACE>
void ModePolicyCommonTemplate<POLICY_INTERFACE>::CipherSetKey(const NameValuePairs &params, const byte *key, unsigned int length)
{
	m_cipher->SetKey(key, length, params);
	ResizeBuffers();
	int feedbackSize = params.GetIntValueWithDefault(Name::FeedbackSize(), 0);
	SetFeedbackSize(feedbackSize);
}

class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE CFB_ModePolicy : public ModePolicyCommonTemplate<CFB_CipherAbstractPolicy>
{
public:
	IV_Requirement IVRequirement() const {return RANDOM_IV;}
	static const char *StaticAlgorithmName() {return "CFB";}

protected:
	unsigned int GetBytesPerIteration() const {return m_feedbackSize;}
	byte * GetRegisterBegin() {return m_register + BlockSize() - m_feedbackSize;}
	void TransformRegister()
	{
		m_cipher->ProcessBlock(m_register, m_temp);
		memmove(m_register, m_register+m_feedbackSize, BlockSize()-m_feedbackSize);
		memcpy(m_register+BlockSize()-m_feedbackSize, m_temp, m_feedbackSize);
	}
	void CipherResynchronize(const byte *iv)
	{
		memcpy(m_register, iv, BlockSize());
		TransformRegister();
	}
	void SetFeedbackSize(unsigned int feedbackSize)
	{
		if (feedbackSize > BlockSize())
			throw InvalidArgument("CFB_Mode: invalid feedback size");
		m_feedbackSize = feedbackSize ? feedbackSize : BlockSize();
	}
	void ResizeBuffers()
	{
		CipherModeBase::ResizeBuffers();
		m_temp.New(BlockSize());
	}

	SecByteBlock m_temp;
	unsigned int m_feedbackSize;
};

//! _
template <class CIPHER, class BASE>
class CipherModeFinalTemplate_CipherHolder : protected ObjectHolder<CIPHER>, public AlgorithmImpl<BASE, CipherModeFinalTemplate_CipherHolder<CIPHER, BASE> >
{
public:
	CipherModeFinalTemplate_CipherHolder()
	{
		this->m_cipher = &this->m_object;
		this->ResizeBuffers();
	}
	CipherModeFinalTemplate_CipherHolder(const byte *key, unsigned int length)
	{
		this->m_cipher = &this->m_object;
		this->SetKey(key, length);
	}
	CipherModeFinalTemplate_CipherHolder(const byte *key, unsigned int length, const byte *iv)
	{
		this->m_cipher = &this->m_object;
		this->SetKey(key, length, MakeParameters(Name::IV(), iv));
	}
	CipherModeFinalTemplate_CipherHolder(const byte *key, unsigned int length, const byte *iv, int feedbackSize)
	{
		this->m_cipher = &this->m_object;
		this->SetKey(key, length, MakeParameters(Name::IV(), iv)(Name::FeedbackSize(), feedbackSize));
	}

	static std::string StaticAlgorithmName()
		{return CIPHER::StaticAlgorithmName() + "/" + BASE::StaticAlgorithmName();}
};

//! CFB mode
template <class CIPHER>
struct CFB_Mode : public CipherModeDocumentation
{
	typedef CipherModeFinalTemplate_CipherHolder<CPP_TYPENAME CIPHER::Encryption, ConcretePolicyHolder<Empty, CFB_EncryptionTemplate<AbstractPolicyHolder<CFB_CipherAbstractPolicy, CFB_ModePolicy> > > > Encryption;
	typedef CipherModeFinalTemplate_CipherHolder<CPP_TYPENAME CIPHER::Encryption, ConcretePolicyHolder<Empty, CFB_DecryptionTemplate<AbstractPolicyHolder<CFB_CipherAbstractPolicy, CFB_ModePolicy> > > > Decryption;
};

NAMESPACE_END

#endif
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// dessp.cpp
// This file is mostly generated by Phil Karn's gensp.c

#ifndef CRYPTOPP_IMPORTS

NAMESPACE_BEGIN(CryptoPP)

// VC60 workaround: gives a C4786 warning without this function
// when runtime lib is set to multithread debug DLL
// even though warning 4786 is disabled!
void DES_VC60Workaround()
{
}

const word32 RawDES::Spbox[8][64] = {
{
0x01010400,0x00000000,0x00010000,0x01010404, 0x01010004,0x00010404,0x00000004,0x00010000,
0x00000400,0x01010400,0x01010404,0x00000400, 0x01000404,0x01010004,0x01000000,0x00000004,
0x00000404,0x01000400,0x01000400,0x00010400, 0x00010400,0x01010000,0x01010000,0x01000404,
0x00010004,0x01000004,0x01000004,0x00010004, 0x00000000,0x00000404,0x00010404,0x01000000,
0x00010000,0x01010404,0x00000004,0x01010000, 0x01010400,0x01000000,0x01000000,0x00000400,
0x01010004,0x00010000,0x00010400,0x01000004, 0x00000400,0x00000004,0x01000404,0x00010404,
0x01010404,0x00010004,0x01010000,0x01000404, 0x01000004,0x00000404,0x00010404,0x01010400,
0x00000404,0x01000400,0x01000400,0x00000000, 0x00010004,0x00010400,0x00000000,0x01010004},
{
0x80108020,0x80008000,0x00008000,0x00108020, 0x00100000,0x00000020,0x80100020,0x80008020,
0x80000020,0x80108020,0x80108000,0x80000000, 0x80008000,0x00100000,0x00000020,0x80100020,
0x00108000,0x00100020,0x80008020,0x00000000, 0x80000000,0x00008000,0x00108020,0x80100000,
0x00100020,0x80000020,0x00000000,0x00108000, 0x00008020,0x80108000,0x80100000,0x00008020,
0x00000000,0x00108020,0x80100020,0x00100000, 0x80008020,0x80100000,0x80108000,0x00008000,
0x80100000,0x80008000,0x00000020,0x80108020, 0x00108020,0x00000020,0x00008000,0x80000000,
0x00008020,0x80108000,0x00100000,0x80000020, 0x00100020,0x80008020,0x80000020,0x00100020,
0x00108000,0x00000000,0x80008000,0x00008020, 0x80000000,0x80100020,0x80108020,0x00108000},
{
0x00000208,0x08020200,0x00000000,0x08020008, 0x08000200,0x00000000,0x00020208,0x08000200,
0x00020008,0x08000008,0x08000008,0x00020000, 0x08020208,0x00020008,0x08020000,0x00000208,
0x08000000,0x00000008,0x08020200,0x00000200, 0x00020200,0x08020000,0x08020008,0x00020208,
0x08000208,0x00020200,0x00020000,0x08000208, 0x00000008,0x08020208,0x00000200,0x08000000,
0x08020200,0x08000000,0x00020008,0x00000208, 0x00020000,0x08020200,0x08000200,0x00000000,
0x00000200,0x00020008,0x08020208,0x08000200, 0x08000008,0x00000200,0x00000000,0x08020008,
0x08000208,0x00020000,0x08000000,0x08020208, 0x00000008,0x00020208,0x00020200,0x08000008,
0x08020000,0x08000208,0x00000208,0x08020000, 0x00020208,0x00000008,0x08020008,0x00020200},
{
0x00802001,0x00002081,0x00002081,0x00000080, 0x00802080,0x00800081,0x00800001,0x00002001,
0x00000000,0x00802000,0x00802000,0x00802081, 0x00000081,0x00000000,0x00800080,0x00800001,
0x00000001,0x00002000,0x00800000,0x00802001, 0x00000080,0x00800000,0x00002001,0x00002080,
0x00800081,0x00000001,0x00002080,0x00800080, 0x00002000,0x00802080,0x00802081,0x00000081,
0x00800080,0x00800001,0x00802000,0x00802081, 0x00000081,0x00000000,0x00000000,0x00802000,
0x00002080,0x00800080,0x00800081,0x00000001, 0x00802001,0x00002081,0x00002081,0x00000080,
0x00802081,0x00000081,0x00000001,0x00002000, 0x00800001,0x00002001,0x00802080,0x00800081,
0x00002001,0x00002080,0x00800000,0x00802001, 0x00000080,0x00800000,0x00002000,0x00802080},
{
0x00000100,0x02080100,0x02080000,0x42000100, 0x00080000,0x00000100,0x40000000,0x02080000,
0x40080100,0x00080000,0x02000100,0x40080100, 0x42000100,0x42080000,0x00080100,0x40000000,
0x02000000,0x40080000,0x40080000,0x00000000, 0x40000100,0x42080100,0x42080100,0x02000100,
0x42080000,0x40000100,0x00000000,0x42000000, 0x02080100,0x02000000,0x42000000,0x00080100,
0x00080000,0x42000100,0x00000100,0x02000000, 0x40000000,0x02080000,0x42000100,0x40080100,
0x02000100,0x40000000,0x42080000,0x02080100, 0x40080100,0x00000100,0x02000000,0x42080000,
0x42080100,0x00080100,0x42000000,0x42080100, 0x02080000,0x00000000,0x40080000,0x42000000,
0x00080100,0x02000100,0x40000100,0x00080000, 0x00000000,0x40080000,0x02080100,0x40000100},
{
0x20000010,0x20400000,0x00004000,0x20404010, 0x20400000,0x00000010,0x20404010,0x00400000,
0x20004000,0x00404010,0x00400000,0x20000010, 0x00400010,0x20004000,0x20000000,0x00004010,
0x00000000,0x00400010,0x20004010,0x00004000, 0x00404000,0x20004010,0x00000010,0x20400010,
0x20400010,0x00000000,0x00404010,0x20404000, 0x00004010,0x00404000,0x20404000,0x20000000,
0x20004000,0x00000010,0x20400010,0x00404000, 0x20404010,0x00400000,0x00004010,0x20000010,
0x00400000,0x20004000,0x20000000,0x00004010, 0x20000010,0x20404010,0x00404000,0x20400000,
0x00404010,0x20404000,0x00000000,0x20400010, 0x00000010,0x00004000,0x20400000,0x00404010,
0x00004000,0x00400010,0x20004010,0x00000000, 0x20404000,0x20000000,0x00400010,0x20004010},
{
0x00200000,0x04200002,0x04000802,0x00000000, 0x00000800,0x04000802,0x00200802,0x04200800,
0x04200802,0x00200000,0x00000000,0x04000002, 0x00000002,0x04000000,0x04200002,0x00000802,
0x04000800,0x00200802,0x00200002,0x04000800, 0x04000002,0x04200000,0x04200800,0x00200002,
0x04200000,0x00000800,0x00000802,0x04200802, 0x00200800,0x00000002,0x04000000,0x00200800,
0x04000000,0x00200800,0x00200000,0x04000802, 0x04000802,0x04200002,0x04200002,0x00000002,
0x00200002,0x04000000,0x04000800,0x00200000, 0x04200800,0x00000802,0x00200802,0x04200800,
0x00000802,0x04000002,0x04200802,0x04200000, 0x00200800,0x00000000,0x00000002,0x04200802,
0x00000000,0x00200802,0x04200000,0x00000800, 0x04000002,0x04000800,0x00000800,0x00200002},
{
0x10001040,0x00001000,0x00040000,0x10041040, 0x10000000,0x10001040,0x00000040,0x10000000,
0x00040040,0x10040000,0x10041040,0x00041000, 0x10041000,0x00041040,0x00001000,0x00000040,
0x10040000,0x10000040,0x10001000,0x00001040, 0x00041000,0x00040040,0x10040040,0x10041000,
0x00001040,0x00000000,0x00000000,0x10040040, 0x10000040,0x10001000,0x00041040,0x00040000,
0x00041040,0x00040000,0x10041000,0x00001000, 0x00000040,0x10040040,0x00001000,0x00041040,
0x10001000,0x00000040,0x10000040,0x10040000, 0x10040040,0x10000000,0x00040000,0x10001040,
0x00000000,0x10041040,0x00040040,0x10000040, 0x10040000,0x10001000,0x10001040,0x00000000,
0x10041040,0x00041000,0x00041000,0x00001040, 0x00001040,0x00040040,0x10000000,0x10041000}
};

NAMESPACE_END

#endif
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// des.cpp - modified by Wei Dai from Phil Karn's des.c
// The original code and all modifications are in the public domain.

NAMESPACE_BEGIN(CryptoPP)

typedef BlockGetAndPut<word32, BigEndian> Block;


// Wei Dai's modification to Richard Outerbridge's initial permutation 
// algorithm, this one is faster if you have access to rotate instructions 
// (like in MSVC)
static inline void IPERM(word32 &left, word32 &right)
{
	word32 work;

	right = rotlFixed(right, 4U);
	work = (left ^ right) & 0xf0f0f0f0;
	left ^= work;
	right = rotrFixed(right^work, 20U);
	work = (left ^ right) & 0xffff0000;
	left ^= work;
	right = rotrFixed(right^work, 18U);
	work = (left ^ right) & 0x33333333;
	left ^= work;
	right = rotrFixed(right^work, 6U);
	work = (left ^ right) & 0x00ff00ff;
	left ^= work;
	right = rotlFixed(right^work, 9U);
	work = (left ^ right) & 0xaaaaaaaa;
	left = rotlFixed(left^work, 1U);
	right ^= work;
}

static inline void FPERM(word32 &left, word32 &right)
{
	word32 work;

	right = rotrFixed(right, 1U);
	work = (left ^ right) & 0xaaaaaaaa;
	right ^= work;
	left = rotrFixed(left^work, 9U);
	work = (left ^ right) & 0x00ff00ff;
	right ^= work;
	left = rotlFixed(left^work, 6U);
	work = (left ^ right) & 0x33333333;
	right ^= work;
	left = rotlFixed(left^work, 18U);
	work = (left ^ right) & 0xffff0000;
	right ^= work;
	left = rotlFixed(left^work, 20U);
	work = (left ^ right) & 0xf0f0f0f0;
	right ^= work;
	left = rotrFixed(left^work, 4U);
}

#ifndef CRYPTOPP_IMPORTS


/* Tables defined in the Data Encryption Standard documents
 * Three of these tables, the initial permutation, the final
 * permutation and the expansion operator, are regular enough that
 * for speed, we hard-code them. They're here for reference only.
 * Also, the S and P boxes are used by a separate program, gensp.c,
 * to build the combined SP box, Spbox[]. They're also here just
 * for reference.
 */
#ifdef notdef
/* initial permutation IP */
static byte ip[] = {
	   58, 50, 42, 34, 26, 18, 10,  2,
	   60, 52, 44, 36, 28, 20, 12,  4,
	   62, 54, 46, 38, 30, 22, 14,  6,
	   64, 56, 48, 40, 32, 24, 16,  8,
	   57, 49, 41, 33, 25, 17,  9,  1,
	   59, 51, 43, 35, 27, 19, 11,  3,
	   61, 53, 45, 37, 29, 21, 13,  5,
	   63, 55, 47, 39, 31, 23, 15,  7
};

/* final permutation IP^-1 */
static byte fp[] = {
	   40,  8, 48, 16, 56, 24, 64, 32,
	   39,  7, 47, 15, 55, 23, 63, 31,
	   38,  6, 46, 14, 54, 22, 62, 30,
	   37,  5, 45, 13, 53, 21, 61, 29,
	   36,  4, 44, 12, 52, 20, 60, 28,
	   35,  3, 43, 11, 51, 19, 59, 27,
	   34,  2, 42, 10, 50, 18, 58, 26,
	   33,  1, 41,  9, 49, 17, 57, 25
};
/* expansion operation matrix */
static byte ei[] = {
	   32,  1,  2,  3,  4,  5,
		4,  5,  6,  7,  8,  9,
		8,  9, 10, 11, 12, 13,
	   12, 13, 14, 15, 16, 17,
	   16, 17, 18, 19, 20, 21,
	   20, 21, 22, 23, 24, 25,
	   24, 25, 26, 27, 28, 29,
	   28, 29, 30, 31, 32,  1
};
/* The (in)famous S-boxes */
static byte sbox[8][64] = {
	   /* S1 */
	   14,  4, 13,  1,  2, 15, 11,  8,  3, 10,  6, 12,  5,  9,  0,  7,
		0, 15,  7,  4, 14,  2, 13,  1, 10,  6, 12, 11,  9,  5,  3,  8,
		4,  1, 14,  8, 13,  6,  2, 11, 15, 12,  9,  7,  3, 10,  5,  0,
	   15, 12,  8,  2,  4,  9,  1,  7,  5, 11,  3, 14, 10,  0,  6, 13,

	   /* S2 */
	   15,  1,  8, 14,  6, 11,  3,  4,  9,  7,  2, 13, 12,  0,  5, 10,
		3, 13,  4,  7, 15,  2,  8, 14, 12,  0,  1, 10,  6,  9, 11,  5,
		0, 14,  7, 11, 10,  4, 13,  1,  5,  8, 12,  6,  9,  3,  2, 15,
	   13,  8, 10,  1,  3, 15,  4,  2, 11,  6,  7, 12,  0,  5, 14,  9,

	   /* S3 */
	   10,  0,  9, 14,  6,  3, 15,  5,  1, 13, 12,  7, 11,  4,  2,  8,
	   13,  7,  0,  9,  3,  4,  6, 10,  2,  8,  5, 14, 12, 11, 15,  1,
	   13,  6,  4,  9,  8, 15,  3,  0, 11,  1,  2, 12,  5, 10, 14,  7,
		1, 10, 13,  0,  6,  9,  8,  7,  4, 15, 14,  3, 11,  5,  2, 12,

	   /* S4 */
		7, 13, 14,  3,  0,  6,  9, 10,  1,  2,  8,  5, 11, 12,  4, 15,
	   13,  8, 11,  5,  6, 15,  0,  3,  4,  7,  2, 12,  1, 10, 14,  9,
	   10,  6,  9,  0, 12, 11,  7, 13, 15,  1,  3, 14,  5,  2,  8,  4,
		3, 15,  0,  6, 10,  1, 13,  8,  9,  4,  5, 11, 12,  7,  2, 14,

	   /* S5 */
		2, 12,  4,  1,  7, 10, 11,  6,  8,  5,  3, 15, 13,  0, 14,  9,
	   14, 11,  2, 12,  4,  7, 13,  1,  5,  0, 15, 10,  3,  9,  8,  6,
		4,  2,  1, 11, 10, 13,  7,  8, 15,  9, 12,  5,  6,  3,  0, 14,
	   11,  8, 12,  7,  1, 14,  2, 13,  6, 15,  0,  9, 10,  4,  5,  3,

	   /* S6 */
	   12,  1, 10, 15,  9,  2,  6,  8,  0, 13,  3,  4, 14,  7,  5, 11,
	   10, 15,  4,  2,  7, 12,  9,  5,  6,  1, 13, 14,  0, 11,  3,  8,
		9, 14, 15,  5,  2,  8, 12,  3,  7,  0,  4, 10,  1, 13, 11,  6,
		4,  3,  2, 12,  9,  5, 15, 10, 11, 14,  1,  7,  6,  0,  8, 13,

	   /* S7 */
		4, 11,  2, 14, 15,  0,  8, 13,  3, 12,  9,  7,  5, 10,  6,  1,
	   13,  0, 11,  7,  4,  9,  1, 10, 14,  3,  5, 12,  2, 15,  8,  6,
		1,  4, 11, 13, 12,  3,  7, 14, 10, 15,  6,  8,  0,  5,  9,  2,
		6, 11, 13,  8,  1,  4, 10,  7,  9,  5,  0, 15, 14,  2,  3, 12,

	   /* S8 */
	   13,  2,  8,  4,  6, 15, 11,  1, 10,  9,  3, 14,  5,  0, 12,  7,
		1, 15, 13,  8, 10,  3,  7,  4, 12,  5,  6, 11,  0, 14,  9,  2,
		7, 11,  4,  1,  9, 12, 14,  2,  0,  6, 10, 13, 15,  3,  5,  8,
		2,  1, 14,  7,  4, 10,  8, 13, 15, 12,  9,  0,  3,  5,  6, 11
};

/* 32-bit permutation function P used on the output of the S-boxes */
static byte p32i[] = {
	   16,  7, 20, 21,
	   29, 12, 28, 17,
		1, 15, 23, 26,
		5, 18, 31, 10,
		2,  8, 24, 14,
	   32, 27,  3,  9,
	   19, 13, 30,  6,
	   22, 11,  4, 25
};
#endif

/* permuted choice table (key) */
static const byte pc1[] = {
	   57, 49, 41, 33, 25, 17,  9,
		1, 58, 50, 42, 34, 26, 18,
	   10,  2, 59, 51, 43, 35, 27,
	   19, 11,  3, 60, 52, 44, 36,

	   63, 55, 47, 39, 31, 23, 15,
		7, 62, 54, 46, 38, 30, 22,
	   14,  6, 61, 53, 45, 37, 29,
	   21, 13,  5, 28, 20, 12,  4
};

/* number left rotations of pc1 */
static const byte totrot[] = {
	   1,2,4,6,8,10,12,14,15,17,19,21,23,25,27,28
};

/* permuted choice key (table) */
static const byte pc2[] = {
	   14, 17, 11, 24,  1,  5,
		3, 28, 15,  6, 21, 10,
	   23, 19, 12,  4, 26,  8,
	   16,  7, 27, 20, 13,  2,
	   41, 52, 31, 37, 47, 55,
	   30, 40, 51, 45, 33, 48,
	   44, 49, 39, 56, 34, 53,
	   46, 42, 50, 36, 29, 32
};

/* End of DES-defined tables */

/* bit 0 is left-most in byte */
static const int bytebit[] = {
	   0200,0100,040,020,010,04,02,01
};

/* Set key (initialize key schedule array) */
void RawDES::UncheckedSetKey(CipherDir dir, const byte *key, unsigned int length)
{
	SecByteBlock buffer(56+56+8);
	byte *const pc1m=buffer;                 /* place to modify pc1 into */
	byte *const pcr=pc1m+56;                 /* place to rotate pc1 into */
	byte *const ks=pcr+56;
	register int i,j,l;
	int m;
	
	for (j=0; j<56; j++) {          /* convert pc1 to bits of key */
		l=pc1[j]-1;             /* integer bit location  */
		m = l & 07;             /* find bit              */
		pc1m[j]=(key[l>>3] &    /* find which key byte l is in */
			bytebit[m])     /* and which bit of that byte */
			? 1 : 0;        /* and store 1-bit result */
	}
	for (i=0; i<16; i++) {          /* key chunk for each iteration */
		memset(ks,0,8);         /* Clear key schedule */
		for (j=0; j<56; j++)    /* rotate pc1 the right amount */
			pcr[j] = pc1m[(l=j+totrot[i])<(j<28? 28 : 56) ? l: l-28];
		/* rotate left and right halves independently */
		for (j=0; j<48; j++){   /* select bits individually */
			/* check bit that goes to ks[j] */
			if (pcr[pc2[j]-1]){
				/* mask it in if it's there */
				l= j % 6;
				ks[j/6] |= bytebit[l] >> 2;
			}
		}
		/* Now convert to odd/even interleaved form for use in F */
		k[2*i] = ((word32)ks[0] << 24)
			| ((word32)ks[2] << 16)
			| ((word32)ks[4] << 8)
			| ((word32)ks[6]);
		k[2*i+1] = ((word32)ks[1] << 24)
			| ((word32)ks[3] << 16)
			| ((word32)ks[5] << 8)
			| ((word32)ks[7]);
	}
	
	if (dir==DECRYPTION)     // reverse key schedule order
		for (i=0; i<16; i+=2)
		{
			std::swap(k[i], k[32-2-i]);
			std::swap(k[i+1], k[32-1-i]);
		}
}

void RawDES::RawProcessBlock(word32 &l_, word32 &r_) const
{
	word32 l = l_, r = r_;
	const word32 *kptr=k;

	for (unsigned i=0; i<8; i++)
	{
		word32 work = rotrFixed(r, 4U) ^ kptr[4*i+0];
		l ^= Spbox[6][(work) & 0x3f]
		  ^  Spbox[4][(work >> 8) & 0x3f]
		  ^  Spbox[2][(work >> 16) & 0x3f]
		  ^  Spbox[0][(work >> 24) & 0x3f];
		work = r ^ kptr[4*i+1];
		l ^= Spbox[7][(work) & 0x3f]
		  ^  Spbox[5][(work >> 8) & 0x3f]
		  ^  Spbox[3][(work >> 16) & 0x3f]
		  ^  Spbox[1][(work >> 24) & 0x3f];

		work = rotrFixed(l, 4U) ^ kptr[4*i+2];
		r ^= Spbox[6][(work) & 0x3f]
		  ^  Spbox[4][(work >> 8) & 0x3f]
		  ^  Spbox[2][(work >> 16) & 0x3f]
		  ^  Spbox[0][(work >> 24) & 0x3f];
		work = l ^ kptr[4*i+3];
		r ^= Spbox[7][(work) & 0x3f]
		  ^  Spbox[5][(work >> 8) & 0x3f]
		  ^  Spbox[3][(work >> 16) & 0x3f]
		  ^  Spbox[1][(work >> 24) & 0x3f];
	}

	l_ = l; r_ = r;
}

void DES_EDE3::Base::UncheckedSetKey(CipherDir dir, const byte *key, unsigned int length)
{
	AssertValidKeyLength(length);

	m_des1.UncheckedSetKey(dir, key+(dir==ENCRYPTION?0:2*8));
	m_des2.UncheckedSetKey(ReverseCipherDir(dir), key+8);
	m_des3.UncheckedSetKey(dir, key+(dir==DECRYPTION?0:2*8));
}

void DES_EDE3::Base::ProcessAndXorBlock(const byte *inBlock, const byte *xorBlock, byte *outBlock) const
{
	word32 l,r;
	Block::Get(inBlock)(l)(r);
	IPERM(l,r);
	m_des1.RawProcessBlock(l, r);
	m_des2.RawProcessBlock(r, l);
	m_des3.RawProcessBlock(l, r);
	FPERM(l,r);
	Block::Put(xorBlock, outBlock)(r)(l);
}

#endif	// #ifndef CRYPTOPP_IMPORTS

NAMESPACE_END

////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// modes.cpp - written and placed in the public domain by Wei Dai


#ifndef CRYPTOPP_IMPORTS


NAMESPACE_BEGIN(CryptoPP)

void CipherModeBase::SetKey(const byte *key, unsigned int length, const NameValuePairs &params)
{
	UncheckedSetKey(params, key, length, GetIVAndThrowIfInvalid(params));	// the underlying cipher will check the key length
}

void CipherModeBase::GetNextIV(byte *IV)
{
	if (!IsForwardTransformation())
		throw NotImplemented("CipherModeBase: GetNextIV() must be called on an encryption object");

	m_cipher->ProcessBlock(m_register);
	memcpy(IV, m_register, BlockSize());
}

NAMESPACE_END

#endif
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
// randpool.cpp - written and placed in the public domain by Wei Dai
// The algorithm in this module comes from PGP's randpool.c


#ifndef CRYPTOPP_IMPORTS


NAMESPACE_BEGIN(CryptoPP)

typedef MDC<SHA> RandomPoolCipher;

RandomPool::RandomPool(unsigned int poolSize)
	: pool(poolSize), key(RandomPoolCipher::DEFAULT_KEYLENGTH)
{
	assert(poolSize > key.size());

	addPos=0;
	getPos=poolSize;
	memset(pool, 0, poolSize);
	memset(key, 0, key.size());
}

void RandomPool::Stir()
{
	CFB_Mode<RandomPoolCipher>::Encryption cipher;

	for (int i=0; i<2; i++)
	{
		cipher.SetKeyWithIV(key, key.size(), pool.end()-cipher.IVSize());
		cipher.ProcessString(pool, pool.size());
		memcpy(key, pool, key.size());
	}

	addPos = 0;
	getPos = key.size();
}

unsigned int RandomPool::Put2(const byte *inString, unsigned int length, int /* messageEnd */, bool /* blocking */)
{
	unsigned t;

	while (length > (t = pool.size() - addPos))
	{
		xorbuf(pool+addPos, inString, t);
		inString += t;
		length -= t;
		Stir();
	}

	if (length)
	{
		xorbuf(pool+addPos, inString, length);
		addPos += length;
		getPos = pool.size(); // Force stir on get
	}

	return 0;
}

unsigned int RandomPool::TransferTo2(BufferedTransformation &target, unsigned long &transferBytes, const std::string &channel, bool blocking)
{
	if (!blocking)
		throw NotImplemented("RandomPool: nonblocking transfer is not implemented by this object");

	unsigned int t;
	unsigned long size = transferBytes;

	while (size > (t = pool.size() - getPos))
	{
		target.ChannelPut(channel, pool+getPos, t);
		size -= t;
		Stir();
	}

	if (size)
	{
		target.ChannelPut(channel, pool+getPos, size);
		getPos += size;
	}

	return 0;
}

byte RandomPool::GenerateByte()
{
	if (getPos == pool.size())
		Stir();

	return pool[getPos++];
}

void RandomPool::GenerateBlock(byte *outString, unsigned int size)
{
	ArraySink sink(outString, size);
	TransferTo(sink, size);
}

NAMESPACE_END

#endif
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// rng.cpp - written and placed in the public domain by Wei Dai

#ifndef CRYPTOPP_IMPORTS

NAMESPACE_BEGIN(CryptoPP)

X917RNG::X917RNG(BlockTransformation *c, const byte *seed, unsigned long deterministicTimeVector)
	: cipher(c),
	  S(cipher->BlockSize()),
	  dtbuf(S),
	  randseed(seed, S),
	  randbuf(S),
	  randbuf_counter(0),
	  m_deterministicTimeVector(deterministicTimeVector)
{
	if (m_deterministicTimeVector)
	{
		memset(dtbuf, 0, S);
		memcpy(dtbuf, (byte *)&m_deterministicTimeVector, STDMIN((int)sizeof(m_deterministicTimeVector), S));
	}
	else
	{
		time_t tstamp1 = time(0);
		xorbuf(dtbuf, (byte *)&tstamp1, STDMIN((int)sizeof(tstamp1), S));
		cipher->ProcessBlock(dtbuf);
		clock_t tstamp2 = clock();
		xorbuf(dtbuf, (byte *)&tstamp2, STDMIN((int)sizeof(tstamp2), S));
		cipher->ProcessBlock(dtbuf);
	}
}

byte X917RNG::GenerateByte()
{
	if (randbuf_counter==0)
	{
		// calculate new enciphered timestamp
		if (m_deterministicTimeVector)
		{
			xorbuf(dtbuf, (byte *)&m_deterministicTimeVector, STDMIN((int)sizeof(m_deterministicTimeVector), S));
			while (++m_deterministicTimeVector == 0) {}	// skip 0
		}
		else
		{
			clock_t tstamp = clock();
			xorbuf(dtbuf, (byte *)&tstamp, STDMIN((int)sizeof(tstamp), S));
		}
		cipher->ProcessBlock(dtbuf);

		// combine enciphered timestamp with seed
		xorbuf(randseed, dtbuf, S);

		// generate a new block of random bytes
		cipher->ProcessBlock(randseed, randbuf);

		// compute new seed vector
		for (int i=0; i<S; i++)
			randseed[i] = randbuf[i] ^ dtbuf[i];
		cipher->ProcessBlock(randseed);

		randbuf_counter=S;
	}
	return(randbuf[--randbuf_counter]);
}

NAMESPACE_END

#endif
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// osrng.cpp - written and placed in the public domain by Wei Dai

// Thanks to Leonard Janke for the suggestion for AutoSeededRandomPool.


#ifndef CRYPTOPP_IMPORTS


#ifdef OS_RNG_AVAILABLE


#ifdef CRYPTOPP_WIN32_AVAILABLE
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#include <windows.h> // Do_not_auto_remove
#include <wincrypt.h> // Do_not_auto_remove
#endif

#ifdef CRYPTOPP_UNIX_AVAILABLE
#include <errno.h>
#include <fcntl.h>
#include <unistd.h> // Do_not_auto_remove (Needed for OpenBSD)
#endif

NAMESPACE_BEGIN(CryptoPP)

#if defined(NONBLOCKING_RNG_AVAILABLE) || defined(BLOCKING_RNG_AVAILABLE)
OS_RNG_Err::OS_RNG_Err(const std::string &operation)
	: Exception(OTHER_ERROR, "OS_Rng: " + operation + " operation failed with error " + 
#ifdef CRYPTOPP_WIN32_AVAILABLE
		"0x" + IntToString(GetLastError(), 16)
#else
		IntToString(errno)
#endif
		)
{
}
#endif

#ifdef NONBLOCKING_RNG_AVAILABLE

#ifdef CRYPTOPP_WIN32_AVAILABLE

MicrosoftCryptoProvider::MicrosoftCryptoProvider()
{
	if(!CryptAcquireContext(&m_hProvider, 0, 0, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
		throw OS_RNG_Err("CryptAcquireContext");
}

MicrosoftCryptoProvider::~MicrosoftCryptoProvider()
{
	CryptReleaseContext(m_hProvider, 0);
}

#endif

NonblockingRng::NonblockingRng()
{
#ifndef CRYPTOPP_WIN32_AVAILABLE
	m_fd = open("/dev/urandom",O_RDONLY);
	if (m_fd == -1)
		throw OS_RNG_Err("open /dev/urandom");
#endif
}

NonblockingRng::~NonblockingRng()
{
#ifndef CRYPTOPP_WIN32_AVAILABLE
	close(m_fd);
#endif
}

byte NonblockingRng::GenerateByte()
{
	byte b;
	GenerateBlock(&b, 1);
	return b;
}

void NonblockingRng::GenerateBlock(byte *output, unsigned int size)
{
#ifdef CRYPTOPP_WIN32_AVAILABLE
#	ifdef WORKAROUND_MS_BUG_Q258000
		static MicrosoftCryptoProvider m_Provider;
#	endif
	if (!CryptGenRandom(m_Provider.GetProviderHandle(), size, output))
		throw OS_RNG_Err("CryptGenRandom");
#else
	if ((unsigned int)read(m_fd, output, size) != size) {
		// Kernel 2.6.10 has non-concurrent access to /dev/urandom, retry at least once
		printf("Shamelessly retrying a random generation attempt\n");
		if ((unsigned int)read(m_fd, output, size) != size) {
			printf("Error reading /dev/urandom! (kernel 2.6.10?)\n");
			throw OS_RNG_Err("read /dev/urandom");
		}
	}
#endif
}

#endif

// *************************************************************

#ifdef BLOCKING_RNG_AVAILABLE

BlockingRng::BlockingRng()
{
	m_fd = open("/dev/random",O_RDONLY);
	if (m_fd == -1)
		throw OS_RNG_Err("open /dev/random");
}

BlockingRng::~BlockingRng()
{
	close(m_fd);
}

byte BlockingRng::GenerateByte()
{
	byte b;
	GenerateBlock(&b, 1);
	return b;
}

void BlockingRng::GenerateBlock(byte *output, unsigned int size)
{
	while (size)
	{
		// on some systems /dev/random will block until all bytes
		// are available, on others it will returns immediately
		int len = read(m_fd, output, STDMIN(size, (unsigned int)INT_MAX));
		if (len == -1)
			throw OS_RNG_Err("read /dev/random");
		size -= len;
		output += len;
		if (size)
			sleep(1);
	}
}

#endif

// *************************************************************

void OS_GenerateRandomBlock(bool blocking, byte *output, unsigned int size)
{
#ifdef NONBLOCKING_RNG_AVAILABLE
	if (blocking)
#endif
	{
#ifdef BLOCKING_RNG_AVAILABLE
		BlockingRng rng;
		rng.GenerateBlock(output, size);
#endif
	}

#ifdef BLOCKING_RNG_AVAILABLE
	if (!blocking)
#endif
	{
#ifdef NONBLOCKING_RNG_AVAILABLE
		NonblockingRng rng;
		rng.GenerateBlock(output, size);
#endif
	}
}

void AutoSeededRandomPool::Reseed(bool blocking, unsigned int seedSize)
{
	SecByteBlock seed(seedSize);
	OS_GenerateRandomBlock(blocking, seed, seedSize);
	Put(seed, seedSize);
}

CRYPTOPP_DLL_TEMPLATE_CLASS AutoSeededX917RNG<DES_EDE3>;

NAMESPACE_END

#endif

#endif
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
// md4.cpp - modified by Wei Dai from Andrew M. Kuchling's md4.c
// The original code and all modifications are in the public domain.

// This is the original introductory comment:

/*
 *  md4.c : MD4 hash algorithm.
 *
 * Part of the Python Cryptography Toolkit, version 1.1
 *
 * Distribute and use freely; there are no restrictions on further 
 * dissemination and usage except those imposed by the laws of your 
 * country of residence.
 *
 */


NAMESPACE_BEGIN(CryptoPP)

void MD4::InitState(HashWordType *state)
{
	state[0] = 0x67452301L;
	state[1] = 0xefcdab89L;
	state[2] = 0x98badcfeL;
	state[3] = 0x10325476L;
}

void MD4::Transform (word32 *digest, const word32 *in)
{
// #define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define F(x, y, z) ((z) ^ ((x) & ((y) ^ (z))))
#define G(x, y, z) (((x) & (y)) | ((x) & (z)) | ((y) & (z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))

    word32 A, B, C, D;

	A=digest[0];
	B=digest[1];
	C=digest[2];
	D=digest[3];

#define function(a,b,c,d,k,s) a=rotlFixed(a+F(b,c,d)+in[k],s);	 
	  function(A,B,C,D, 0, 3);
	  function(D,A,B,C, 1, 7);
	  function(C,D,A,B, 2,11);
	  function(B,C,D,A, 3,19);
	  function(A,B,C,D, 4, 3);
	  function(D,A,B,C, 5, 7);
	  function(C,D,A,B, 6,11);
	  function(B,C,D,A, 7,19);
	  function(A,B,C,D, 8, 3);
	  function(D,A,B,C, 9, 7);
	  function(C,D,A,B,10,11);
	  function(B,C,D,A,11,19);
	  function(A,B,C,D,12, 3);
	  function(D,A,B,C,13, 7);
	  function(C,D,A,B,14,11);
	  function(B,C,D,A,15,19);

#undef function	  
#define function(a,b,c,d,k,s) a=rotlFixed(a+G(b,c,d)+in[k]+0x5a827999,s);	 
	  function(A,B,C,D, 0, 3);
	  function(D,A,B,C, 4, 5);
	  function(C,D,A,B, 8, 9);
	  function(B,C,D,A,12,13);
	  function(A,B,C,D, 1, 3);
	  function(D,A,B,C, 5, 5);
	  function(C,D,A,B, 9, 9);
	  function(B,C,D,A,13,13);
	  function(A,B,C,D, 2, 3);
	  function(D,A,B,C, 6, 5);
	  function(C,D,A,B,10, 9);
	  function(B,C,D,A,14,13);
	  function(A,B,C,D, 3, 3);
	  function(D,A,B,C, 7, 5);
	  function(C,D,A,B,11, 9);
	  function(B,C,D,A,15,13);

#undef function	 
#define function(a,b,c,d,k,s) a=rotlFixed(a+H(b,c,d)+in[k]+0x6ed9eba1,s);	 
	  function(A,B,C,D, 0, 3);
	  function(D,A,B,C, 8, 9);
	  function(C,D,A,B, 4,11);
	  function(B,C,D,A,12,15);
	  function(A,B,C,D, 2, 3);
	  function(D,A,B,C,10, 9);
	  function(C,D,A,B, 6,11);
	  function(B,C,D,A,14,15);
	  function(A,B,C,D, 1, 3);
	  function(D,A,B,C, 9, 9);
	  function(C,D,A,B, 5,11);
	  function(B,C,D,A,13,15);
	  function(A,B,C,D, 3, 3);
	  function(D,A,B,C,11, 9);
	  function(C,D,A,B, 7,11);
	  function(B,C,D,A,15,15);

	digest[0]+=A;
	digest[1]+=B;
	digest[2]+=C;
	digest[3]+=D;
}

NAMESPACE_END
////////////////////////////////////////////////////////////////////////////////
// File_checked_for_headers
