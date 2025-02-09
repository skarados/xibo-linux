// pem-com.cpp - commom PEM routines. Written and placed in the public domain by Jeffrey Walton
//               Copyright assigned to the Crypto++ project.
//
// Crypto++ Library is copyrighted as a compilation and (as of version 5.6.2) licensed
// under the Boost Software License 1.0, while the individual files in the compilation
// are all public domain.

///////////////////////////////////////////////////////////////////////////
// For documentation on the PEM read and write routines, see
//   http://www.cryptopp.com/wiki/PEM_Pack
///////////////////////////////////////////////////////////////////////////

#include "cryptlib.h"
#include "secblock.h"
#include "base64.h"
#include "osrng.h"

#include <algorithm>
#include <cctype>
#include <cstring>

#include "pem.h"
#include "pem-com.h"

NAMESPACE_BEGIN(CryptoPP)

void PEM_WriteLine(BufferedTransformation& bt, const SecByteBlock& line)
{
    bt.Put(line.data(), line.size());
    bt.Put(reinterpret_cast<const byte*>(RFC1421_EOL.data()), RFC1421_EOL.size());
}

void PEM_WriteLine(BufferedTransformation& bt, const std::string& line)
{
    bt.Put(reinterpret_cast<const byte*>(line.data()), line.size());
    bt.Put(reinterpret_cast<const byte*>(RFC1421_EOL.data()), RFC1421_EOL.size());
}

void PEM_Base64Decode(BufferedTransformation& source, BufferedTransformation& dest)
{
    Base64Decoder decoder(new Redirector(dest));
    source.TransferTo(decoder);
    decoder.MessageEnd();
}

void PEM_Base64Encode(BufferedTransformation& source, BufferedTransformation& dest)
{
    Base64Encoder encoder(new Redirector(dest), true /*lineBreak*/, RFC1421_LINE_BREAK);
    source.TransferTo(encoder);
    encoder.MessageEnd();
}

SecByteBlock GetControlField(const SecByteBlock& line)
{
    SecByteBlock::const_iterator it = std::search(line.begin(), line.end(), SBB_COLON.begin(), SBB_COLON.end());
    if(it != line.end())
    {
        size_t len = it - line.begin();
        return SecByteBlock(line.data(), len);
    }

    return SecByteBlock();
}

SecByteBlock GetControlFieldData(const SecByteBlock& line)
{
    SecByteBlock::const_iterator it = std::search(line.begin(), line.end(), SBB_COLON.begin(), SBB_COLON.end());
    if(it != line.end() && ++it != line.end())
    {
        size_t len = line.end() - it;
        return SecByteBlock(it, len);
    }

    return SecByteBlock();
}

struct ByteToLower {
    byte operator() (byte val) {
        return (byte)std::tolower((int)(word32)val);
    }
};

// Returns 0 if a match, non-0 otherwise
int CompareNoCase(const SecByteBlock& first, const SecByteBlock& second)
{
    if(first.size() < second.size())
        return -1;
    else if(second.size() < first.size())
        return 1;

    // Same size... compare them....
#if (_MSC_VER >= 1500)
    SecByteBlock t1(first), t2(second);
    std::transform(t1.begin(), t1.end(), stdext::make_checked_array_iterator(t1.begin(), t1.size()), ByteToLower());
    std::transform(t2.begin(), t2.end(), stdext::make_checked_array_iterator(t2.begin(), t2.size()), ByteToLower());
#else
    SecByteBlock t1(first), t2(second);
    std::transform(t1.begin(), t1.end(), t1.begin(), ByteToLower());
    std::transform(t2.begin(), t2.end(), t2.begin(), ByteToLower());
#endif

    return std::memcmp(t1.begin(), t2.begin(), t2.size());
}

// From crypto/evp/evp_key.h. Signature changed a bit to match Crypto++.
int OPENSSL_EVP_BytesToKey(HashTransformation& hash,
    const unsigned char *salt, const unsigned char* data, size_t dlen,
    size_t count, unsigned char *key, size_t ksize,
    unsigned char *iv, size_t vsize)
{
    unsigned int niv,nkey,nhash;
    unsigned int addmd=0,i;

    nkey=static_cast<unsigned int>(ksize);
    niv = static_cast<unsigned int>(vsize);
    nhash = static_cast<unsigned int>(hash.DigestSize());

    SecByteBlock digest(hash.DigestSize());

    if (data == NULL) return (0);

    for (;;)
    {
        hash.Restart();

        if(addmd++)
            hash.Update(digest.data(), digest.size());

        hash.Update(data, dlen);

        if (salt != NULL)
            hash.Update(salt, OPENSSL_PKCS5_SALT_LEN);

        hash.TruncatedFinal(digest.data(), digest.size());

        for (i=1; i<count; i++)
        {
            hash.Restart();
            hash.Update(digest.data(), digest.size());
            hash.TruncatedFinal(digest.data(), digest.size());
        }

        i=0;
        if (nkey)
        {
            for (;;)
            {
                if (nkey == 0) break;
                if (i == nhash) break;
                if (key != NULL)
                    *(key++)=digest[i];
                nkey--;
                i++;
            }
        }
        if (niv && (i != nhash))
        {
            for (;;)
            {
                if (niv == 0) break;
                if (i == nhash) break;
                if (iv != NULL)
                    *(iv++)=digest[i];
                niv--;
                i++;
            }
        }
        if ((nkey == 0) && (niv == 0)) break;
    }

    return static_cast<int>(ksize);
}

NAMESPACE_END
