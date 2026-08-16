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
            "SerialNumber": "1234"
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

    auto instanceBuilder = InstanceBuilder();
    instanceBuilder.addModulePath(MODULE_PATH);
    instanceBuilder.addConfigProvider(JsonConfigProvider(JSON_CONFIG_FILE_NAME));
    instanceBuilder.addCredentialProvider(credentialProvider.getName(), credentialProvider);
    auto instance = instanceBuilder.build();

    // add without authentication
    auto device = instance.addDevice("daq://openDAQ_1234");
    std::cout << "Connected to \"" << device.getInfo().getName() << "\" without authentication. Press \"enter\" to continue..." << std::endl;
    std::cin.get();
    instance.removeDevice(device);

    // get the type to obtain default authentication settings
    auto deviceType = instance.getAvailableDeviceTypes().get("CredentialDemoDevice");

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

    std::cout << "Press \"enter\" to exit the application..." << std::endl;
    std::cin.get();
    return 0;
}
