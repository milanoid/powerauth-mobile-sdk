/*
 * Copyright 2016-2017 Wultra s.r.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#import "PA2Password.h"
#import "PA2OtpUtil.h"

#pragma mark - Session setup & Error -

/**
 The PA2SessionSetup object defines unique constants required during the lifetime
 of the Session class.
 */
@interface PA2SessionSetup : NSObject
/**
 Defines APPLICATION_KEY for the session.
 */
@property (nonatomic, strong, nonnull) NSString * applicationKey;
/**
 Defines APPLICATION_SECRET for the session.
 */
@property (nonatomic, strong, nonnull) NSString * applicationSecret;
/**
 The master server public key, in BASE64 format. It's strongly recommended to use
 different keys for the testing and production servers.
 */
@property (nonatomic, strong, nonnull) NSString * masterServerPublicKey;
/**
 Optional session identifier helps with session identification
 in multi-session environments. You can assign any value
 which helps you identify multiple sessions in your system.
 The session itself doesn't use this value.
 */
@property (nonatomic, assign) UInt32 sessionIdentifier;
/**
 Optional external encryption key. If the data object size is equal to 16 bytes,
 then the key is considered as valid and will be used during the cryptographic operations.
 
 The additional encryption key is useful in  multibanking applications, where it allows the
 application to create chain of trusted PA2 activations. If the key is set, then the session will
 perform additional encryption / decryption operations when the signature keys are being used.
 
 The session implements a couple of simple protections against misuse of this feature and therefore
 once the session is activated with the EEK, then you have to use that EEK for all future cryptographic
 operations. The key is NOT serialized in the session's state and thus it's up to the application,
 how it manages the chain of multiple PA2 sessions.
 */
@property (nonatomic, strong, nullable) NSData * externalEncryptionKey;

@end


/**
 The PA2CoreErrorCode enumeration defines all possible error codes
 produced by PA2Session and PA2Vault objects. You normally need 
 to check only if operation ended with EC_Ok or not. All other codes are
 only hints and should be used only for debugging purposes.
 
 For example, if the operation fails at PA2CoreErrorCode_WrongState or PA2CoreErrorCode_WrongParam,
 then it's usualy your fault and you're using the session in wrong way.
 */
typedef NS_ENUM(int, PA2CoreErrorCode) {
	/**
	 Everything is OK.
	 You can go out with your friends and enjoy the rest of the day :)
	 */
	PA2CoreErrorCode_Ok			= 0,
	/**
	 The method failed on an encryption. Whatever that means it's
	 usually very wrong and the UI response depends on what
	 method did you call. Typically, you have to perform retry
	 or restart for the whole process.
	 
	 This error code is also returned when decoding of important
	 parameter failed. For example, if BASE64 encoded value
	 is in wrong format, then this is considered as an attack
	 attempt.
	 */
	PA2CoreErrorCode_Encryption	= 1,
	/**
	 You have called method in wrong session's state. Usually that
	 means that you're using session in a  wrong way. This kind
	 of error should not be propagated to the UI. It's your
	 responsibility to handle session states correctly.
	 */
	PA2CoreErrorCode_WrongState	= 2,
	/**
	 You have called method with wrong or missing parameters.
	 Usually this error code means that you're using Session
	 in wrong way and you did not provide all required data.
	 This kind of error should not be propagated to UI. It's
	 your responsibility to handle all user's inputs
	 and validate all responses from the server before you
	 ask session for processing.
	 */
	PA2CoreErrorCode_WrongParam	= 3,
};


/**
 The PA2ProtocolVersion enum defines PowerAuth protocol version. The main difference
 between V2 & V3 is that V3 is using hash-based counter instead of linear one,
 and all E2EE tasks are now implemented by ECIES.
 
 This version of SDK is supporting V2 protol in very limited scope, where only
 the V2 signature calculations are supported. Basically, you cannot connect
 to V2 servers with V3 SDK.
 */
