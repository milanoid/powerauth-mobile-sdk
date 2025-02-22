# PowerAuth Mobile SDK for Android

## Table of Contents

- [SDK Installation](#installation)
- [SDK Configuration](#configuration)
- [Device Activation](#activation)
   - [Activation via Activation Code](#activation-via-activation-code)
   - [Activation via Custom Credentials](#activation-via-custom-credentials)
   - [Activation via Recovery Code](#activation-via-recovery-code)
   - [Committing Activation Data](#committing-activation-data)
   - [Validating user inputs](#validating-user-inputs)
- [Requesting Device Activation Status](#requesting-activation-status)
- [Data Signing](#data-signing)
  - [Symmetric Multi-Factor Signature](#symmetric-multi-factor-signature)
  - [Asymmetric Private Key Signature](#asymmetric-private-key-signature)
  - [Symmetric Offline Multi-Factor Signature](#symmetric-offline-multi-factor-signature)
  - [Verify server signed data](#verify-server-signed-data)
- [Password Change](#password-change)
- [Fingerprint Authentication Setup](#fingerprint-authentication-setup)
- [Device Activation Removal](#activation-removal)
- [End-To-End Encryption](#end-to-end-encryption)
- [Secure Vault](#secure-vault)
- [Recovery Codes](#recovery-codes)
   - [Getting Recovery Data](#getting-recovery-data)
   - [Confirm Recovery Postcard](#confirm-recovery-postcard)
- [Token Based Authentication](#token-based-authentication)
- [Common SDK Tasks](#common-sdk-tasks)
- [Additional Features](#additional-features)
  - [Password Strength Indicator](#password-strength-indicator)
  - [Debug Build Detection](#debug-build-detection)
  - [Request Interceptors](#request-interceptors)
  
    
## Installation

To get PowerAuth SDK for Android up and running in your app, add following dependency in your `gradle.build` file:

```gradle
repositories {
    jcenter() // if not defined elsewhere...
}

dependencies {
    compile 'io.getlime.security.powerauth:powerauth-android-sdk:1.0.0'
}
```
Note that this documentation is using version `1.0.0` as an example. You can find the latest version in our [List of Releases](https://github.com/wultra/powerauth-crypto/blob/develop/docs/Releases.md). The Android Studio IDE can also find and offer updates for your application's dependencies.

From now on, you can use `PowerAuthSDK` class in your project.

To use development version, you need to install it in your local Maven repository and make this repository available to your Gradle script.

```sh
$ git clone --recurse-submodules https://github.com/wultra/powerauth-mobile-sdk.git
$ cd powerauth-mobile-sdk/proj-android
$ sh build-publish-local.sh
```

In your Gradle script:

```gradle
apply plugin: "maven"

repositories {
    mavenLocal()
}
```

You also need to install base Java modules in your maven repository:

```sh
$ git clone --recurse-submodules https://github.com/wultra/powerauth-restful-integration.git
$ cd powerauth-restful-integration
$ mvn clean install -DskipTests=true
```

## Configuration

In order to be able to configure your `PowerAuthSDK` instance, you need following values from the PowerAuth Server:

- `APP_KEY` - Application key, that binds activation with specific application.
- `APP_SECRET` - Application secret, that binds activation with specific application.
- `KEY_MASTER_SERVER_PUBLIC` - Master Server Public Key, used for non-personalized encryption and server signature verification.

Also, you need to specify your instance ID (by default, this can be for example an app package name). This is because one application may use more than one custom instances of `PowerAuthSDK` and identifier is the way to distinguish these instances while working with Keychain data.

Finally, you need to know the location of your [PowerAuth Standard RESTful API](https://github.com/wultra/powerauth-crypto/blob/develop/docs/Standard-RESTful-API.md) endpoints. That path should contain everything that goes before the `/pa/**` prefix of the API endpoints.

To sum it up, in order to configure `PowerAuthSDK` default instance, add following code to your application main activity `onCreate()` method:

```java
String INSTANCE_ID = getApplicationContext().getPackageName();
String PA_APPLICATION_KEY = "sbG8gd...MTIzNA==";
String PA_APPLICATION_SECRET = "aGVsbG...MTIzNA==";
String PA_MASTER_SERVER_PUBLIC_KEY = "MTIzNDU2Nz...jc4OTAxMg==";
String API_SERVER = "https://localhost:8080/demo-server";

final PowerAuthConfiguration configuration = new PowerAuthConfiguration.Builder(
        INSTANCE_ID,
        API_SERVER,
        PA_APPLICATION_KEY,
        PA_APPLICATION_SECRET,
        PA_MASTER_SERVER_PUBLIC_KEY)
        .build();

PowerAuthSDK powerAuthSDK = new PowerAuthSDK.Builder(configuration)
        .build(getApplicationContext());
```

## Activation

After you configure the SDK instance, you are ready to make your first activation.

### Activation via Activation Code

The original activation method uses a one-time activation code generated in PowerAuth Server. To create an activation using this method, some external application (Internet banking, ATM application, branch / kiosk application) must generate an activation code for you and display it (as a text or in a QR code).

In case you would like to use QR code scanning to enter an activation code, you can use any library of your choice, for example [Barcode Scanner](https://github.com/dm77/barcodescanner) open-source library based on ZBar lib.

Use following code to create an activation once you have an activation code:

```java
String deviceName = "Petr's Leagoo T5C";
String activationCode = "VVVVV-VVVVV-VVVVV-VTFVA"; // let user type or QR-scan this value

// Create a new activation with given device name and activation code
powerAuthSDK.createActivation(deviceName, activationCode, new ICreateActivationListener() {
    @Override
    public void onActivationCreateSucceed(CreateActivationResult result) {
        final String fingerprint = result.getActivationFingerprint();
        final RecoveryData activationRecovery = result.getRecoveryData();
        // No error occurred, proceed to credentials entry (PIN prompt, Enable "Fingerprint Authentication" switch, ...) and commit
        // The 'fingerprint' value represents the combination of device and server public keys - it may be used as visual confirmation
        // If server supports recovery codes for activation, then `activationRecovery` contains object with information about activation recovery.
    }

    @Override
    public void onActivationCreateFailed(Throwable t) {
        // Error occurred, report it to the user
    }
});
```

If the received activation result also contains recovery data, then you should display that values to the user. To do that, please read [Getting Recovery Data](#getting-recovery-data) section of this document, which describes how to treat that sensitive information. This is relevant for all types of activation you use.

### Activation via Custom Credentials

You may also create an activation using any custom login data - it can be anything that server can use to obtain user ID to associate with a new activation. Since the credentials are custom, the server's implementation must be able to process such request. Unlike the previous versions of SDK, the custom activation no longer requires a custom activation endpoint.

Use following code to create an activation using custom credentials:

```java
// Create a new activation with given device name and login credentials
String deviceName = "Juraj's JiaYu S3";
Map<String, String> credentials = new HashMap<>();
credentials.put("username", "john.doe@example.com");
credentials.put("password", "YBzBEM");

powerAuthSDK.createActivation(deviceName, credentials, null, null, new ICreateActivationListener() {
    @Override
    public void onActivationCreateSucceed(CreateActivationResult result) {
        final String fingerprint = result.getActivationFingerprint();
        final RecoveryData activationRecovery = result.getRecoveryData();
        // No error occurred, proceed to credentials entry (PIN prompt, Enable "Fingerprint Authentication" switch, ...) and commit
        // The 'fingerprint' value represents the combination of device and server public keys - it may be used as visual confirmation
        // If server supports recovery codes for activation, then `activationRecovery` contains object with information about activation recovery.
    }

    @Override
    public void onActivationCreateFailed(Throwable t) {
        // Error occurred, report it to the user
    }
});
```

Note that by using weak identity attributes to create an activation, the resulting activation is confirming a "blurry identity". This may greately limit the legal weight and usability of a signature. We recommend using a strong identity verification before an activation can actually be created.


### Activation via Recovery Code

If PowerAuth Server is configured to support [Recovery Codes](https://github.com/wultra/powerauth-crypto/blob/develop/docs/Activation-Recovery.md), then also you can create an activation via the recovery code and PUK. 

Use following code to create an activation using recovery code:

```java
final String deviceName = "John Tramonta"
final String recoveryCode = "55555-55555-55555-55YMA" // User's input
final String puk = "0123456789" // User's input. You should validate RC & PUK with using OtpUtil 

powerAuthSDK.createRecoveryActivation(deviceName, recoveryCode, puk, null, null, new ICreateActivationListener() {
    @Override
    public void onActivationCreateSucceed(CreateActivationResult result) {
        final String fingerprint = result.getActivationFingerprint();
        final RecoveryData activationRecovery = result.getRecoveryData();
        // No error occurred, proceed to credentials entry (PIN prompt, Enable "Fingerprint Authentication" switch, ...) and commit
        // The 'fingerprint' value represents the combination of device and server public keys - it may be used as visual confirmation
        // If server supports recovery codes for activation, then `activationRecovery` contains object with information about activation recovery.
    }

    @Override
    public void onActivationCreateFailed(Throwable t) {
        // Error occurred, report it to the user
        // On top of a regular error processing, you should handle a special situation, when server gives an additional information
        // about which PUK must be used for the recovery. The information is valid only when recovery code from a postcard is applied.
        if (t instanceof ErrorResponseApiException) {
            ErrorResponseApiException exception = (ErrorResponseApiException) t;
            Error errorResponse = exception.getErrorResponse();
            int currentRecoveryPukIndex = errorResponse.getCurrentRecoveryPukIndex();
            if (currentRecoveryPukIndex > 0) {
                // The PUK index is known, you should inform user that it has to rewrite PUK from a specific position. 
            }
        }
    }
});
```

### Committing Activation Data

After you create an activation using one of the methods mentioned above, you need to commit the activation - to use provided user credentials to store the activation data on the device. Use following code to do this.

```java
// Commit activation using given PIN
int result = powerAuthSDK.commitActivationWithPassword(context, pin);
if (result != PowerAuthErrorCodes.PA2Succeed) {
    // happens only in case SDK was not configured or activation is not in state to be committed
}
```

This code has created activation with two factors: possession (key stored using a key derived from a device fingerprint) and knowledge (password, in our case a simple PIN code). If you would like to enable fingerprint authentication support at this moment, use following code instead of the one above:

```java
// Commit activation using given PIN and ad-hoc generated biometric related key
powerAuthSDK.commitActivation(context, fragmentManager, "Enable Fingerprint Authentication", "To enable fingerprint authentication, touch the fingerprint sensor on your device.", pin, new ICommitActivationWithFingerprintListener() {
    @Override
    public void onFingerprintDialogCancelled() {
        // Fingerprint enrolment cancelled by user
    }

    @Override
    public void onFingerprintDialogSuccess() {
        // success, activation has been committed
    }
    
    @Override
    public void onFingerprintDialogFailed(PowerAuthErrorException error) {
        // failure, typically as a result of API misuse.
    }
});
```

Also, you can use following code to create activation with the best granularity control:

```java
PowerAuthAuthentication authentication = new PowerAuthAuthentication();
authentication.usePossession = true;
authentication.usePassword = "1234";
authentication.useBiometry = encryptedBiometryKey;

int result =  powerAuthSDK.commitActivationWithAuthentication(context, authentication);
if (result != PowerAuthErrorCodes.PA2Succeed) {
    // happens only in case SDK was not configured or activation is not in state to be committed
}
```

Note that you currently need to obtain the encrypted biometry key yourself - you have to use `FingerprintManager.CryptoObject` or integration with Android `KeyStore` to do so.



### Validating user inputs

The mobile SDK is providing a couple of functions in `OtpUtil` class, helping with user input validation. You can:

- Parse activation code when it's scanned from QR code
- Validate a whole code at once
- Validate recovery code or PUK
- Auto-correct characters typed on the fly

#### Validating scanned QR code

To validate an activation code scanned from QR code, you can use `OtpUtil.parseFromActivationCode()` function. You have to provide the code, with or without the signature part. For example:

```java
final String scannedCode = "VVVVV-VVVVV-VVVVV-VTFVA#aGVsbG8.....gd29ybGQ=";
final Otp otp = OtpUtil.parseFromActivationCode(scannedCode);
if (otp == null || otp.activationCode == null) {
    // Invalid code, QR code should contain a signature
    return;
}
```

Note that the signature is only formally validated in the function above. The actual signature verification is done in the activation process, or you can do it on your own:

```java
final String scannedCode = "VVVVV-VVVVV-VVVVV-VTFVA#aGVsbG8......gd29ybGQ=";
final Otp otp = OtpUtil.parseFromActivationCode(scannedCode);
if (otp == null || otp.activationCode == null) { 
    return; 
}
final byte[] codeBytes = otp.activationCode.getBytes(Charset.defaultCharset());
final byte[] signatureBytes = Base64.decode(otp.activationSignature, Base64.NO_WRAP);
if (!powerAuthSDK.verifyServerSignedData(codeBytes, signatureBytes, true)) {
    // Invalid signature
}
```

#### Validating entered activation code

To validate an activation code at once, you can call `OtpUtil.validateActivationCode()` function. You have to provide the code, without the signature part. For example:

```java
boolean isValid   = OtpUtil.validateActivationCode("VVVVV-VVVVV-VVVVV-VTFVA");
boolean isInvalid = OtpUtil.validateActivationCode("VVVVV-VVVVV-VVVVV-VTFVA#aGVsbG8gd29ybGQ=");
```

If your application is using your own validation, then you should switch to functions provided by SDK. The reason for that is that since SDK `1.0.0`, all activation codes contains a checksum, so it's possible to detect mistyped characters before you start the activation. Check our [Activation Code](https://github.com/wultra/powerauth-crypto/blob/develop/docs/Activation-Code.md) documentation for more details.

#### Validating recovery code and PUK

To validate a recovery code at once, you can call `OtpUtil.validateRecoveryCode()` function. You can provide the whole code, which may, or may not contain `"R:"` prefix. So, you can validate manually entered codes, but also codes scanned from QR. For example:

```java
boolean isValid1 = OtpUtil.validateRecoveryCode("VVVVV-VVVVV-VVVVV-VTFVA");
boolean isValid2 = OtpUtil.validateRecoveryCode("R:VVVVV-VVVVV-VVVVV-VTFVA");
```

To validate PUK at once, you can call `OtpUtil.validateRecoveryPuk()` function:

```java
boolean isValid   = OtpUtil.validateRecoveryPuk("0123456789");
```

#### Auto-correcting typed characters

You can implement auto-correcting of typed characters with using `OtpUtil.validateAndCorrectTypedCharacter()` function in screens, where user is suppose to enter an activation or recovery code. This technique is possible due to fact, that Base32 is specially constructed that doesn't contain visually confusing characters. For example, `1` (number one) and `I` (capital I) are confusing, so only `I` is allowed. The benefit is that provided function can correct typed `1` and translate it to `I`. 

Here's an example how to iterate over the string and validate it character by character:

```java
/// Returns corrected character or null in case of error.
@Nullable String validateTypedCharacters(@NonNull String input) {
    final int length = input.length();
    final StringBuilder output = new StringBuilder(length);
    for (int offset = 0; offset < length; ) {
        final int codepoint = input.codePointAt(offset);
        offset += Character.charCount(codepoint);
        final int corrected = OtpUtil.validateAndCorrectTypedCharacter(codepoint);
        if (corrected == 0) {
            return null;
        }
        // Character.isBmpCodePoint(corrected) is always true
        output.append((char)corrected);
    }
    return output.toString();
}

// validateTypedCharacter("v1") == "VI"
// validateTypedCharacter("9") == null
```


## Requesting Activation Status

To obtain a detailed activation status information, use following code:

```java
// Check if there is some activation on the device
if (powerAuthSDK.hasValidActivation()) {

    // If there is an activation on the device, check the status with server
    powerAuthSDK.fetchActivationStatusWithCallback(context, new IActivationStatusListener() {
        @Override
        public void onActivationStatusSucceed(ActivationStatus status) {
            // Activation state: State_Created, State_OTP_Used, State_Active, State_Blocked, State_Removed
            int state = status.state;

            // Failed login attempts, remaining = max - current
            int currentFailCount = status.failCount;
            int maxAllowedFailCount = status.maxFailCount;
            int remainingFailCount = maxAllowedFailCount - currentFailCount;

        }

        @Override
        public void onActivationStatusFailed(Throwable t) {
            // Network error occurred, report it to the user
        }
    });
} else {
    // No activation present on device
}
```

Note that the status fetch may fail at an unrecoverable error `PowerAuthErrorCodes.PA2ErrorCodeProtocolUpgrade`, meaning that it's not possible to upgrade PowerAuth protocol to a newer version. In this case, it's recommended to [remove the activation locally](#activation-removal).

## Data Signing

The main feature of PowerAuth protocol is data signing. PowerAuth has two types of signatures:

- **Symmetric Multi-Factor Signature**: Suitable for most operations, such as login, new payment or confirming changes in settings.
- **Asymmetric Private Key Signarture**: Suitable for documents, where strong one sided signature is desired.
- **Symmetric Offline Multi-Factor Signature**: Suitable for very secure operations, where the signature is validated over the out-of-band channel.
- **Verify server signed data**: Suitable for receiving arbitrary data from the server.

### Symmetric Multi-Factor Signature

To sign request data, you need to first obtain user credentials (password, PIN code, fingerprint scan) from the user. The task of obtaining the user credentials is used in more use-cases covered by the SDK. The core class is `PowerAuthAuthentication`, that holds information about used authentication factors:

```java
// 2FA signature, uses device related key and user PIN code.
// To use biometry, you need to fetch the encrypted biometry key value using `FingerprintManager.CryptoObject`.
PowerAuthAuthentication authentication = new PowerAuthAuthentication();
authentication.usePossession = true;
authentication.usePassword = "1234";
authentication.useBiometry = null;
```

When signing `POST`, `PUT` or `DELETE` requests, use request body bytes (UTF-8) as request data and following code:

```java
// 2FA signature, uses device related key and user PIN code
PowerAuthAuthentication authentication = new PowerAuthAuthentication();
authentication.usePossession = true;
authentication.usePassword = "1234";
authentication.useBiometry = null;

// Sign POST call with provided data made to URI with custom identifier "/payment/create"
PowerAuthAuthorizationHttpHeader header = powerAuthSDK.requestSignatureWithAuthentication(context, authentication, "POST", "/payment/create", requestBodyBytes);
if (header.isValid()) {
    String httpHeaderKey = header.getKey();
    String httpHeaderValue = header.getValue();
} else {
    // In case of invalid configuration, invalid activation state or corrupted state data
}
```

When signing `GET` requests, use the same code as above with normalized request data as described in specification, or (preferrably) use the following helper method:

```java
// 2FA signature, uses device related key and user PIN code
PowerAuthAuthentication authentication = new PowerAuthAuthentication();
authentication.usePossession = true;
authentication.usePassword = "1234";
authentication.useBiometry = null;

// Sign GET call with provided query parameters made to URI with custom identifier "/payment/create"
Map<String, String> params = new HashMap<>();
params.put("param1", "value1");
params.put("param2", "value2");

PowerAuthAuthorizationHttpHeader header = powerAuthSDK.requestGetSignatureWithAuthentication(context, authentication, "/payment/create", params);
if (header.isValid()) {
    String httpHeaderKey = header.getKey();
    String httpHeaderValue = header.getValue();
} else {
    // In case of invalid configuration, invalid activation state or corrupted state data
}
```

The result of the signature is appropriate HTTP header - you are responsible for hooking up the header value in your request correctly. The process with libraries like `OkHttp` goes like this:

```java
// Prepare the request builder
final Request.Builder builder = new Request.Builder().url(endpoint);

// Compute PA signature header
PowerAuthAuthorizationHttpHeader header = powerAuthSDK.requestSignatureWithAuthentication(context, signatureUnlockKeys, "POST", "/session/login", jsonBody);
if (!header.isValid()) {
    // request signature failed, for example due to incorrect activation status - cancel the process
    return;
 }

// Add HTTP header in the request builder
builder.header(header.getKey(), header.getValue());

// Build the request, send it and process response...
// ...
```

#### Request Synchronization

It is recommended that your application executes only one signed request at the time. The reason for that is that our signature scheme is using a counter as a representation of logical time. In other words, the order of request validation on the server is very important. If you issue more that one signed request at the same time, then the order is not guaranteed and therefore one from the requests may fail. On top of that, Mobile SDK itself is using this type of signatures for its own purposes. For example, if you ask for token, then the SDK is using signed request to obtain the token's data. To deal with this problem, Mobile SDK is providing a custom serial `Executor`, which can be used for signed requests execution: 

```java
final Executor serialExecutor = powerAuthSDK.getSerialExecutor();
serialExecutor.execute(new Runnable() {
    @Override
    public void run() {
        // Recommended practice:
        // 1. You have to calculate PowerAuth signature here.
        // 2. In case that you start yet another asynchronous operation from run(),
        //    then you have to wait for that operation's execution.
    }
});
```

### Asymmetric Private Key Signature

Asymmetric Private Key Signature uses a private key stored in the PowerAuth secure vault. In order to unlock the secure vault and retrieve the private key, user has to be first authenticated using a symmetric multi-factor signature with at least two factors. This mechanism protects the private key on the device - server plays a role of a "doorkeeper" and holds the vault unlock key.

This process is completely transparent on the SDK level. To compute an asymmetric private key signature, request user credentials (password, PIN, fingerprint scan) and use following code:

```java
// Prepare the authentication object
PowerAuthAuthentication authentication = new PowerAuthAuthentication();
authentication.usePossession = true;
authentication.usePassword = "1234";
authentication.useBiometry = null;

// Get the data to be signed
byte[] data = this.getMyData();

powerAuthSDK.signDataWithDevicePrivateKey(context, authentication, data, new IDataSignatureListener() {
    @Override
    public void onDataSignedSucceed(byte[] signature) {
        // Use data signature...
    }

    @Override
    public void onDataSignedFailed(Throwable t) {
        // Report error
    }
});
```

### Symmetric Offline Multi-Factor Signature

This type of signature is very similar to [Symmetric Multi-Factor Signature](#symmetric-multi-factor-signature) but the result is provided in form of simple, human readable string (unlike the online version, where the result is HTTP header). To calculate the signature you need a typical `PowerAuthAuthentication` object to define all required factors, nonce and data to sign. The `nonce` and `data` should be also transmitted to the application over the OOB channel (for example by scanning a QR code). Then the signature calculation is straightforward:

```java
// Prepare the authentication object
PowerAuthAuthentication authentication = new PowerAuthAuthentication();
authentication.usePossession = true;
authentication.usePassword = "1234";

final String signature = powerAuthSDK.offlineSignatureWithAuthentication(context, authentication, "/confirm/offline/operation", data, nonce);
if (signature != null) {
    android.util.Log.d(TAG, "Offline signature is: " + signature);
} else {
    // failure: session is probably invalid, or some required data is missing
}
```

Now the application has to show that calculated signature to the user and user has to re-type that code into the web application for the verification.

### Verify server signed data

This task is useful when you need to receive an arbitrary data from the server and you need to be sure that you need to be sure that data has has been issued by the server. The PowerAuthSDK is providing a high level method for validating data and associated signature:  

```java
// Validate data signed with the master server key
if (powerAuthSDK.verifyServerSignedData(data, signature, true)) {
    // data is signed with server's private master key
}
// Validate data signed with the personalized server key
if (powerAuthSDK.verifyServerSignedData(data, signature, false)) {
    // data is signed with server's private key
}
```

## Password Change

Since the device does not know the password and is unable to verify the password without the help of the server side, you need to first call an endpoint that verifies a signature computed with the password. SDK offers two ways to do that.

The safe, but typically slower way is to use following code:

```java
// Change password from "oldPassword" to "newPassword".
powerAuthSDK.changePassword(context, "oldPassword", "newPassword", new IChangePasswordListener() {
    @Override
    public void onPasswordChangeSucceed() {
        // Password was changed
    }

    @Override
    public void onPasswordChangeFailed(Throwable t) {
        // Error occurred
    }
})
```

This method calls `/pa/v3/signature/validate` under the hood with a 2FA signature with provided original password to verify the password correctness.

However, using this method does not usually fit to the typical UI workflow of a password change. The method may be used in cases where old password and new password are on a single screen, and therefore are both available at the same time. In most mobile apps, however, user first visits a screen to enter an old password and then (if the password is OK), the user proceeds to the two-screen flow of a new password setup (select password, confirm password). In other words, the workflow works like this:

1. Show a screen to enter old password.
2. Check old password on the server.
3. If the old password is OK, then let user chose and confirm a new one.
4. Change the password by "recrypting" the activation data.

For this purpose, you can use following code:

```java
// Ask for old password
String oldPassword = "1234";

// Validate password on the server
powerAuthSDK.validatePasswordCorrect(context, oldPassword, new IValidatePasswordListener() {
    @Override
    public void onPasswordValid() {
        // Proceed to the new password setup
    }

    @Override
    public void onPasswordValidationFailed(Throwable t) {
        // Retry entering an old password
    }
});

// ...

// Ask for new password
String newPassword = "2468";

// Change the password locally
powerAuthSDK.changePasswordUnsafe(oldPassword, newPassword);
```

**Now, beware!** Since the device does not know the actual old password, you need to make sure that the old password is validated before you use it in `unsafeChangePassword`. In case you provide a wrong old password, it will be used to decrypt the original data and these data will be encrypted using a new password. As a result, the activation data will be broken and irreversibly lost.

## Fingerprint Authentication Setup

PowerAuth SDK for Android provides an abstraction on top of the base Fingerprint Authentication support. While the authentication / data signing itself is handled using `PowerAuthAuthentication` object used in [regular request signing](#data-signing), other related processes require their own API.

### Check Fingerprint Authentication Status

You have to check for Fingerprint Authentication on three levels:

- **System Availability**: If fingerprint scanner is present on the system.
- **Activation Availability**: If biometry factor data are available for given activation.
- **Application Availability**: If user decided to use fingerprint authentication for given app. _(optional)_

PowerAuth SDK for Android provides code for the first and second of these checks.

To check if you can use fingerprint authentication on the system, use Adnroid `FingerprintManager` class directly, or our helper class:

```java
// This method is equivalent to `hasFingerprintHardware && hasEnrolledFingerprints`.
// Use it to check of fingerprint authentication can be used at the moment.
boolean isFingerprintAuthAvailable = FingerprintUtilities.isFingerprintAuthAvailable(context);

// For more fine-grained control about the actual fingerprint authentication status,
// you may use these two methods separately.
boolean hasFingerprintHardware = FingerprintUtilities.hasFingerprintHardware(context);
boolean hasEnrolledFingerprints = FingerprintUtilities.hasEnrolledFingerprints(context);
```

To check if given activation has biometry factor related data available, use following code:

```java
// Does activation have biometric factor related data in place?
boolean hasBiometryFactor = powerAuthSDK.hasBiometryFactor(context);
```

The last check is fully under your control. By keeping the fingerprint settings flag, for example a `BOOL` in `SharedPreferences`, you are able to show expected user fingerprint authentication status (in disabled state, though) even in the case fingerprint authentication is not enabled or when no fingers are enrolled on the device.

### Enable Fingerprint Authentication

In case an activation does not yet have biometry related factor data and you would like to enable fingerprint authentication support, device must first retrieve the original private key from the secure vault for the purpose of key derivation. As a result, you have to use successful 2FA with password to enable fingerprint authentication support.

Use following code to enable biometric authentication using fingerprint authentication:

```java
// Establish biometric data using provided password
powerAuthSDK.addBiometryFactor(context, fragmentManager, "Enable Fingerprint Authentication", "To enable fingerprint authentication, touch the fingerprint sensor on your device.", "1234", new IAddBiometryFactorListener() {
    @Override
    public void onAddBiometryFactorSucceed() {
        // Everything went OK, fingerprint authentication is ready to be used
    }

    @Override
    public void onAddBiometryFactorFailed(Throwable t) {
        // Error occurred, report it to user
    }
});
```

### Disable fingerprint authentication

You can remove biometry related factor data used by fingerprint authentication support by simply removing the related key locally, using this one-liner:

```java
// Remove bimetric data
powerAuthSDK.removeBiometryFactor(context);
```

### Fetching the Biometry Factor Related Key for Authentication

In order to obtain an encrypted biometry factor related key for the purpose of authentication, call following code:

```java
// Authenticate user with fingerprint and obtain encrypted biometry factor related key.
powerAuthSDK.authenticateUsingFingerprint(context, fragmentManager, "Sign in", "Confirm fingerprint to continue", new IFingerprintActionHandler() {
    @Override
    public void onFingerprintDialogCancelled() {
        // User cancelled the operation
    }

    @Override
    public void onFingerprintDialogSuccess(@Nullable byte[] encryptedBiometryKey) {
        // User authenticated and biometry key was returned
    }

    @Override
    public void onFingerprintInfoDialogClosed() {
        // User closed an information dialog with some fingerprint authentication related message
    }
});
```

## Activation Removal

You can remove activation using several ways - the choice depends on a desired behavior.

### Simple Device-Only Removal

You can clear activation data anytime from `SharedPreferences`. The benefit of this method is that it does not require help from server and user does not have to be logged in. The issue with this removal method is simple: The activation still remains active on the server side. This, however, does not have to be an issue in your case.

To remove only data related to PowerAuth SDK for Android, use following code:

```java
powerAuthSDK.removeActivationLocal(context);
```

### Removal via Authenticated Session

In case your server uses an authenticated session for keeping user logged in, you can combine the previous method with calling your proprietary endpoint that removes activation for currently logged in user. The advantage of this method is that activation does not remain active on the server. The issue is that user has to be logged in (the session must be active and must have activation ID stored) and that you have to publish your own method for the purpose of this use case.

The code for this activation removal method is as follows:

```java
// Use custom call to proprietary server endpoint to remove activation.
// User must be logged in at this moment, so that session can find
// associated activation ID
this.httpClient.post(null, "/custom/activation/remove", new ICustomListener() {
    @Override
    public void onSucceed() {
        powerAuthSDK.removeActivationLocal(context);
    }

    @Override
    public void onFailed(Throwable t) {
        // Error occurred, report it to user
    }
});
```

### Removal via Signed Request

PowerAuth Standard RESTful API has a default endpoint `/pa/v3/activation/remove` for an activation removal. This endpoint uses a signature verification for looking up the activation to be removed. The benefit of this method is that it is already present in both PowerAuth SDK for Android and PowerAuth Standard RESTful API - nothing has to be programmed. Also, user does not have to be logged in to use it. However, user has to authenticate using 2FA with either password or fingerprint authentication.

Use following code for an activation removal using signed request:

```java
PowerAuthAuthentication authentication = new PowerAuthAuthentication();
authentication.usePossession = true;
authentication.usePassword = "1234";
authentication.useBiometry = null;

// Remove activation using provided authentication object
powerAuthSDK.removeActivationWithAuthentication(context, authentication, new IActivationRemoveListener() {
    @Override
    public void onActivationRemoveSucceed() {
        // OK, activation was removed
    }

    @Override
    public void onActivationRemoveFailed(Throwable t) {
        // Report error to user
    }
})
```

## End-To-End Encryption

Currently, PowerAuth SDK supports two basic modes of end-to-end encryption, based on ECIES scheme:

- In "application" scope, the encryptor can be acquired and used during the whole lifetime of the application. We used to call this mode as "non-personalized encryption" in the previous versions of SDK.
- In "activation" scope, the encryptor can be acquired only if `PowerAuthSDK` has a valid activation. The encryptor created for this mode is cryptographically bound to the parameters, agreed during the activation process. You can combine this encryption with [PowerAuth Symmetric Multi-Factor Signature](#symmetric-multi-factor-signature), in "sign-then-encrypt" mode. 


For both scenarios, you need to acquire `EciesEncryptor` object, which will then provide interface for the request encryption and the response decryption. The object currently provides only low level encryption and decryption methods, so you need to implement your own JSON (de)serialization and request and response processing.

Following steps are typically required for a full E2EE request and response processing:

1. Acquire the right encryptor from `PowerAuthSDK` instance. For example:
   ```java
   // Encryptor for "application" scope.
   final EciesEncryptor encryptor = powerAuthSDK.getEciesEncryptorForApplicationScope();
   // ...or similar, for an "activation" scope.
   final EciesEncryptor encryptor = powerAuthSDK.getEciesEncryptorForActivationScope(context);
   ```

2. Serialize your request payload, if needed, into sequence of bytes. This step typically means that you need to serialize your model object into JSON formatted sequence of bytes.

3. Encrypt your payload:
   ```java
   final EciesCryptogram cryptogram = encryptor.encryptRequest(payloadData);
   if (cryptogram == null) {
       // cannot encrypt data
   }
   ```

4. Construct a JSON from provided cryptogram object. The dictionary with following keys is expected:
   - `ephemeralPublicKey` property fill with `cryptogram.getKeyBase64()`
   - `encryptedData` property fill with `cryptogram.getBodyBase64()`
   - `mac` property fill with `cryptogram.getMacBase64()`
   
   So, the final request JSON should looks like:
   ```json
   {
      "ephemeralPublicKey" : "BASE64-DATA-BLOB",
      "encryptedData": "BASE64-DATA-BLOB",
      "mac" : "BASE64-DATA-BLOB"
   }
   ```
   
5. Add following HTTP header (for signed requests, see note below):
   ```java
   // Acquire a "metadata" object, which contains an additional information for the request construction
   final EciesMetadata metadata = encryptor.getMetadata();
   final String httpHeaderName = metadata.getHttpHeaderKey();
   final String httpHeaderValue = metadata.getHttpHeaderValue();
   ```
   *Note, that if "activation" scoped encryptor is combined with PowerAuth Symmetric Multi-Factor signature, then this step is not required. The signature's header already contains all information required for proper request decryption on the server.* 
   
6. Fire your HTTP request and wait for a response
   - In case that non-200 HTTP status code is received, then the error processing is identical to a standard RESTful response, defined in our protocol. So, you can expect JSON object with `"error"` and `"message"` properties in the response.

7. Decrypt the response. The received JSON typically looks like:
   ```json
   {
      "encryptedData": "BASE64-DATA-BLOB",
      "mac" : "BASE64-DATA-BLOB"
   }
   ```
   So, you need to create yet another "cryptogram" object, but with only two properties set:
   ```java
   final EciesCryptogram responseCryptogram = new EciesCryptogram(response.getEncryptedData(), response.getMac());
   final byte[] responseData = encryptor.decryptResponse(responseCryptogram);
   if (responseData == null) {
       // failed to decrypt response data
   }
   ```

8. And finally, you can process your received response.

As you can see, the E2EE is quite non-trivial task. We recommend you to contact us before you even consider to use an application-specific E2EE. We can provide you more support on per-scenario basis, especially if we understand first, what you need to achieve with end-to-end encryption in your application.


## Secure Vault

PowerAuth SDK for iOS has a basic support for an encrypted secure vault. At this moment, the only supported method allows application to establish an encryption / decryption key with given index. Index represents a "key number" - your identifier for given key. Different business logic purposes should have encryption keys with different index value.

On a server side, all secure vault related work is concentrated in a `/pa/v3/vault/unlock` endpoint of PowerAuth Standard RESTful API. In order to receive data from this response, call must be authenticated with at least 2FA (using password or PIN).

### Obtaining Encryption Key

In order to obtain an encryption key with given index, use following code:

```java
// 2FA signature, uses device related key and user PIN code
PowerAuthAuthentication authentication = new PowerAuthAuthentication();
authentication.usePossession = true;
authentication.usePassword = "1234";
authentication.useBiometry = null;

// Select custom key index
long index = 1000L;

// Fetch encryption key with given index
powerAuthSDK.fetchEncryptionKey(context, authentication, index, new IFetchEncryptionKeyListener() {
    @Override
    public void onFetchEncryptionKeySucceed(byte[] encryptedEncryptionKey) {
        // ... use encryption key to encrypt or decrypt data
    }

    @Override
    public void onFetchEncryptionKeyFailed(Throwable t) {
        // Report error
    }
})
```

## Recovery Codes

The recovery codes allows your users to recover their activation in case that mobile device is lost or stolen. Before you start, please read [Activation Recovery](https://github.com/wultra/powerauth-crypto/blob/develop/docs/Activation-Recovery.md) document, available in our [powerauth-crypto](https://github.com/wultra/powerauth-crypto) repository.

To recover an activation, the user has to re-type two separate values:

1. Recovery Code itself, which is very similar to an activation code. So you can detect typing errors before you submit such code to the server. 
1. PUK, which is an additional numeric value and acts as an one time password in the scheme.

PowerAuth currently supports two basic types of recovery codes:

1. Recovery Code bound to a previous PowerAuth activation.
   - This type of code can be obtained only in an already activated application.
   - This type of code has only one PUK available, so only one recovery operation is possible.
   - The activation associated with the code is removed once the recovery operation succeeds.
  
2. Recovery Code delivered via OOB channel, typically in form of securely printed postcard, delivered by a post service.
   - This type of code has typically more than one PUK associated with the code, so it can be used for multiple times.
   - User has to keep that postcard at safe and secure place and mark already used PUKs.
   - The code delivery must be confirmed by the user, before it can be used for a recovery operation.

The feature is not automatically available, but must be enabled and configured on PowerAuth Server. If it's so, then your mobile application can use several methods related to this feature.

### Getting Recovery Data

If the recovery data was received during the activation process, then you can later display that information to the user. To check existence of recovery data and get that information, use following code:

```java
if (!powerAuthSDK.hasActivationRecoveryData()) {
    // Recovery information is not available
    return;
}

// 2FA signature, uses device related key and user PIN code
PowerAuthAuthentication authentication = new PowerAuthAuthentication();
authentication.usePossession = true;
authentication.usePassword = "1234";
authentication.useBiometry = null;

powerAuthSDK.getActivationRecoveryData(context, authentication, new IGetRecoveryDataListener() {
    @Override
    public void onGetRecoveryDataSucceeded(RecoveryData recoveryData) {
        final String recoveryCode = recoveryData.recoveryCode;
        final String puk = recoveryData.puk;
        // Show values on the screen...
    }
    
    @Override
    public void onGetRecoveryDataFailed(Throwable t) {
        // Report error
    }
});
```

WARNING: The obtained information is very sensitive, so you should be very careful how your application manipulate with that received values:

- You should never store `recoveryCode` or `puk` on the device.
- You should never print that values to the debug log.
- You should never send that values over the network.
- You should never copy that values to the clipboard.
- Do not cache that values in RAM.
- Your UI logic should require PIN every time the vales are going to display on the screen.
- Your application should not allow taking screenshots when values are displayed on the screen.

You should inform user that:

- Making screenshot when values are displayed on the screen is dangerous (in case that you did not disable taking screenshots).
- User should write down that values on paper and keep it as much safe as possible for future use.


### Confirm Recovery Postcard

The recovery postcard can contain the recovery code and multiple PUK values on one printed card. Due to security reasons, this kind of recovery code cannot be used for the recovery operation before user confirms its physical delivery. To confirm such recovery code, use following code:

```java
// 2FA signature with possession factor is required
PowerAuthAuthentication authentication = new PowerAuthAuthentication();
authentication.usePossession = true;
authentication.usePassword = "1234";
authentication.useBiometry = null;

final String recoveryCode = "VVVVV-VVVVV-VVVVV-VTFVA" // You can also use code scanned from QR
powerAuthSDK.confirmRecoveryCode(context, authentication, recoveryCode, new IConfirmRecoveryCodeListener{
    @Override
    public void onRecoveryCodeConfirmed(boolean alreadyConfirmed) {
        if (alreadyConfirmed) {
            android.util.Log.d(TAG, "Recovery code has been already confirmed. This is not an error, just information.");
        } else {
            android.util.Log.d(TAG, "Recovery code has been successfully confirmed.");
        }
    }

    @Override
    public void onRecoveryCodeConfirmFailed(Throwable t) {
        // Report error
    }
});
```

The `alreadyConfirmed` boolean indicates that code was already confirmed in past. You can choose a different "success" screen, describing that user has already confirmed such code. Also note that codes bound to the activations are already confirmed.

## Token Based Authentication

WARNING: Before you start using access tokens, please visit our [wiki page for powerauth-crypto](https://github.com/wultra/powerauth-crypto/blob/develop/docs/MAC-Token-Based-Authentication.md) for more information about this feature.

The tokens are simple, locally cached objects, producing timestamp-based authorization headers. Be aware that tokens are NOT a replacement for general PowerAuth signatures, but are helpful in situations, when the signatures are too heavy or too complicated for implementation. Each token has following properties:

- It needs PowerAuth signature for its creation (e.g. you need to provide `PowerAuthAuthentication` object)
- It has unique identifier on the server. This identifier is not exposed to the public API, but you can reveal that value in the debugger.
- It has symbolic name (e.g. "MyToken") defined by the application programmer to identify already created tokens.
- It can generate timestamp based authorization HTTP headers.
- Can be used concurrently. Token's private data doesn't change in time.
- The token is associated with the `PowerAuthSDK` instance. So, you can use the same symbolic name in multiple SDK instances and each created token will be unique.
- Tokens are persisted in the `PA2Keychain` service and cached in the memory.
- Once the parent `PowerAuthSDK` instance loose its activation, then all its tokens are removed from the local database.

### Getting token

To get an access token, you can use following code:

```java
// 1FA signature, uses device related key
PowerAuthAuthentication authentication = new PowerAuthAuthentication();
authentication.usePossession = true;

final PowerAuthTokenStore tokenStore = powerAuthSDK.getTokenStore();
final AsyncTask task = tokenStore.requestAccessToken(context, "MyToken", authentication, new IGetTokenListener() {
    @Override
    public void onGetTokenSucceeded(@NonNull PowerAuthToken powerAuthToken) {
        // the token has been successfully acquired
    }

    @Override
    public void onGetTokenFailed(@NonNull Throwable throwable) {
        // an error occured
    }
});
```

The request is performed synchronously or asynchronously depending on whether the token is locally cached on the device. You can test this situation by calling `tokenStore.hasLocalToken(context, "MyToken")`. If operation is asynchronous, then `requestAccessToken()` returns cancellable task. Be aware that you should not issue multiple asynchronous operations for the same token name.

### Generating authorization header

Once you have a `PowerAuthToken` object, use following code to generate an authorization header:

```java
PowerAuthAuthorizationHttpHeader header = token.generateHeader();
if (header.isValid()) {
    // Header is valid, you can construct HTTP header...
    String httpHeaderKey = header.key;
    String httpHeaderValue = header.value;
} else {
    // handle error
}
```

### Removing token from server

To remove token from the server, you can use following code:

```java
final PowerAuthTokenStore tokenStore = powerAuthSDK.getTokenStore();
tokenStore.removeAccessToken(context, "MyToken", new IRemoveTokenListener() {
    @Override
    public void onRemoveTokenSucceeded() {
        android.util.Log.d(TAG, "Token has been removed");
    }

    @Override
    public void onRemoveTokenFailed(@NonNull Throwable t) {
        // handle HTTP error
    }
});
```

### Removing token locally

To remove token locally, you can simply use following code:

```java
final PowerAuthTokenStore tokenStore = powerAuthSDK.getTokenStore();
// Remove just one token
tokenStore.removeLocalToken(context, "MyToken");
// Remove all local tokens
tokenStore.removeAllLocalTokens(context);
```

Note that removing tokens locally you'll loose control about tokens stored on the server.



## Common SDK Tasks

### Error Handling

The PowerAuth SDK is using following types of exceptions:

- `PowerAuthMissingConfigException` - is typically thrown immediately when `PowerAuthSDK` instance is initialized with an invalid configuration.
- `FailedApiException` - is typically returned to callbacks when an asynchronous HTTP request ends on error.
- `ErrorResponseApiException` - is typically returned to callbacks when an asynchronous HTTP request ends on error and the error model object is present in the response.
- `PowerAuthErrorException` - typically covers all other erroneous situations. You can investigate a detailed reason of failure by getting the integer, from set of `PowerAuthErrorCodes` constants.

Here's an example for a typical error handling procedure:

```java
Throwable t; // reported in asynchronous callback
if (t instanceof PowerAuthErrorException) {
    switch (((PowerAuthErrorException) t).getPowerAuthErrorCode()) {
        case PowerAuthErrorCodes.PA2ErrorCodeNetworkError:
            android.util.Log.d(TAG, "Error code for error with network connectivity or download"); break;
        case PowerAuthErrorCodes.PA2ErrorCodeSignatureError:
            android.util.Log.d(TAG,"Error code for error in signature calculation"); break;
        case PowerAuthErrorCodes.PA2ErrorCodeInvalidActivationState:
            android.util.Log.d(TAG,"Error code for error that occurs when activation state is invalid"); break;
        case PowerAuthErrorCodes.PA2ErrorCodeInvalidActivationData:
            android.util.Log.d(TAG,"Error code for error that occurs when activation data is invalid"); break;
        case PowerAuthErrorCodes.PA2ErrorCodeMissingActivation:
            android.util.Log.d(TAG,"Error code for error that occurs when activation is required but missing"); break;
        case PowerAuthErrorCodes.PA2ErrorCodeActivationPending:
            android.util.Log.d(TAG,"Error code for error that occurs when pending activation is present and work with completed activation is required"); break;
        case PowerAuthErrorCodes.PA2ErrorCodeBiometryCancel:
            android.util.Log.d(TAG,"Error code for Biometry action cancel error"); break;
        case PowerAuthErrorCodes.PA2ErrorCodeOperationCancelled:
            android.util.Log.d(TAG,"Error code for cancelled operations"); break;
        case PowerAuthErrorCodes.PA2ErrorCodeEncryptionError:
            android.util.Log.d(TAG,"Error code for errors related to end-to-end encryption"); break;
        case PowerAuthErrorCodes.PA2ErrorCodeInvalidToken:
            android.util.Log.d(TAG,"Error code for errors related to token based auth."); break;
        case PowerAuthErrorCodes.PA2ErrorCodeProtocolUpgrade:
            android.util.Log.d(TAG,"Error code for error that occurs when protocol upgrade fails at unrecoverable error."); break;
        case PowerAuthErrorCodes.PA2ErrorCodePendingProtocolUpgrade:
            android.util.Log.d(TAG,"The operation is temporarily unavailable, due to pending protocol upgrade."); break;
    }
} else if (t instanceof ErrorResponseApiException) {
    ErrorResponseApiException exception = (ErrorResponseApiException) t;
    Error errorResponse = exception.getErrorResponse();
    int httpResponseStatusCode = exception.getResponseCode();
    // Additional, optional objects assigned to the exception.
    JsonObject jsonResponseObject = exception.getResponseJson();
    String responseBodyString = exception.getResponseBody();
} else if (t instanceof FailedApiException) {
    FailedApiException exception = (FailedApiException) t;
    int httpStatusCode = exception.getResponseCode();
    // Additional, optional objects assigned to the exception.
    JsonObject jsonResponseObject = exception.getResponseJson();
    String responseBodyString = exception.getResponseBody();
}

```

Note that you typically don't need to handle all error codes reported in `PowerAuthErrorException`, or report all that situations to the user. Most of the codes are informational and helps the developers properly integrate SDK to the application. The good example is `PA2ErrorCodeInvalidActivationState`, which typically means that your application's logic is broken and you're using PowerAuthSDK in an unexpected way. 

Here's the list of an important error codes, which should be properly handled by the application:

- `PA2ErrorCodeBiometryCancel` is reported when user cancels biometric authentication dialog
- `PA2ErrorCodeProtocolUpgrade` is reported when SDK failed to upgrade itself to a newer protocol version. The code may be reported from `PowerAuthSDK.fetchActivationStatusWithCallback()`. This is an unrecoverable error resulting to the broken activation on the device, so the best situation is to inform user about the situation and remove the activation locally.
- `PA2ErrorCodePendingProtocolUpgrade` is reported when the requested SDK operation cannot be completed due to pending PowerAuth protocol upgrade. You can retry the operation later. The code is typically reported in the situations, when SDK is performing protocol upgrade on the background (as a part of activation status fetch) and the application want's to calculate PowerAuth signature in parallel operation. Such kind of concurrency is forbidden since SDK version `1.0.0`


### Working with Invalid SSL Certificates

Sometimes, you may need to develop or test you application against a service that runs over HTTPS protocol with invalid (self-signed) SSL certificate. By default, HTTP client used in PowerAuth SDK communication validates certificate. To disable the certificate validation, use a `PowerAuthSDK` initializer with custom client configuration to initialize your PowerAuth SDK instance, like so:

```java
// Set `PA2ClientSslNoValidationStrategy as the defauld client SSL certificate validation strategy`
final PowerAuthClientConfiguration clientConfiguration = new PowerAuthClientConfiguration.Builder()
                .clientValidationStrategy(new PA2ClientSslNoValidationStrategy())
                .build();

// Prepare the configuration, see above...
// ...

// Create a PowerAuth SDK instance
PowerAuthSDK powerAuthSDK = new PowerAuthSDK();
powerAuthSDK.initializeWithConfiguration(context, configuration, clientConfiguration);
```

### Debugging

The debug log is by default turned-off. To turn it on, use following code:
```java
PA2Log.setEnabled(true);
```

To turn-on even more detailed log, use following code:
```java
PA2Log.setVerbose(true);
```

Note that it's highly recommended to turn-on this feature only for `DEBUG` build of your application. For example:
```java
if (BuildConfig.DEBUG) {
    PA2Log.setEnabled(true);
}
```

## Additional Features

PowerAuth SDK for Android contains multiple additional features that are useful for mobile apps.

### Password Strength Indicator

Whenever user selects a password, you may estimate the password strength using following code:

```java
// Determine the password strength: INVALID, WEAK, NORMAL, STRONG
PasswordStrength strength = PasswordUtil.evaluateStrength("1234", PasswordType.PIN);
```

Passwords that are `INVALID` should block the progress. You should translate `WEAK` passwords to UI warnings (not blocking progress), `NORMAL` passwords to OK state, and `STRONG` passwords to rewarding UI message (so that we appreciate user selected strong password, without pushing the user to selecting strong password too hard).

Of course, you may apply any additional measures for the password validity and strength on top of the logics we provide.

> **WARNING:**
> The `PasswordUtil` class is deprecated and will be removed in some future version of PowerAuth Mobile SDK. You can migrate your code to [Wultra Passphrase Meter](https://github.com/wultra/passphrase-meter) library, which provides a better PIN or password strength evaluation.

### Debug build detection

It is sometimes useful to switch PowerAuth SDK to a DEBUG build configuration, to get more logs from the library. The DEBUG build is usually helpful during the application development, but on other side, it's highly unwanted in production applications. For this purpose, the `PowerAuthSDK.hasDebugFeatures()` method provides an information, whether the PowerAuth JNI library was compiled in DEBUG configuration. It is a good practice to check this flag and crash the process when the production application is linked against the DEBUG PowerAuth:

```java
if (!BuildConfig.DEBUG) {
    // You can also check your production build configuration
    if (powerAuthSDK.hasDebugFeatures()) {
        throw new RuntimeException("Production app with DEBUG PowerAuth");
    }
}
```

### Request Interceptors

The `PowerAuthClientConfiguration` can contain a multiple request interceptor objects, allowing you to adjust all HTTP requests created by SDK, before execution. Currently, you can use following two classes:

- `BasicHttpAuthenticationRequestInterceptor` to add basic HTTP authentication header to all requests
- `CustomHeaderRequestInterceptor` to add a custom HTTP header to all requests

For example: 

```java
final PowerAuthClientConfiguration clientConfiguration = new PowerAuthClientConfiguration.Builder()
            .requestInterceptor(new BasicHttpAuthenticationRequestInterceptor("gateway-user", "gateway-password"))
            .requestInterceptor(new CustomHeaderRequestInterceptor("X-CustomHeader", "123456"))
            .build();
```

We don't recommend you to implement `HttpRequestInterceptor` interface on your own. The interface allows you to tweak the requests created in the `PowerAuthSDK`, but also gives you an opportunity to break the things. So, rather than create your own interceptor, try to contact us and describe what's your problem with the networking in the PowerAuth SDK. Also keep in mind, that the interface may change in the future. We can guarantee the API stability of public classes implementing this interface, but not the stability of interface itself.
