#include <iostream>
#include <fstream>
#include <opendaq/opendaq.h>
#include <opendaq/module_manager_utils_ptr.h>

using namespace daq;

static const std::string JSON_CONFIG_FILE_NAME = "credential-demo-opendaq-config.json";

void createJsonConfigFile()
{
    std::string filename = JSON_CONFIG_FILE_NAME;
    std::string options = R"(
    {
    "Modules": {
        "CredentialDemoModule": {
            "Manufacturer": "openDAQ",
            "SerialNumber": "1234",
            "PublicKeyPath": ")" + std::string(CREDENTIAL_DEMO_KEYS_DIR) + R"(/public_key.pem"
            }
        }
    }
    )";

    std::ofstream file;
    file.open(filename);
    if (!file.is_open())
        throw std::runtime_error("can not open file for writing");

    file << options;
    file.close();
}

// `createDefaultAuthenticationConfig()` returns one self-contained config listing every authentication
// method the component type supports (UserNamePassword, Pin, PrivateKeyFile) as a candidate of its
// "PayloadDescriptor" selection property, defaulting to the type's own default method - never a separate
// config per method. This switches that selection to the method named by `payloadId`, entirely through
// plain property object calls: the candidates are read generically off the property itself, and the match
// is found by comparing each candidate Struct's own "Id" field - no `ICredentialPayloadDescriptor` cast
// needed for the comparison itself, only to read `getId()` off it.
void SelectAuthenticationMethod(const AuthenticationConfigPtr& authConfig, const StringPtr& payloadId)
{
    ListPtr<IStruct> candidates = authConfig.getProperty("PayloadDescriptor").getSelectionValues();
    for (const auto& candidate : candidates)
    {
        if (candidate.asPtr<ICredentialPayloadDescriptor>().getId() == payloadId)
        {
            authConfig.setPropertySelectionValue("PayloadDescriptor", candidate);
            return;
        }
    }
    throw std::runtime_error("Unknown authentication method payload id: " + payloadId.toStdString());
}

// PrivateKeyFile authentication - another String-format credential payload, but instead of comparing a
// fixed secret, the module verifies a signed challenge against the public key configured via the
// "PublicKeyPath" module option (set above to keys/public_key.pem). When prompted, supply the path
// to the matching private key.
void demoPrivateKeyFileAuthentication(const InstancePtr& instance, const DeviceTypePtr& deviceType)
{
    std::cout << "When prompted for the private-key path, enter: " << CREDENTIAL_DEMO_KEYS_DIR << "/private_key.pem" << std::endl;
    auto privateKeyFileConfig = deviceType.createDefaultAuthenticationConfig();
    SelectAuthenticationMethod(privateKeyFileConfig, "PrivateKeyFile");
    auto device = instance.addAuthenticatedDevice("daq://openDAQ_1234", nullptr, privateKeyFileConfig);
    std::cout << "Connected to \"" << device.getInfo().getName() << "\" with private-key challenge authentication. Press \"enter\" to continue..." << std::endl;
    std::cin.get();
    instance.removeDevice(device);
}

// add without authentication
void demoNoAuthentication(const InstancePtr& instance)
{
    auto device = instance.addDevice("daq://openDAQ_1234");
    std::cout << "Connected to \"" << device.getInfo().getName() << "\" without authentication. Press \"enter\" to continue..." << std::endl;
    std::cin.get();
    instance.removeDevice(device);
}

// authenticate with username and password
// UserName/Password authentication - a KeyValuePairs-format credential payload.
void demoUserNamePasswordAuthentication(const InstancePtr& instance, const DeviceTypePtr& deviceType)
{
    auto userNamePasswordConfig = deviceType.createDefaultAuthenticationConfig();
    auto device = instance.addAuthenticatedDevice("daq://openDAQ_1234", nullptr, userNamePasswordConfig);
    std::cout << "Connected to \"" << device.getInfo().getName() << "\" with UserName/Password authentication. Press \"enter\" to continue..." << std::endl;
    std::cin.get();
    instance.removeDevice(device);
}