typedef NS_ENUM(int, PA2ProtocolVersion) {
	/**
	 Protocol version is not specified, or cannot be determined.
	 */
	PA2ProtocolVersion_NA = 0,
	/**
	 Protocol version 2
	 */
	PA2ProtocolVersion_V2 = 2,
	/**
	 Protocol version 3
	 */
	PA2ProtocolVersion_V3 = 3,
};


#pragma mark - Signatures -

/**
 The PA2SignatureFactor constants defines factors involved in the signature
 computation. The factor types are tightly coupled with the PASignatureUnlockKeys
 object.
 */
typedef int PA2SignatureFactor;
/**
 The possession factor, you have to provide possessionUnlocKey.
 */
extern const PA2SignatureFactor PA2SignatureFactor_Possession;
/**
 The knowledge factor, you have to provide userPassword
 */
extern const PA2SignatureFactor PA2SignatureFactor_Knowledge;
/**
 The biometry factor, you have to provide biometryUnlockKey.
 */
extern const PA2SignatureFactor PA2SignatureFactor_Biometry;
/**
 2FA, with using possession and knowledge factors.
 */
extern const PA2SignatureFactor PA2SignatureFactor_Possession_Knowledge;
/**
 2FA, with using possession and biometric factors.
 */
extern const PA2SignatureFactor PA2SignatureFactor_Possession_Biometry;
/**
 3FA, with using all supported factors.
 */
extern const PA2SignatureFactor PA2SignatureFactor_Possession_Knowledge_Biometry;

/**
 The PA2SignatureUnlockKeys object contains all keys, required for signature computation.
 You have to provide all keys involved into the signature computation, for selected combination
 of factors. For example, if you're going to compute signature for Possession + Biometry
 factor, then this object must contain valid possessionUnlockKey and biometryUnlockKey.
 
 Discussion

 Internally, the underlying Session keeps keys for signature computation always encrypted
 and doesn't expose these from the outside of the class. This very strict approach is
 a prevention against accidental sensitive information leakage. Your application has
 control only over the keys, which actually encrypts and decrypts this sensitive information.
 
 At first read, it looks like that this additional protection layer has no cryptographic benefit
 at all. Yes, this is basically true :) The purpose of this layer is just to simplify the Session's
 interface. In this approach, the exact state of the session is always fully serializable and the
 only application's responsibility is to provide the lock / unlock keys in the right time, 
 when these are really required. 
 
 As you can see, you still need to take care about how you're working with these unlock keys.
 Check the details below, each key type has its own rules how to construct the key or
 */
@interface PA2SignatureUnlockKeys : NSObject

/**
 The key required for signatures with "possession" factor.
 You have to provide a key based on the unique properties of the device.
 For example, WI-FI MAC address or UDID are a good sources for this
 key. You can use PA2Session::normalizeSignatureUnlockKeyFromData method
 to convert arbitrary data into normalized key.
 
 It is recommended to calculate this key for once, when the application starts
 and store in the volatile memory. You should never save this key to the
 permanent storage, like file system or keychain.
 
 You cannot use data object filled with zeros as a key.
 */
@property (nonatomic, strong, nullable) NSData * possessionUnlockKey;
/**
 The key required for signatures with "biometry" factor. You should not
 use this key and factor, if device has no biometric engine available.
 You can use PA2Session::generateSignatureUnlockKey for new key creation.
 
 You should store this key only to the storage, which can protect the
 key with using the biometry engine. For example, on iOS9+, you can use
 a keychain record, created with kSecAccessControlTouchID* flags.
 
 You cannot use data object filled with zeros as a key.
 */
@property (nonatomic, strong, nullable) NSData * biometryUnlockKey;
/**
 The password required for signatures with "knowledge" factor. The complexity
 of the password depends on the rules, defined by the application. You should 
 never store the password to the permanent storage (like file system, or keychain)
 
 The PA2Session validates only the minimum lenght of the password (check private
 Constants.h and MINIMAL_PASSWORD_LENGTH constant for details)
 */
