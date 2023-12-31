#include <web_token.hpp>

#include <json.hpp>

#if __has_include(<uva/binary.hpp>)
#   include <uva/binary.hpp>

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

    bool uva::json::web_token::decode(const std::string &text, var &output)
    {
        const char* header_end = text.c_str();

        while(*header_end && *header_end != '.') {
            ++header_end;
        }

        std::string header_base64_view(text.c_str(), header_end - text.c_str());

        while(header_base64_view.size() % 4 != 0) {
            header_base64_view.push_back('=');
        }

        std::string decoded_header = uva::binary::decode_base64<std::string>(header_base64_view, true);

        const char* payload_data_begin = 1 + header_end;
        const char* payload_data_end = payload_data_begin;

        while(*payload_data_end && *payload_data_end != '.') {
            ++payload_data_end;
        }

        std::string_view payload_base64_view(payload_data_begin, payload_data_end - payload_data_begin);

        std::string decoded_payload = uva::binary::decode_base64<std::string>(payload_base64_view, true);

        const char* signature_begin = payload_data_end + 1;

        std::string_view payload_signature_view(signature_begin, signature_begin - text.c_str());

        std::string decoded_signature = uva::binary::decode_base64<std::string>(payload_signature_view, true);

        output = var::map({
            { "header",    json::decode(decoded_header) },
            { "payload",   json::decode(decoded_payload) },
            { "signature", decoded_signature },
        });

        return true;
    }
#endif