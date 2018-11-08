//
//  main.m
//  ios
//
//  Created by Feng Ruiming on 2018/6/22.
//  Copyright © 2018 phantom. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "AppDelegate.h"
#import "protocol/Chain.pbobjc.h"

int main(int argc, char * argv[]) {
    @autoreleasepool {
        Transaction *transaction = [Transaction message];
        transaction.nonce = 1;
        transaction.sourceAddress = @"buQiu6i3aVP4SXBNmPsvJZxwYEcEBHUZd4Wj";
        transaction.gasPrice = 1000;
        transaction.feeLimit = 999999999897999986;
        
        Operation *operation = [Operation message];
        operation.sourceAddress = @"buQiu6i3aVP4SXBNmPsvJZxwYEcEBHUZd4Wj";
        
        OperationCreateAccount *createAccount = [OperationCreateAccount message];
        createAccount.destAddress = @"buQpCTN3x6K4pAyboF4C1CoUYbr2ooqRyCjZ";
        createAccount.initBalance = 999999999897999986;
        
        operation.createAccount = createAccount;
        
        [transaction.operationsArray addObject: operation];
        NSLog(@"before: %@", transaction);
        
        // Serialization
        NSData *serialData = transaction.data;
        
        // De-serialization
        Transaction *tran = [Transaction parseFromData:serialData error:NULL];
        NSLog(@"after: %@", tran);
        
        
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
    }
    
}