@property (nonatomic, strong, nullable) PA2Password * userPassword;

@end


/**
 The PA2HTTPRequestData object contains all data required for calculating signature from
 HTTP request. You have to provide values at least non-empty strings to `method` and `uri` 
 members, to pass a data validation.
 */
@interface PA2HTTPRequestData : NSObject

/**
 A whole POST body or data blob prepared in 'Session::prepareKeyValueMapForDataSigning'
 method. You can also calculate signature for an empty request with no body or without
 any GET parameters. In this case the member may be empty.
 */
@property (nonatomic, strong, nullable) NSData * body;
/**
 HTTP method ("POST", "GET", "HEAD", "PUT", "DELETE" value is expected)
 */
@property (nonatomic, strong, nonnull) NSString * method;
/**
 Relative URI of the request.
 */
@property (nonatomic, strong, nonnull) NSString * uri;
/**
 Optional, contains NONCE generated externally. The value should be used for offline data
 signing purposes only. The Base64 string is expected.
 */
@property (nonatomic, strong, nullable) NSString * offlineNonce;

@end


/**
 The PA2HTTPRequestDataSignature object contains result from HTTP request data signing
 operation.
 */
@interface PA2HTTPRequestDataSignature : NSObject

/**
 Version of PowerAuth protocol.
 */
@property (nonatomic, strong, nonnull, readonly) NSString * version;
/**
 Activation identifier received during the activation process.
 */
@property (nonatomic, strong, nonnull, readonly) NSString * activationId;
/**
 Application key copied from Session.
 */
@property (nonatomic, strong, nonnull, readonly) NSString * applicationKey;
/**
 NONCE used for the signature calculation.
 */
@property (nonatomic, strong, nonnull, readonly) NSString * nonce;
/**
 String representation of signature factor or combination of factors.
 */
@property (nonatomic, strong, nonnull, readonly) NSString * factor;
/**
 Calculated signature
 */
@property (nonatomic, strong, nonnull, readonly) NSString * signature;
/**
 Contains a complete value for "X-PowerAuth-Authorization" HTTP header.
 */
@property (nonatomic, strong, nonnull, readonly) NSString * authHeaderValue;

@end

/**
 The PA2SigningDataKey enumeration defines key type used for signature calculation.
 */
typedef NS_ENUM(int, PA2SigningDataKey) {
	/**
	 `KEY_SERVER_MASTER_PRIVATE` key was used for signature calculation
	 */
	PA2SigningDataKey_ECDSA_MasterServerKey = 0,
	/**
	 `KEY_SERVER_PRIVATE` key was used for signature calculation
	 */
	PA2SigningDataKey_ECDSA_PersonalizedKey = 1,
};

/**
 The PA2SignedData object contains data and signature calculated from data.
 */
@interface PA2SignedData : NSObject

@property (nonatomic, assign) PA2SigningDataKey signingDataKey;
/**
 A data protected with signature
 */
@property (nonatomic, strong, nonnull) NSData * data;
/**
 A signagure calculated for data
 */
@property (nonatomic, strong, nonnull) NSData * signature;
/**
 A data protected with signature in Base64 format. The value is
 mapped to the `data` property.
 */
@property (nonatomic, strong, nonnull) NSString * dataBase64;
/**
 A signagure calculated for data in Base64 format. The value is
 mapped to the `signature` property.
 */
@property (nonatomic, strong, nonnull) NSString * signatureBase64;

@end


#pragma mark - Recovery codes -

/**
 RecoveryData object contains information about recovery code and PUK, created
 during the activation process.
 */
@interface PA2RecoveryData : NSObject
/**
 Contains recovery code.
 */
@property (nonatomic, strong, nonnull) NSString * recoveryCode;
/**
 Contains PUK, valid with recovery code.
 */
@property (nonatomic, strong, nonnull) NSString * puk;

@end


#pragma mark - Activation steps -

