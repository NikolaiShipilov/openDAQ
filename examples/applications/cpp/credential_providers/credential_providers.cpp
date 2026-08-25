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

// PrivateKeyFile authentication - another String-format credential payload, but instead of comparing a
// fixed secret, the module verifies a signed challenge against the public key configured via the
// "PublicKeyPath" module option (set above to keys/public_key.pem). When prompted, supply the path
// to the matching private key.
void demoPrivateKeyFileAuthentication(const InstancePtr& instance, const DeviceTypePtr& deviceType)
{
    std::cout << "When prompted for the private-key path, enter: " << CREDENTIAL_DEMO_KEYS_DIR << "/private_key.pem" << std::endl;
    auto privateKeyFileConfig = deviceType.getSupportedAuthenticationConfigs().get("PrivateKeyFile");
    auto device = instance.addAuthenticatedDevice("daq://openDAQ_1234", nullptr, privateKeyFileConfig);
    std::cout << "Connected to \"" << device.getInfo().getName() << "\" with private-key challenge authentication. Press \"enter\" to continue..." << std::endl;
    std::cin.get();
    instance.removeDevice(device);
}

// PrivateKeyBlob authentication - the same private-key challenge, but via a BinaryBlob-format
// credential payload instead of a FilePath one: fileCredentialProvider still prompts for the file's
// path, but now reads the file itself and hands the module the raw key bytes directly, so the module
// never touches the file (or even learns its path).
void demoPrivateKeyBlobAuthentication(const InstancePtr& instance, const DeviceTypePtr& deviceType)
{
    std::cout << "When prompted for the private-key path, enter: " << CREDENTIAL_DEMO_KEYS_DIR << "/private_key.pem" << std::endl;
    auto privateKeyBlobConfig = deviceType.getSupportedAuthenticationConfigs().get("PrivateKeyBlob");
    auto device = instance.addAuthenticatedDevice("daq://openDAQ_1234", nullptr, privateKeyBlobConfig);
    std::cout << "Connected to \"" << device.getInfo().getName() << "\" with private-key challenge authentication via a binary blob. Press \"enter\" to continue..." << std::endl;
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
void demoUserNamePasswordAuthenticationNonVerbose(const InstancePtr& instance, const DeviceTypePtr& deviceType)
{
    auto userNamePasswordConfig = deviceType.createDefaultAuthenticationConfig();
    auto device = instance.addAuthenticatedDevice("daq://openDAQ_1234", nullptr, userNamePasswordConfig);
    std::cout << "Connected to \"" << device.getInfo().getName() << "\" with UserName/Password authentication, non-verbose credential request. Press \"enter\" to continue..." << std::endl;
    std::cin.get();
    instance.removeDevice(device);
}

// authenticate with username and password but not hide the password
void demoUserNamePasswordAuthenticationVerbose(const InstancePtr& instance, const DeviceTypePtr& deviceType)
{
    auto userNamePasswordConfig = deviceType.createDefaultAuthenticationConfig();
    userNamePasswordConfig.getConfig().setPropertyValue("VerboseCredentialRequest", True);
    userNamePasswordConfig.getConfig().setPropertyValue("HidePasswordInput", True);
    auto device = instance.addAuthenticatedDevice("daq://openDAQ_1234", nullptr, userNamePasswordConfig);
    std::cout << "Connected to \"" << device.getInfo().getName() << "\" with UserName/Password authentication, verbose credential request. Press \"enter\" to continue..." << std::endl;
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
    auto privateKeyFileConfig = deviceType.getSupportedAuthenticationConfigs().get("PrivateKeyFile");
    auto explicitProviderConfig = AuthenticationConfigBuilder()
                                       .setPayloadId(privateKeyFileConfig.getCredentialPayloadId())
                                       .setPayloadDescriptor(privateKeyFileConfig.getCredentialPayloadDescriptor())
                                       .setConfig(privateKeyFileConfig.getConfig())
                                       .setCredentialProviderId(credentialProviderId)
                                       .build();
    std::cout << "When prompted for the private-key path, enter: " << CREDENTIAL_DEMO_KEYS_DIR << "/private_key.pem" << std::endl;
    auto device = instance.addAuthenticatedDevice("daq://openDAQ_1234", nullptr, explicitProviderConfig);
    std::cout << "Connected to \"" << device.getInfo().getName() << "\" with private-key challenge authentication via the explicitly selected \""
              << credentialProviderId << "\" credential provider. Press \"enter\" to continue..." << std::endl;
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
    auto streamingPrivateKeyFileConfig = streamingType.getSupportedAuthenticationConfigs().get("PrivateKeyFile");
    auto streamingAuthConfig = AuthenticationConfigBuilder()
                                   .setPayloadId(streamingPrivateKeyFileConfig.getCredentialPayloadId())
                                   .setPayloadDescriptor(streamingPrivateKeyFileConfig.getCredentialPayloadDescriptor())
                                   .setConfig(streamingPrivateKeyFileConfig.getConfig())
                                   .setCredentialProviderId(credentialProviderId)
                                   .build();

    auto devicePrivateKeyFileConfig = deviceType.getSupportedAuthenticationConfigs().get("PrivateKeyFile");
    auto deviceAuthConfig = AuthenticationConfigBuilder()
                                 .setPayloadId(devicePrivateKeyFileConfig.getCredentialPayloadId())
                                 .setPayloadDescriptor(devicePrivateKeyFileConfig.getCredentialPayloadDescriptor())
                                 .setConfig(devicePrivateKeyFileConfig.getConfig())
                                 .setCredentialProviderId(credentialProviderId)
                                 .setSuppliedSecret(String(std::string(CREDENTIAL_DEMO_KEYS_DIR) + "/private_key.pem"))
                                 .addStreamingAuthenticationConfig(streamingType, streamingAuthConfig)
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

// A single authentication config can carry settings for more than one connection at once: the device's
// own (here its default, UserName/Password), plus one nested per streaming type for anything else
// that's part of the same overall connection - here the demo streaming type, authenticated via a
// different method (PrivateKeyBlob) than the device. The nested config is keyed internally by the
// streaming type's own id - passing the type itself (rather than a bare id string) means only a real,
// registered streaming type can ever be used as the key. The nested config is an ordinary
// `AuthenticationConfig` in its own right - once pulled back out of the dictionary returned by
// `getStreamingAuthenticationConfigs`, it's usable anywhere a standalone one would be, e.g. handed
// directly to a manual `addStreaming` call below.
void demoDeviceAndStreamingAuthentication(const InstancePtr& instance, const DeviceTypePtr& deviceType)
{

    auto deviceDefaultAuthConfig = deviceType.createDefaultAuthenticationConfig();
    auto deviceAuthConfig = AuthenticationConfigBuilder()
                                 .setPayloadId(deviceDefaultAuthConfig.getCredentialPayloadId())
                                 .setPayloadDescriptor(deviceDefaultAuthConfig.getCredentialPayloadDescriptor())
                                 .setConfig(deviceDefaultAuthConfig.getConfig())
                                 .build();

    std::cout << "Device authentication (default method - UserName/Password):" << std::endl;
    auto device = instance.addAuthenticatedDevice("daq://openDAQ_1234", nullptr, deviceAuthConfig);
    std::cout << "Connected to \"" << device.getInfo().getName() << "\" with UserName/Password authentication." << std::endl;

    auto streamingType = instance.getModuleManager().asPtr<IModuleManagerUtils>().getAvailableStreamingTypes().get("CredentialDemoStreaming");
    auto streamingAuthConfig = streamingType.getSupportedAuthenticationConfigs().get("PrivateKeyBlob");
    std::cout << "When prompted for the private-key path, enter: " << CREDENTIAL_DEMO_KEYS_DIR << "/private_key.pem" << std::endl;
    device.addStreaming("daq.credential_demo_streaming://credential_demo_device", nullptr, streamingAuthConfig);
    std::cout << "Attached a streaming connection authenticated via the config nested inside the device's own. "
                 "Streaming sources: "
              << device.asPtr<IMirroredDevice>().getStreamingSources().getCount() << std::endl;
    std::cout << "Press \"enter\" to continue..." << std::endl;
    std::cin.get();
    instance.removeDevice(device);
}

// PIN authentication - an alternative, String-format credential payload. The device is authenticated via
// PIN, then a streaming connection is attached manually with its own, separate PrivateKeyBlob credential
// request - attaching a streaming connection is authenticated independently of however the device itself
// got connected. Finally, the instance is saved and reloaded into a completely separate instance to show
// that a previously authenticated device is re-authenticated (not silently reconnected) on load.
void demoPinAuthenticationAndReload(const InstancePtr& instance, const DeviceTypePtr& deviceType)
{
    auto pinConfig = deviceType.getSupportedAuthenticationConfigs().get("Pin");
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
    // demoPrivateKeyBlobAuthentication(instance, deviceType);
    // demoNoAuthentication(instance);
    // demoUserNamePasswordAuthenticationNonVerbose(instance, deviceType);
    demoUserNamePasswordAuthenticationVerbose(instance, deviceType);
    demoDeviceAndStreamingAuthentication(instance, deviceType);
    // demoExplicitCredentialProviderSelection(instance, deviceType, credentialProvider.getName());
    demoCachedFilePathCredentialAcrossDeviceAndStreaming(instance, deviceType, credentialProvider.getName());
    demoPinAuthenticationAndReload(instance, deviceType);

    std::cout << "Press \"enter\" to exit the application..." << std::endl;
    std::cin.get();
    return 0;
}
