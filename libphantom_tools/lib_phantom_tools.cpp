#include <string>
#include <stdio.h>
#include <json/json.h>
#include "lib_phantom_tools.h"
#include "common/private_key.h"
#include "common/key_store.h"
#include "common/data_secret_key.h"
#include "common/general.h"
#include "utils/logger.h"

PHANTOM_TOOLS_API int InitPhantomTools()
{
	utils::Logger::InitInstance();
	utils::Logger &logger = utils::Logger::Instance();
	logger.SetExpireDays(1);
	if (!logger.Initialize(utils::LOG_DEST_ALL, utils::LOG_LEVEL_ALL, "", false)){
		printf("Initialize logger failed\n");
		return -1;
	}

	return 0;
}

PHANTOM_TOOLS_API void UnInitPhantomTools()
{
	utils::Logger::Instance().Exit();
	utils::Logger::ExitInstance();
}

PHANTOM_TOOLS_API int CreateAccountAddress(const char *input_signtype, char *output_result, int *output_len) {
	if (input_signtype == NULL || strlen(input_signtype) == 0) {
		printf("input_signtype data or len error\n");
		return -1;
	}

	if (output_result == NULL || output_len == nullptr ||  (*output_len <= 0)) {
		printf("output_result data or len error\n");
		return -1;
	}

	std::string str_singtype(input_signtype);
	phantom::SignatureType type = phantom::GetSignTypeByDesc(str_singtype);
	if (type == phantom::SIGNTYPE_NONE) {
		printf("input_signtype \"%s\" error, support: ed25519 or sm2 \n", str_singtype.c_str());
		return -2;
	}

	phantom::PrivateKey priv_key(type);
	if (!priv_key.IsValid()) {
		printf("Generate private key error");
		return -2;
	}

	std::string public_key = priv_key.GetEncPublicKey();
	std::string private_key = priv_key.GetEncPrivateKey();
	std::string public_address = priv_key.GetEncAddress();

	//LOG_TRACE("Creating account address:%s", public_address.c_str());
	Json::Value result = Json::Value(Json::objectValue);
	result["public_key"] = public_key;
	result["private_key"] = private_key;
	result["private_key_aes"] = utils::Aes::CryptoHex(private_key, phantom::GetDataSecuretKey());
	result["address"] = public_address;
	result["public_key_raw"] = phantom::EncodePublicKey(priv_key.GetRawPublicKey());
	result["sign_type"] = GetSignTypeDesc(priv_key.GetSignType());
	//printf("%s\n", result.toStyledString().c_str());

	if (*output_len <= (int)result.toStyledString().size()) {
		printf("output_result len error\n");
		return -1;
	}

	sprintf(output_result, "%s", result.toStyledString().c_str());
	*output_len = result.toStyledString().size();
	return 0;
}

PHANTOM_TOOLS_API int CheckAccountAddressValid(const char *input_encode_address) {
	if (input_encode_address == NULL || strlen(input_encode_address) == 0) {
		printf("input data or len error\n");
		return -1;
	}

	std::string endcode_address(input_encode_address);
	return phantom::PublicKey::IsAddressValid(endcode_address) ? 0 : -2;
}

PHANTOM_TOOLS_API int CreateKeystore(const char *input_password, char *output_keystore, int *output_len) {
	if (input_password == NULL || strlen(input_password) == 0) {
		printf("input data or len error\n");
		return -1;
	}

	if (output_keystore == NULL || output_len == nullptr || (*output_len <= 0)) {
		printf("output data or len error\n");
		return -1;
	}

	std::string password(input_password);
	phantom::KeyStore key_store;
	std::string new_priv_key;
	Json::Value key_store_json;
	bool ret = key_store.Generate(password, key_store_json, new_priv_key);
	if (!ret) {
		return -2;
	}

	std::string key_store_result = key_store_json.toFastString();
	if (*output_len <= (int)key_store_result.size()) {
		return -1;
	}

	sprintf(output_keystore, "%s", key_store_result.c_str());
	*output_len = key_store_result.size();
	return 0;
}