/**
 The PA2ActivationStep1Param object contains parameters for first step of device activation.
 */
@interface PA2ActivationStep1Param : NSObject

/**
 Full, parsed activation code. The parameter is optional and may be nil
 in case of custom activation.
 */
@property (nonatomic, strong, nullable) PA2Otp * activationCode;

@end


/**
 The PA2ActivationStep1Result object represents result from first
 step of the device activation.
 */
@interface PA2ActivationStep1Result : NSObject

/**
 Device's public key, in Base64 format
 */
@property (nonatomic, strong, nonnull) NSString * devicePublicKey;

@end


/**
 The PA2ActivationStep2Param contains parameters for second step of
 device activation
 */
@interface PA2ActivationStep2Param : NSObject

/**
 Real Activation ID received from server.
 */
@property (nonatomic, strong, nonnull) NSString * activationId;
/**
 Server's public key, in Base64 format.
 */
@property (nonatomic, strong, nonnull) NSString * serverPublicKey;
/**
 Initial value for hash-based counter.
 */
@property (nonatomic, strong, nonnull) NSString * ctrData;
/**
 If configured on the server, contains recovery data received from the server.
 */
@property (nonatomic, strong, nullable) PA2RecoveryData * activationRecovery;

@end


/**
 The PA2ActivationStep2Result object represent result from 2nd
 step of activation.
 */
@interface PA2ActivationStep2Result : NSObject

/**
 Short, human readable string, calculated from device's public key.
 You can display this code to the UI and user can confirm visually
 if the code is the same on both, server & client sides. This feature
 must be supported on the server's side of the activation flow.
 */
@property (nonatomic, strong, nonnull) NSString * activationFingerprint;

@end

#pragma mark - Activation status -

/**
 The PA2ActivationState enum defines all possible states of activation.
 The state is a part of information received together with the rest
 of the PA2ActivationStatus object.
 */
typedef NS_ENUM(int, PA2ActivationState) {
	/**
	 The activation is just created.
	 */
	PA2ActivationState_Created  = 1,
	/**
	 The OTP was already used.
	 */
	PA2ActivationState_OTP_Used = 2,
	/**
	 The shared secure context is valid and active.
	 */
	PA2ActivationState_Active   = 3,
	/**
	 The activation is blocked.
	 */
	PA2ActivationState_Blocked  = 4,
	/**
	 The activation doesn't exist anymore.
	 */
	PA2ActivationState_Removed  = 5,
};


/**
 The PA2ActivationStatus object represents complete status of the activation.
 The status is typically received as an encrypted blob and you can use module
 to decode that blob into this object.
 */
@interface PA2ActivationStatus : NSObject

/**
 State of the activation
 */
@property (nonatomic, assign, readonly) PA2ActivationState state;
/**
 Number of failed authentication attempts in a row.
 */
@property (nonatomic, assign, readonly) UInt32 failCount;
/**
 Maximum number of allowed failed authentication attempts in a row.
 */
@property (nonatomic, assign, readonly) UInt32 maxFailCount;
/**
 Contains (maxFailCount - failCount) if state is `PA2ActivationState_Active`,
 otherwise 0.
 */
@property (nonatomic, assign, readonly) UInt32 remainingAttempts;

@end

#pragma mark - End to End Encryption -

// Forward declaration for ECIES encryptor
@class PA2ECIESEncryptor;

/**
 The `PA2ECIESEncryptorScope` enumeration defines how `PA2ECIESEncryptor` encryptor is configured
 in `PA2Session.getEciesEncryptor()` method.
 */
typedef NS_ENUM(int, PA2ECIESEncryptorScope) {
	/**
	 An application scope means that encryptor can be constructed also when
	 the session has no valid activation.
	 */
	PA2ECIESEncryptorScope_Application  = 0,
	/**
	 An activation scope means that the encryptor can be constructed only when
	 the session has a valid activation.
	 */
	PA2ECIESEncryptorScope_Activation  = 1,
};
