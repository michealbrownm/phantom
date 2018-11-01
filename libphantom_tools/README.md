English 

# libphantom_tools

## Introduction
The phantom tool library is provided as a third-party library to implement operations such as wallet control, account management , and transaction signature. The features are listed below:
- Cross-platform. The c++ implementation is packaged into a third-party library for developers to use.
- Lightweight tool. Node data synchronization is not included.
- Safe operation. The private key information will not be revealed during use.

## Module Structure

Module name | Statement file | Function
|:--- | --- | ---
| `libphantom_tools` | [lib_phantom_tools.h](./lib_phantom_tools.h) | The functions of wallet control, account management, and transaction signature are implemented by the third-party library.
## Interface List

The interfaces implemented in the code include but are not limited to the following:
```
CreateAccountAddress   #Create a public-private key pair
CheckAccountAddressValid    #Check if the account number is correct
CreateKeystore  #Create a private key store
CheckKeystoreValid  #Check if the private key store is correct
SignData    #Sign with the private key
SignDataWithKeystore    #Sign with the private key storage
CheckSignedData     #Detect signature data
CreateKeystoreFromPrivkey   #Create a private key store using the private key
GetAddressFromPubkey    #Obtain the account address and the original public key address from the public key address
GetPrivatekeyFromKeystore   #Get the private key from the private key store
```


