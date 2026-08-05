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

int main(int /*argc*/, const char* /*argv*/[])
{
    using namespace daq;

    createJsonConfigFile();
    auto credentialProvider = CmdLineCredentialProvider();

    auto instanceBuilder = InstanceBuilder();
    instanceBuilder.addModulePath(MODULE_PATH);
    instanceBuilder.addConfigProvider(JsonConfigProvider(JSON_CONFIG_FILE_NAME));
    instanceBuilder.addCredentialProvider(credentialProvider.getName(), credentialProvider);
    auto instance = instanceBuilder.build();

    auto device = instance.addDevice("daq.credential_demo://openDAQ_1234");
    std::cout << "Connected to \"" << device.getInfo().getName() << "\"" << std::endl;

    std::cout << "Press \"enter\" to exit the application..." << std::endl;
    std::cin.get();
    return 0;
}