// Normally, the module auto-selects a registered credential provider supporting the payload's format - but
// an authentication config can instead name a specific provider explicitly via `setCredentialProviderId`,
// bypassing auto-selection. This only makes an observable difference for a payload format more than one
// registered provider supports - FilePath is one (both fileCredentialProvider and credentialProvider
// support it), and fileCredentialProvider - registered first - is the one auto-selection would otherwise
// pick (see the comment where it's registered, below). Left unset (as in the other demos above),
// auto-selection is used, same as before this was added.
void demoExplicitCredentialProviderSelection(const InstancePtr& instance, const DeviceTypePtr& deviceType, const StringPtr& credentialProviderId)
{
    auto privateKeyFileConfig = deviceType.createDefaultAuthenticationConfig();
    SelectAuthenticationMethod(privateKeyFileConfig, "PrivateKeyFile");
    auto explicitProviderConfig = AuthenticationConfigBuilder()
                                       .setPayloadDescriptor(privateKeyFileConfig.getCredentialPayloadDescriptor())
                                       .setCredentialProviderId(credentialProviderId)
                                       .build();
    std::cout << "When prompted for the private-key path, enter: " << CREDENTIAL_DEMO_KEYS_DIR << "/private_key.pem" << std::endl;
    auto device = instance.addAuthenticatedDevice("daq://openDAQ_1234", nullptr, explicitProviderConfig);
    std::cout << "Connected to \"" << device.getInfo().getName() << "\" with private-key challenge authentication via the explicitly selected \""
              << credentialProviderId << "\" credential provider. Press \"enter\" to continue..." << std::endl;
    std::cin.get();
    instance.removeDevice(device);
}

// `IAuthenticationConfig` derives from `IPropertyObject` - the same way `IDeviceInfo` does - so besides the
// typed getters/builder setters used in every other demo, the exact same settings can be read and set with
// plain property object calls instead. `deviceType.createDefaultAuthenticationConfig()` already returns one
// self-contained config with every supported method (UserNamePassword, Pin, PrivateKeyFile) as a candidate
// of its "PayloadDescriptor" selection property, defaulting to UserNamePassword - `SelectAuthenticationMethod`
// switches that selection to "Pin" via plain property object calls, no separate per-method config fetched
// anywhere. The credential provider id is then likewise set as an ordinary "CredentialProviderId" String
// property. No `IAuthenticationConfigBuilder` involved anywhere in this demo.
void demoAuthenticationConfigAsPropertyObject(const InstancePtr& instance, const DeviceTypePtr& deviceType, const StringPtr& credentialProviderId)
{
    auto authConfig = deviceType.createDefaultAuthenticationConfig();
    SelectAuthenticationMethod(authConfig, "Pin");

    StructPtr payloadDescriptor = authConfig.getPropertySelectionValue("PayloadDescriptor");
    std::cout << "Payload id, read as a plain property object selection value: " << payloadDescriptor.get("Id") << std::endl;

    authConfig.setPropertyValue("CredentialProviderId", credentialProviderId);
    std::cout << "Credential provider id, read back as a plain String property: " << authConfig.getPropertyValue("CredentialProviderId") << std::endl;

    std::cout << "When prompted for the PIN, enter: 1234" << std::endl;
    auto device = instance.addAuthenticatedDevice("daq://openDAQ_1234", nullptr, authConfig);
    std::cout << "Connected to \"" << device.getInfo().getName()
              << "\" with PIN authentication, configured entirely via plain property object calls on the authentication config itself. "
                 "Press \"enter\" to continue..."
              << std::endl;
    std::cin.get();
    instance.removeDevice(device);
}

