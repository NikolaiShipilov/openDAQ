#include <iostream>
#include <fstream>
#include <opendaq/opendaq.h>

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

int main(int argc, const char* argv[])
{
    using namespace daq;

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

    DevicePtr device;

    // get the type to obtain default authentication settings
    auto deviceType = instance.getAvailableDeviceTypes().get("CredentialDemoDevice");

    // PrivateKeyFile authentication - another String-format credential payload, but instead of comparing a
    // fixed secret, the module verifies a signed challenge against the public key configured via the
    // "PublicKeyPath" module option (set above to keys/public_key.pem). When prompted, supply the path
    // to the matching private key.
    std::cout << "When prompted for the private-key path, enter: " << CREDENTIAL_DEMO_KEYS_DIR << "/private_key.pem" << std::endl;
    auto privateKeyFileConfig = deviceType.getSupportedAuthenticationConfigs().get("PrivateKeyFile");
    device = instance.addAuthenticatedDevice("daq://openDAQ_1234", nullptr, privateKeyFileConfig);
    std::cout << "Connected to \"" << device.getInfo().getName() << "\" with private-key challenge authentication. Press \"enter\" to continue..." << std::endl;
    std::cin.get();
    instance.removeDevice(device);

    // PrivateKeyBlob authentication - the same private-key challenge, but via a BinaryBlob-format
    // credential payload instead of a FilePath one: fileCredentialProvider still prompts for the file's
    // path, but now reads the file itself and hands the module the raw key bytes directly, so the module
    // never touches the file (or even learns its path).
    std::cout << "When prompted for the private-key path, enter: " << CREDENTIAL_DEMO_KEYS_DIR << "/private_key.pem" << std::endl;
    auto privateKeyBlobConfig = deviceType.getSupportedAuthenticationConfigs().get("PrivateKeyBlob");
    device = instance.addAuthenticatedDevice("daq://openDAQ_1234", nullptr, privateKeyBlobConfig);
    std::cout << "Connected to \"" << device.getInfo().getName() << "\" with private-key challenge authentication via a binary blob. Press \"enter\" to continue..." << std::endl;
    std::cin.get();
    instance.removeDevice(device);

    // add without authentication
    device = instance.addDevice("daq://openDAQ_1234");
    std::cout << "Connected to \"" << device.getInfo().getName() << "\" without authentication. Press \"enter\" to continue..." << std::endl;
    std::cin.get();
    instance.removeDevice(device);

    // authenticate with username and password
    // UserName/Password authentication - a KeyValuePairs-format credential payload.
    auto userNamePasswordConfig = deviceType.createDefaultAuthenticationConfig();
    device = instance.addAuthenticatedDevice("daq://openDAQ_1234", nullptr, userNamePasswordConfig);
    std::cout << "Connected to \"" << device.getInfo().getName() << "\" with UserName/Password authentication, non-verbose credential request. Press \"enter\" to continue..." << std::endl;
    std::cin.get();
    instance.removeDevice(device);

    // authenticate with username and password but not hide the password
    userNamePasswordConfig.getConfig().setPropertyValue("VerboseCredentialRequest", True);
    userNamePasswordConfig.getConfig().setPropertyValue("HidePasswordInput", False);
    device = instance.addAuthenticatedDevice("daq://openDAQ_1234", nullptr, userNamePasswordConfig);
    std::cout << "Connected to \"" << device.getInfo().getName() << "\" with UserName/Password authentication, verbose credential request. Press \"enter\" to continue..." << std::endl;
    std::cin.get();
    instance.removeDevice(device);

    // PIN authentication - an alternative, String-format credential payload.
    auto pinConfig = deviceType.getSupportedAuthenticationConfigs().get("Pin");
    device = instance.addAuthenticatedDevice("daq://openDAQ_1234", nullptr, pinConfig);
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

    std::cout << "Press \"enter\" to exit the application..." << std::endl;
    std::cin.get();
    return 0;
}