PHANTOM_TOOLS_API int CheckKeystoreValid(const char *input_keystore, const char *input_password) {
	if (input_keystore == NULL || strlen(input_keystore) == 0) {
		printf("input_keystore data or len error\n");
		return -1;
	}

	if (input_password == NULL || strlen(input_password) == 0) {
		printf("input_password data or len error\n");
		return -1;
	}
	std::string str_keystore(input_keystore);
	std::string str_password(input_password);

	phantom::KeyStore key_store;
	Json::Value key_store_json;
	key_store_json.fromString(str_keystore);
	std::string private_key;
	bool ret = key_store.From(key_store_json, str_password, private_key);
	if (!ret) {
		return -2;
	}

	return 0;
}

PHANTOM_TOOLS_API int SignData(const char *input_privkey, const char *input_rawdata, char *output_data, int *output_len) {
	if (input_privkey == NULL || strlen(input_privkey) == 0) {
		printf("input_privkey data or len error\n");
		return -1;
	}

	if (input_rawdata == NULL || strlen(input_rawdata) == 0) {
		printf("input_rawdata data or len error\n");
		return -1;
	}

	if (output_data == NULL || output_len == nullptr || (*output_len <= 0)) {
		printf("output_data data or len error\n");
		return -1;
	}

	std::string str_priv_key(input_privkey);
	std::string str_rawdata(input_rawdata);

	phantom::PrivateKey priv_key(str_priv_key);
	std::string public_key = priv_key.GetEncPublicKey();
	std::string raw_data = utils::String::HexStringToBin(str_rawdata);
	Json::Value result = Json::Value(Json::objectValue);

	result["data"] = str_rawdata;
	result["public_key"] = public_key;
	result["sign_data"] = utils::String::BinToHexString(priv_key.Sign(raw_data));
	//printf("%s\n", result.toStyledString().c_str());

	if (*output_len <= (int)result.toStyledString().size()) {
		return -1;
	}

	if (result["sign_data"].asString().size() == 0) {
		return -2;
	}

	sprintf(output_data, "%s", result.toStyledString().c_str());
	*output_len = result.toStyledString().size();
	return 0;
}

PHANTOM_TOOLS_API int SignDataWithKeystore(const char *input_keystore, const char *input_password, const char *input_blob, char *output_data, int *output_len) {
	if (input_keystore == NULL || strlen(input_keystore) == 0) {
		printf("input_keystore data or len error\n");
		return -1;
	}
	if (input_password == NULL || strlen(input_password) == 0) {
		printf("input_password data or len error\n");
		return -1;
	}
	if (input_blob == NULL || strlen(input_blob) == 0) {
		printf("input_password data or len error\n");
		return -1;
	}
	if (output_data == NULL || output_len == nullptr || (*output_len <= 0)) {
		printf("output_data data or len error\n");
		return -1;
	}

	std::string str_key_store(input_keystore);
	std::string str_password(input_password);
	std::string str_blob(input_blob);

	phantom::KeyStore key_store;
	Json::Value key_store_json;
	key_store_json.fromString(str_key_store);
	std::string private_key;
	bool ret = key_store.From(key_store_json, str_password, private_key);
	if (!ret) {
		printf("error\n");
		return -2;
	}

	phantom::PrivateKey priv_key(private_key);
	std::string public_key = priv_key.GetEncPublicKey();
	std::string raw_data = utils::String::HexStringToBin(str_blob);
	Json::Value result = Json::Value(Json::objectValue);

	result["data"] = str_blob;
	result["public_key"] = public_key;
	result["sign_data"] = utils::String::BinToHexString(priv_key.Sign(raw_data));
	
	if (*output_len <= (int)result.toStyledString().size()) {
		return -1;
	}

	sprintf(output_data, "%s", result.toStyledString().c_str());
	*output_len = result.toStyledString().size();
	return 0;
}

