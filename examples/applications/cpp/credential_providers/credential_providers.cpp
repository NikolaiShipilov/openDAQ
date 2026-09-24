#include <iostream>
#include <fstream>
#include <coreobjects/callable_info_factory.h>
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

// `AuthenticationConfig(componentType, context)` returns one self-contained config listing every
// authentication method the named component type supports (UserNamePassword, Pin, PrivateKeyFile) as a
// candidate of its "AuthenticationMethod" selection property, defaulting to the type's own default method -
// never a separate config per method. This switches that selection to the method named by `authenticationMethodId`,
// entirely through plain property object calls: the candidates are read generically off the property
// itself, and the match is found by comparing each candidate Struct's own "AuthenticationMethodId" field - no
// `ICredentialDescriptor` cast needed for the comparison itself, only to read `getAuthenticationMethodId()` off it.
void SelectAuthenticationMethod(const AuthenticationConfigPtr& authConfig, const StringPtr& authenticationMethodId)
{
    ListPtr<IStruct> candidates = authConfig.getProperty("AuthenticationMethod").getSelectionValues();
    for (const auto& candidate : candidates)
    {
        if (candidate.asPtr<ICredentialDescriptor>().getAuthenticationMethodId() == authenticationMethodId)
        {
            authConfig.setPropertySelectionValue("AuthenticationMethod", candidate);
            return;
        }
    }
    throw std::runtime_error("Unknown authentication method id: " + authenticationMethodId.toStdString());
}

// Temporary bridge until `IAuthenticationConfig` becomes a real part of the add-device config schema:
// `addAuthenticatedDevice`/`addStreaming(..., authenticationConfig)` are gone - only the plain, config-only
// overloads remain. Until the real config-schema integration lands, `authConfig` is instead smuggled onto an
// otherwise plain `config` property object under this property name, which `Module`/`ModuleManagerImpl` look
// for and honor exactly as the old, now-removed parameter used to be - this key must match theirs exactly
// (see `AuthenticationConfigConfigKey` in `module_impl.h`). Stashed as a zero-argument Function property that
// returns it when called, since neither a plain Object-type property value (the property framework only
// accepts a literal `IPropertyObject`, not a more specific derived interface like `IAuthenticationConfig`) nor
// a List/Dict item (Container-type properties forbid object-type items/keys entirely) will hold it.
static const char* AuthenticationConfigConfigKey = "__AuthenticationConfig";

PropertyObjectPtr WithAuthenticationConfig(const AuthenticationConfigPtr& authConfig)
{
    auto config = PropertyObject();
    config.addProperty(FunctionProperty(AuthenticationConfigConfigKey, FunctionInfo(ctObject)));
    config.setPropertyValue(AuthenticationConfigConfigKey, Function([authConfig]() { return authConfig; }));
    return config;
}

