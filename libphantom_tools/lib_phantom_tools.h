#ifndef LIB_PHANTOM_TOOLS_H
#define LIB_PHANTOM_TOOLS_H


#ifdef PHANTOM_TOOLS_EXPORTS
#define PHANTOM_TOOLS_API  extern "C" __declspec(dllexport)
#else
#ifdef PHANTOM_TOOLS_IMPORTS
#define PHANTOM_TOOLS_API  extern "C" __declspec(dllimport)
#else 
#define PHANTOM_TOOLS_API extern "C"
#endif
#endif

#ifdef LINUX
#define DEFAULT_VISIBILITY  __attribute__ ((visibility("default")))
#else
#define DEFAULT_VISIBILITY
#endif

/********************************************************************************
description:
	must be init lib 
return:
	0：OK，-1: data is null, other: ERR
********************************************************************************/
PHANTOM_TOOLS_API int InitPhantomTools();

/********************************************************************************
description:
	uninit lib
********************************************************************************/
PHANTOM_TOOLS_API void UnInitPhantomTools();

/********************************************************************************
description: 
	create an account address

input:
	input_signtype：sign type, support with "ed25519","sm2"

output:
	output_result：output json format data
	output_len：   first reset the output buffer's max len, and then it's output len

return:
	0：OK，-1: data is null, or data len is error, -2: parameter invalid, other: ERR
********************************************************************************/
PHANTOM_TOOLS_API int CreateAccountAddress(const char *input_signtype, char *output_result, int *output_len) DEFAULT_VISIBILITY;

/********************************************************************************
description:
	check validation of the account address 
return:
	0：OK，-1: data is null, or data len is error, -2: parameter invalid, other: ERR
********************************************************************************/
PHANTOM_TOOLS_API int CheckAccountAddressValid(const char *input_encode_address) DEFAULT_VISIBILITY;

/********************************************************************************
description:
	create keystore

output:
	output_keystore: json string
	output_len

return:
	0：OK，-1: data is null, or data len is error, -2: parameter invalid, other: ERR
********************************************************************************/
PHANTOM_TOOLS_API int CreateKeystore(const char *input_password, char *output_keystore, int *output_len) DEFAULT_VISIBILITY;

/********************************************************************************
description:
	check validation of keystore

input:
	input_keystore:json string
	input_password

return:
	0：OK，-1: data is null, or data len is error, -2: parameter invalid, other: ERR
********************************************************************************/
PHANTOM_TOOLS_API int CheckKeystoreValid(const char *input_keystore, const char *input_password) DEFAULT_VISIBILITY;

/********************************************************************************
description:
	sign data

return:
	0：OK，-1: data is null, or data len is error, -2: parameter invalid, other: ERR
********************************************************************************/
PHANTOM_TOOLS_API int SignData(const char *input_privkey, const char *input_rawdata, char *output_data, int *output_len) DEFAULT_VISIBILITY;

/********************************************************************************
description:
	sign data with keystore

return:
	0：OK，-1: data is null, or data len is error, -2: parameter invalid, other: ERR
********************************************************************************/
PHANTOM_TOOLS_API int SignDataWithKeystore(const char *input_keystore, const char *input_password, const char *input_blob, char *output_data, int *output_len) DEFAULT_VISIBILITY;

/********************************************************************************
description:
	CheckSignedData

return:
	0：OK，-1: data is null, or data len is error, -2: parameter invalid, other: ERR
********************************************************************************/
PHANTOM_TOOLS_API int CheckSignedData(const char *input_blob, const char *input_signeddata, const char *input_pubkey) DEFAULT_VISIBILITY;

/********************************************************************************
description:
	CreateKeystoreFromPrivkey
return:
	0：OK，-1: data is null, or data len is error, -2: parameter invalid, other: ERR
********************************************************************************/
PHANTOM_TOOLS_API int CreateKeystoreFromPrivkey(const char *input_privkey, const char *input_password, char *output_data, int *output_len) DEFAULT_VISIBILITY;

/********************************************************************************
description:
	GetAddressFromPubkey
return:
	0：OK，-1: data is null, or data len is error, -2: parameter invalid, other: ERR
********************************************************************************/
PHANTOM_TOOLS_API int GetAddressFromPubkey(const char *input_pubkey, char *output_data, int *output_len) DEFAULT_VISIBILITY;

/********************************************************************************
description:
	GetPrivatekeyFromKeystore
return:
	0：OK，-1: data is null, or data len is error, -2: parameter invalid, other: ERR
********************************************************************************/
PHANTOM_TOOLS_API int GetPrivatekeyFromKeystore(const char *input_keystore, const char *input_password, char *output_data, int *output_len) DEFAULT_VISIBILITY;

#endif
