#include <iostream>
#include <opendaq/opendaq.h>

int main(int /*argc*/, const char* /*argv*/[])
{
    using namespace daq;

    auto credentialProvider = CmdLineCredentialProvider();

    auto instanceBuilder = InstanceBuilder();
    instanceBuilder.addModulePath(MODULE_PATH);
    instanceBuilder.addCredentialProvider(credentialProvider.getName(), credentialProvider);
    auto instance = instanceBuilder.build();

    auto device = instance.addDevice("daq.credential_demo://credential_demo_device");
    std::cout << "Connected to \"" << device.getInfo().getName() << "\"" << std::endl;

    std::cout << "Press \"enter\" to exit the application..." << std::endl;
    std::cin.get();
    return 0;
}
