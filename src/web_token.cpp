#include <web_token.hpp>

#include <json.hpp>
#include <binary.hpp>

std::string base64_json(const std::map<var, var>& j) {
    return uva::binary::encode_base64(uva::json::enconde(j), false);
}

std::string uva::json::web_token::enconde(const std::map<var, var> &values, const std::string &secret)
{
    static std::string header = base64_json({
        { "typ", "JWT" },
        { "alg", "HS256" },
    });

    std::string token = header;
    token.push_back('.');
    token += base64_json(values);
#ifdef __UVA_OPENSSL_FOUND
    uva::binary::binary_uint256_t sha256 = uva::binary::hmac_sha256(token, secret);
    std::string signature = uva::binary::encode_base64(sha256, false);

    token.reserve(signature.size()+1);
    token.push_back('.');
    token += signature;
#endif

    return token;
}

bool uva::json::web_token::decode(const std::string &text, const std::string &secret, var &output)
{
    return false;
}