PHANTOM_TOOLS_API int CheckSignedData(const char *input_blob, const char *input_signeddata, const char *input_pubkey) {
	if (input_blob == NULL || strlen(input_blob) == 0) {
		printf("input_blob data or len error\n");
		return -1;
	}
	if (input_signeddata == NULL || strlen(input_signeddata) == 0) {
		printf("input_signeddata data or len error\n");
		return -1;
	}
	if (input_pubkey == NULL || strlen(input_pubkey) == 0) {
		printf("input_pubkey data or len error\n");
		return -1;
	}

	std::string str_blob(input_blob);
	std::string str_signeddata(input_signeddata);
	std::string str_pubkey(input_pubkey);

	return phantom::PublicKey::Verify(utils::String::HexStringToBin(str_blob), utils::String::HexStringToBin(str_signeddata), str_pubkey) ? 0 : -2;
}

PHANTOM_TOOLS_API int CreateKeystoreFromPrivkey(const char *input_privkey, const char *input_password, char *output_data, int *output_len) {
	if (input_privkey == NULL || strlen(input_privkey) == 0) {
		printf("input_privkey data or len error\n");
		return -1;
	}
	if (input_password == NULL || strlen(input_password) == 0) {
		printf("input_password data or len error\n");
		return -1;
	}
	if (output_data == NULL || output_len == nullptr || (*output_len <= 0)) {
		printf("output_data data or len error\n");
		return -1;
	}

	std::string srt_password(input_password);
	std::string str_privkey(input_privkey);

	phantom::PrivateKey key(str_privkey);
	if (!key.IsValid()) {
		printf("error, private key not valid\n");
		return -2;
	}

	phantom::KeyStore key_store;
	Json::Value key_store_json;
	if (!key_store.Generate(srt_password, key_store_json, str_privkey)) {
		printf("error\n");
		return -2;
	}

	if (*output_len <= (int)key_store_json.toStyledString().size()) {
		return -1;
	}

	sprintf(output_data, "%s", key_store_json.toStyledString().c_str());
	*output_len = key_store_json.toStyledString().size();
	return 0;
}

int GetAddressFromPubkey(const char *input_pubkey, char *output_data, int *output_len) {
	if (input_pubkey == NULL || strlen(input_pubkey) == 0) {
		printf("input_pubkey data or len error\n");
		return -1;
	}

	if (output_data == NULL || *output_len <= 0) {
		printf("output_data data or len error\n");
		return -1;
	}
	std::string str_pubkey(input_pubkey);

	phantom::PublicKey pub_key(str_pubkey);
	std::string public_key_str = pub_key.GetEncPublicKey();
	std::string public_address = pub_key.GetEncAddress();
	Json::Value result = Json::Value(Json::objectValue);

	result["public_key"] = public_key_str;
	result["address"] = public_address;
	result["public_key_raw"] = phantom::EncodePublicKey(pub_key.GetRawPublicKey());
	result["sign_type"] = GetSignTypeDesc(pub_key.GetSignType());
	//printf("%s\n", result.toStyledString().c_str());

	if (result["public_key_raw"].asString().size() <= 0 || result["sign_type"].asString().size() <= 0) {
		printf("input_pubkey format error!\n");
		return -2;
	}

	if (*output_len <= (int)result.toStyledString().size()) {
		return -3;
	}

	sprintf(output_data, "%s", result.toStyledString().c_str());
	*output_len = result.toStyledString().size();
	return 0;
}

PHANTOM_TOOLS_API int GetPrivatekeyFromKeystore(const char *input_keystore, const char *input_password, char *output_data, int *output_len) {
	if (input_keystore == NULL || strlen(input_keystore) == 0) {
		printf("input_keystore data or len error\n");
		return -1;
	}

	if (input_password == NULL || strlen(input_password) == 0) {
		printf("input_password data or len error\n");
		return -1;
	}

	if (output_data == NULL || output_len == nullptr || (*output_len <= 0)) {
		printf("output_data data or len error\n");
		return -1;
	}

	std::string str_keystore(input_keystore);
	std::string str_password(input_password);

	phantom::KeyStore key_store;
	Json::Value key_store_json;
	key_store_json.fromString(str_keystore);
	std::string private_key;
	bool ret = key_store.From(key_store_json, str_password, private_key);
	if (!ret) {
		printf("error");
		return -2;
	}

	if (*output_len <= (int)private_key.size()){
		return -1;
	}

	sprintf(output_data, "%s", private_key.c_str());
	*output_len = private_key.size();
	return 0;
}