// CmdLineCredentialProvider caches FilePath secrets in-memory for the active session, keyed by
// (manufacturer, serialNumber) - so authenticating a second connection to the very same device via the
// same FilePath-format method reuses the path already entered instead of prompting again. Demonstrated
// here across two different connections to the same device - first the device itself, then a streaming
// connection attached to it - both explicitly using the caching provider (FilePath is otherwise
// auto-selected to fileCredentialProvider, which does not cache). The device's own path is supplied
// directly via `setSuppliedSecret` rather than typed interactively - the specified provider still caches
// it (see `ICredentialProvider::cacheCredentials`), so no user prompt is needed anywhere in this demo.
void demoCachedFilePathCredentialAcrossDeviceAndStreaming(const InstancePtr& instance, const DeviceTypePtr& deviceType, const StringPtr& credentialProviderId)
{
    auto streamingType = instance.getModuleManager().asPtr<IModuleManagerUtils>().getAvailableStreamingTypes().get("CredentialDemoStreaming");
    auto streamingPrivateKeyFileConfig = streamingType.createDefaultAuthenticationConfig();
    SelectAuthenticationMethod(streamingPrivateKeyFileConfig, "PrivateKeyFile");
    auto streamingAuthConfig = AuthenticationConfigBuilder()
                                   .setPayloadDescriptor(streamingPrivateKeyFileConfig.getCredentialPayloadDescriptor())
                                   .setCredentialProviderId(credentialProviderId)
                                   .build();

    auto devicePrivateKeyFileConfig = deviceType.createDefaultAuthenticationConfig();
    SelectAuthenticationMethod(devicePrivateKeyFileConfig, "PrivateKeyFile");

    // The supplied secret must be shaped like the descriptor's own `createDefaultPayload` template - here
    // just a single "Secret" property, filled in with the private key's path.
    auto suppliedSecret = devicePrivateKeyFileConfig.getCredentialPayloadDescriptor().createDefaultPayload();
    suppliedSecret.setPropertyValue("Secret", String(std::string(CREDENTIAL_DEMO_KEYS_DIR) + "/private_key.pem"));

    auto deviceAuthConfig = AuthenticationConfigBuilder()
                                 .setPayloadDescriptor(devicePrivateKeyFileConfig.getCredentialPayloadDescriptor())
                                 .setCredentialProviderId(credentialProviderId)
                                 .setSuppliedSecret(suppliedSecret)
                                 .build();

    auto device = instance.addAuthenticatedDevice("daq://openDAQ_1234", nullptr, deviceAuthConfig);
    std::cout << "Connected to \"" << device.getInfo().getName() << "\" with private-key challenge authentication via the \""
              << credentialProviderId << "\" credential provider, using a secret supplied directly - no prompt." << std::endl;

    std::cout << "Attaching a streaming connection authenticated the same way - same device, same FilePath-format "
                 "method, same credential provider - so no path prompt should appear this time; the provider serves "
                 "it from its cache instead."
              << std::endl;
    device.addStreaming("daq.credential_demo_streaming://credential_demo_device", nullptr, streamingAuthConfig);
    std::cout << "Attached. Streaming sources: " << device.asPtr<IMirroredDevice>().getStreamingSources().getCount() << std::endl;
    std::cout << "Press \"enter\" to continue..." << std::endl;
    std::cin.get();
    instance.removeDevice(device);
}

// The device and a streaming connection attached to it are authenticated entirely independently of one
// another - the device here via its default method (UserName/Password), the streaming connection via its
// own, separate authentication config (PrivateKeyFile), attached with a manual `addStreaming` call.
void demoDeviceAndStreamingAuthentication(const InstancePtr& instance, const DeviceTypePtr& deviceType)
{
    auto deviceAuthConfig = deviceType.createDefaultAuthenticationConfig();

    std::cout << "Device authentication (default method - UserName/Password):" << std::endl;
    auto device = instance.addAuthenticatedDevice("daq://openDAQ_1234", nullptr, deviceAuthConfig);
    std::cout << "Connected to \"" << device.getInfo().getName() << "\" with UserName/Password authentication." << std::endl;

    auto streamingType = instance.getModuleManager().asPtr<IModuleManagerUtils>().getAvailableStreamingTypes().get("CredentialDemoStreaming");
    auto streamingAuthConfig = streamingType.createDefaultAuthenticationConfig();
    SelectAuthenticationMethod(streamingAuthConfig, "PrivateKeyFile");
    std::cout << "When prompted for the private-key path, enter: " << CREDENTIAL_DEMO_KEYS_DIR << "/private_key.pem" << std::endl;
    device.addStreaming("daq.credential_demo_streaming://credential_demo_device", nullptr, streamingAuthConfig);
    std::cout << "Attached a streaming connection authenticated independently of the device. Streaming sources: "
              << device.asPtr<IMirroredDevice>().getStreamingSources().getCount() << std::endl;
    std::cout << "Press \"enter\" to continue..." << std::endl;
    std::cin.get();
    instance.removeDevice(device);
}