// PrivateKeyFile authentication - another String-format credential, but instead of comparing a
// fixed secret, the module verifies a signed challenge against the public key configured via the
// "PublicKeyPath" module option (set above to keys/public_key.pem). When prompted, supply the path
// to the matching private key.
void demoPrivateKeyFileAuthentication(const InstancePtr& instance, const DeviceTypePtr& deviceType)
{
    std::cout << "When prompted for the private-key path, enter: " << CREDENTIAL_DEMO_KEYS_DIR << "/private_key.pem" << std::endl;
    auto privateKeyFileConfig = AuthenticationConfig(deviceType, instance.getContext());
    SelectAuthenticationMethod(privateKeyFileConfig, "PrivateKeyFile");
    auto device = instance.addDevice("daq://openDAQ_1234", WithAuthenticationConfig(privateKeyFileConfig));
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
// UserName/Password authentication - a KeyValuePairs-format credential. `AuthenticationConfig(deviceType, context)`'s
// "CredentialProviderId" selection defaults to the first registered provider (fileCredentialProvider), which only
// supports FilePath - explicitly switch it to credentialProvider, which supports every format.
void demoUserNamePasswordAuthentication(const InstancePtr& instance, const DeviceTypePtr& deviceType, const StringPtr& credentialProviderId)
{
    auto userNamePasswordConfig = AuthenticationConfig(deviceType, instance.getContext());
    userNamePasswordConfig.setPropertySelectionValue("CredentialProviderId", credentialProviderId);
    auto device = instance.addDevice("daq://openDAQ_1234", WithAuthenticationConfig(userNamePasswordConfig));
    std::cout << "Connected to \"" << device.getInfo().getName() << "\" with UserName/Password authentication. Press \"enter\" to continue..." << std::endl;
    std::cin.get();
    instance.removeDevice(device);
}

// Explicitly switches "CredentialProviderId" away from its live default (the config's own selection, not
// something the module chooses - it only ever consumes whichever id is currently selected here), naming a
// provider instead of leaving the default in place. This only makes an observable difference for a format
// more than one registered provider supports - FilePath is one (both fileCredentialProvider and
// credentialProvider support it), and fileCredentialProvider - registered first - is the one the live
// default (an unset id) would otherwise pick (see the comment where it's registered, below).
void demoExplicitCredentialProviderSelection(const InstancePtr& instance, const DeviceTypePtr& deviceType, const StringPtr& credentialProviderId)
{
    auto privateKeyFileConfig = AuthenticationConfig(deviceType, instance.getContext());
    SelectAuthenticationMethod(privateKeyFileConfig, "PrivateKeyFile");
    privateKeyFileConfig.setPropertySelectionValue("CredentialProviderId", credentialProviderId);
    std::cout << "When prompted for the private-key path, enter: " << CREDENTIAL_DEMO_KEYS_DIR << "/private_key.pem" << std::endl;
    auto device = instance.addDevice("daq://openDAQ_1234", WithAuthenticationConfig(privateKeyFileConfig));
    std::cout << "Connected to \"" << device.getInfo().getName() << "\" with private-key challenge authentication via the explicitly selected \""
              << credentialProviderId << "\" credential provider. Press \"enter\" to continue..." << std::endl;
    std::cin.get();
    instance.removeDevice(device);
}

// `IAuthenticationConfig` derives from `IPropertyObject` - the same way `IDeviceInfo` does - so besides the
// typed getters used in every other demo, the exact same settings can be read and set with plain property
// object calls instead. `AuthenticationConfig(deviceType, context)` already returns one self-contained config
// with every supported method (UserNamePassword, Pin, PrivateKeyFile) as a candidate of its
// "AuthenticationMethod" selection property, defaulting to UserNamePassword -
// `SelectAuthenticationMethod` switches that selection to "Pin" via plain property object calls, no separate
// per-method config fetched anywhere. The credential provider id is likewise a "CredentialProviderId"
// Selection property (over every provider registered on the instance) - set the same way.
void demoAuthenticationConfigAsPropertyObject(const InstancePtr& instance, const DeviceTypePtr& deviceType, const StringPtr& credentialProviderId)
{
    auto authConfig = AuthenticationConfig(deviceType, instance.getContext());
    SelectAuthenticationMethod(authConfig, "Pin");

    StructPtr credentialDescriptor = authConfig.getPropertySelectionValue("AuthenticationMethod");
    std::cout << "Authentication method id, read as a plain property object selection value: " << credentialDescriptor.get("AuthenticationMethodId") << std::endl;

    authConfig.setPropertySelectionValue("CredentialProviderId", credentialProviderId);
    std::cout << "Credential provider id, read back as a Selection property: " << authConfig.getPropertySelectionValue("CredentialProviderId") << std::endl;

    std::cout << "When prompted for the PIN, enter: 1234" << std::endl;
    auto device = instance.addDevice("daq://openDAQ_1234", WithAuthenticationConfig(authConfig));
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
// connection attached to it - both explicitly using the caching provider (FilePath's own live default is
// otherwise fileCredentialProvider, which does not cache). The device's own path is supplied
// directly via `setSuppliedSecret` rather than typed interactively - the specified provider still caches
// it (see `ICredentialProvider::cacheCredentials`), so no user prompt is needed anywhere in this demo.
void demoCachedFilePathCredentialAcrossDeviceAndStreaming(const InstancePtr& instance, const DeviceTypePtr& deviceType, const StringPtr& credentialProviderId)
{
    auto streamingType = instance.getModuleManager().asPtr<IModuleManagerUtils>().getAvailableStreamingTypes().get("CredentialDemoStreaming");
    auto streamingAuthConfig = AuthenticationConfig(streamingType, instance.getContext());
    SelectAuthenticationMethod(streamingAuthConfig, "PrivateKeyFile");
    streamingAuthConfig.setPropertySelectionValue("CredentialProviderId", credentialProviderId);

    auto deviceAuthConfig = AuthenticationConfig(deviceType, instance.getContext());
    SelectAuthenticationMethod(deviceAuthConfig, "PrivateKeyFile");
    deviceAuthConfig.setPropertySelectionValue("CredentialProviderId", credentialProviderId);

    // The supplied secret must be shaped like the descriptor's own `createEmptySecret` template - here
    // just a single "PrivateKeyFilePath" property, filled in with the private key's path.
    auto suppliedSecret = deviceAuthConfig.getSupportedAuthenticationMethods().get(deviceAuthConfig.getSelectedAuthenticationMethodId()).createEmptySecret();
    suppliedSecret.setPropertyValue("PrivateKeyFilePath", String(std::string(CREDENTIAL_DEMO_KEYS_DIR) + "/private_key.pem"));
    deviceAuthConfig.setPropertyValue("SuppliedSecret", suppliedSecret);

    auto device = instance.addDevice("daq://openDAQ_1234", WithAuthenticationConfig(deviceAuthConfig));
    std::cout << "Connected to \"" << device.getInfo().getName() << "\" with private-key challenge authentication via the \""
              << credentialProviderId << "\" credential provider, using a secret supplied directly - no prompt." << std::endl;

    std::cout << "Attaching a streaming connection authenticated the same way - same device, same FilePath-format "
                 "method, same credential provider - so no path prompt should appear this time; the provider serves "
                 "it from its cache instead."
              << std::endl;
    device.addStreaming("daq.credential_demo_streaming://credential_demo_device", WithAuthenticationConfig(streamingAuthConfig));
    std::cout << "Attached. Streaming sources: " << device.asPtr<IMirroredDevice>().getStreamingSources().getCount() << std::endl;
    std::cout << "Press \"enter\" to continue..." << std::endl;
    std::cin.get();
    instance.removeDevice(device);
}

// The device and a streaming connection attached to it are authenticated entirely independently of one
// another - the device here via its default method (UserName/Password), the streaming connection via its
// own, separate authentication config (PrivateKeyFile), attached with a manual `addStreaming` call.
void demoDeviceAndStreamingAuthentication(const InstancePtr& instance, const DeviceTypePtr& deviceType, const StringPtr& credentialProviderId)
{
    auto deviceAuthConfig = AuthenticationConfig(deviceType, instance.getContext());
    // UserName/Password (KeyValuePairs) isn't supported by the default "CredentialProviderId" selection
    // (fileCredentialProvider, FilePath-only) - switch to credentialProvider, which supports every format.
    deviceAuthConfig.setPropertySelectionValue("CredentialProviderId", credentialProviderId);

    std::cout << "Device authentication (default method - UserName/Password):" << std::endl;
    auto device = instance.addDevice("daq://openDAQ_1234", WithAuthenticationConfig(deviceAuthConfig));
    std::cout << "Connected to \"" << device.getInfo().getName() << "\" with UserName/Password authentication." << std::endl;

    auto streamingType = instance.getModuleManager().asPtr<IModuleManagerUtils>().getAvailableStreamingTypes().get("CredentialDemoStreaming");
    auto streamingAuthConfig = AuthenticationConfig(streamingType, instance.getContext());
    SelectAuthenticationMethod(streamingAuthConfig, "PrivateKeyFile");
    std::cout << "When prompted for the private-key path, enter: " << CREDENTIAL_DEMO_KEYS_DIR << "/private_key.pem" << std::endl;
    device.addStreaming("daq.credential_demo_streaming://credential_demo_device", WithAuthenticationConfig(streamingAuthConfig));
    std::cout << "Attached a streaming connection authenticated independently of the device. Streaming sources: "
              << device.asPtr<IMirroredDevice>().getStreamingSources().getCount() << std::endl;
    std::cout << "Press \"enter\" to continue..." << std::endl;
    std::cin.get();
    instance.removeDevice(device);
}

// PIN authentication - an alternative, String-format credential. The device is authenticated via
// PIN. Finally, the instance is saved and reloaded into a completely separate instance to show that a
// previously authenticated device is re-authenticated (not silently reconnected) on load.
void demoPinAuthenticationAndReload(const InstancePtr& instance, const DeviceTypePtr& deviceType, const StringPtr& credentialProviderId)
{
    auto pinConfig = AuthenticationConfig(deviceType, instance.getContext());
    SelectAuthenticationMethod(pinConfig, "Pin");
    // Pin (String) isn't supported by the default "CredentialProviderId" selection (fileCredentialProvider,
    // FilePath-only) - switch to credentialProvider, which supports every format.
    pinConfig.setPropertySelectionValue("CredentialProviderId", credentialProviderId);
    auto device = instance.addDevice("daq://openDAQ_1234", WithAuthenticationConfig(pinConfig));
    std::cout << "Connected to \"" << device.getInfo().getName() << "\" with PIN authentication." << std::endl;

    std::cout << "Press \"enter\" to save the configuration and reload it into a new instance..." << std::endl;
    std::cin.get();

    // Saving the instance carries the connected device's credential request along with it - its authentication method id,
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
    reloadedInstanceBuilder.addCredentialProvider(reloadedFileCredentialProvider.getId(), reloadedFileCredentialProvider);
    reloadedInstanceBuilder.addCredentialProvider(reloadedCredentialProvider.getId(), reloadedCredentialProvider);
    auto reloadedInstance = reloadedInstanceBuilder.build();

    reloadedInstance.loadConfiguration(savedConfiguration);

    auto reloadedDevices = reloadedInstance.getDevices();
    if (reloadedDevices.getCount() == 0)
        throw std::runtime_error("Reloaded instance has no devices - the device failed to reconnect on load");

    auto reloadedDevice = reloadedDevices[0];
    std::cout << "Reloaded instance re-authenticated and reconnected to \"" << reloadedDevice.getInfo().getName() << std::endl;
}

int main(int /*argc*/, const char* /*argv*/[])
{
    createJsonConfigFile();

    auto credentialProvider = CmdLineCredentialProvider();
    auto fileCredentialProvider = FileCredentialProvider();

    auto instanceBuilder = InstanceBuilder();
    instanceBuilder.addModulePath(MODULE_PATH);
    instanceBuilder.addConfigProvider(JsonConfigProvider(JSON_CONFIG_FILE_NAME));

    // Registered first, so it - not CmdLineCredentialProvider - becomes the live default "CredentialProviderId"
    // selection for FilePath-format requests (e.g. the PrivateKeyFile auth method below), unless overridden.
    instanceBuilder.addCredentialProvider(fileCredentialProvider.getId(), fileCredentialProvider);
    auto instance = instanceBuilder.build();

    // CmdLineCredentialProvider is registered here instead, directly on the already-built instance's Context
    // via `IContext::addCredentialProvider` - show-casing that credential providers aren't limited to instance-build time.
    instance.getContext().addCredentialProvider(credentialProvider.getId(), credentialProvider);

    // get the type to obtain default authentication settings
    auto deviceType = instance.getAvailableDeviceTypes().get("CredentialDemoDevice");

    // demoPrivateKeyFileAuthentication(instance, deviceType);
    // demoNoAuthentication(instance);
    demoUserNamePasswordAuthentication(instance, deviceType, credentialProvider.getId());
    demoDeviceAndStreamingAuthentication(instance, deviceType, credentialProvider.getId());
    // demoExplicitCredentialProviderSelection(instance, deviceType, credentialProvider.getId());
    demoAuthenticationConfigAsPropertyObject(instance, deviceType, credentialProvider.getId());
    demoCachedFilePathCredentialAcrossDeviceAndStreaming(instance, deviceType, credentialProvider.getId());
    demoPinAuthenticationAndReload(instance, deviceType, credentialProvider.getId());

    std::cout << "Press \"enter\" to exit the application..." << std::endl;
    std::cin.get();
    return 0;
}