// PIN authentication - an alternative, String-format credential payload. The device is authenticated via
// PIN. Finally, the instance is saved and reloaded into a completely separate instance to show that a
// previously authenticated device is re-authenticated (not silently reconnected) on load.
void demoPinAuthenticationAndReload(const InstancePtr& instance, const DeviceTypePtr& deviceType)
{
    auto pinConfig = deviceType.createDefaultAuthenticationConfig();
    SelectAuthenticationMethod(pinConfig, "Pin");
    auto device = instance.addAuthenticatedDevice("daq://openDAQ_1234", nullptr, pinConfig);
    std::cout << "Connected to \"" << device.getInfo().getName() << "\" with PIN authentication." << std::endl;

    std::cout << "Press \"enter\" to save the configuration and reload it into a new instance..." << std::endl;
    std::cin.get();

    // Saving the instance carries the connected device's credential request along with it - its payload id,
    // descriptor and non-secret metadata - but never the authentication config or the credentials themselves.
    auto savedConfiguration = instance.saveConfiguration();

    // A completely separate instance, loading the saved configuration - it needs its own credential provider
    // registered, since the reloaded device is re-authenticated (the provider is asked for real credentials
    // again) rather than silently reconnected without any.
    auto reloadedCredentialProvider = CmdLineCredentialProvider();
    auto reloadedFileCredentialProvider = FileCredentialProvider();

    auto reloadedInstanceBuilder = InstanceBuilder();
    reloadedInstanceBuilder.addModulePath(MODULE_PATH);
    reloadedInstanceBuilder.addConfigProvider(JsonConfigProvider(JSON_CONFIG_FILE_NAME));
    reloadedInstanceBuilder.addCredentialProvider(reloadedFileCredentialProvider.getName(), reloadedFileCredentialProvider);
    reloadedInstanceBuilder.addCredentialProvider(reloadedCredentialProvider.getName(), reloadedCredentialProvider);
    auto reloadedInstance = reloadedInstanceBuilder.build();

    reloadedInstance.loadConfiguration(savedConfiguration);

    auto reloadedDevices = reloadedInstance.getDevices();
    if (reloadedDevices.getCount() == 0)
        throw std::runtime_error("Reloaded instance has no devices - the device failed to reconnect on load");

    auto reloadedDevice = reloadedDevices[0];
    std::cout << "Reloaded instance re-authenticated and reconnected to \"" << reloadedDevice.getInfo().getName() << std::endl;
}

int main(int argc, const char* argv[])
{
    createJsonConfigFile();

    auto credentialProvider = CmdLineCredentialProvider();
    auto fileCredentialProvider = FileCredentialProvider();

    auto instanceBuilder = InstanceBuilder();
    instanceBuilder.addModulePath(MODULE_PATH);
    instanceBuilder.addConfigProvider(JsonConfigProvider(JSON_CONFIG_FILE_NAME));
    // Registered first, so it - not CmdLineCredentialProvider - is the one FindMatchingCredentialProvider
    // picks for FilePath-format requests (e.g. the PrivateKeyFile auth method below).
    instanceBuilder.addCredentialProvider(fileCredentialProvider.getName(), fileCredentialProvider);
    instanceBuilder.addCredentialProvider(credentialProvider.getName(), credentialProvider);
    auto instance = instanceBuilder.build();

    // get the type to obtain default authentication settings
    auto deviceType = instance.getAvailableDeviceTypes().get("CredentialDemoDevice");

    // demoPrivateKeyFileAuthentication(instance, deviceType);
    // demoNoAuthentication(instance);
    demoUserNamePasswordAuthentication(instance, deviceType);
    demoDeviceAndStreamingAuthentication(instance, deviceType);
    // demoExplicitCredentialProviderSelection(instance, deviceType, credentialProvider.getName());
    demoAuthenticationConfigAsPropertyObject(instance, deviceType, credentialProvider.getName());
    demoCachedFilePathCredentialAcrossDeviceAndStreaming(instance, deviceType, credentialProvider.getName());
    demoPinAuthenticationAndReload(instance, deviceType);

    std::cout << "Press \"enter\" to exit the application..." << std::endl;
    std::cin.get();
    return 0;
}
