#include <testutils/testutils.h>
#include <credential_demo_module/module_dll.h>
#include <credential_demo_module/version.h>

#include <opendaq/module_ptr.h>
#include <coretypes/common.h>

#include <opendaq/context_factory.h>

using CredentialDemoModuleTest = testing::Test;
using namespace daq;

static ModulePtr createModule()
{
    ModulePtr module;
    createModule(&module, NullContext());
    return module;
}

TEST_F(CredentialDemoModuleTest, CreateModule)
{
    IModule* module = nullptr;
    ErrCode errCode = createModule(&module, NullContext());
    ASSERT_TRUE(OPENDAQ_SUCCEEDED(errCode));

    ASSERT_NE(module, nullptr);
    module->releaseRef();
}

TEST_F(CredentialDemoModuleTest, ModuleName)
{
    auto module = createModule();
    ASSERT_EQ(module.getModuleInfo().getName(), "Credential demo module");
}

TEST_F(CredentialDemoModuleTest, ModuleId)
{
    auto module = createModule();
    ASSERT_EQ(module.getModuleInfo().getId(), "CredentialDemoModule");
}

TEST_F(CredentialDemoModuleTest, VersionAvailable)
{
    auto module = createModule();
    ASSERT_TRUE(module.getModuleInfo().getVersionInfo().assigned());
}

TEST_F(CredentialDemoModuleTest, VersionCorrect)
{
    auto module = createModule();
    auto version = module.getModuleInfo().getVersionInfo();

    ASSERT_EQ(version.getMajor(), CREDENTIAL_DEMO_MODULE_MAJOR_VERSION);
    ASSERT_EQ(version.getMinor(), CREDENTIAL_DEMO_MODULE_MINOR_VERSION);
    ASSERT_EQ(version.getPatch(), CREDENTIAL_DEMO_MODULE_PATCH_VERSION);
}

TEST_F(CredentialDemoModuleTest, EnumerateDevices)
{
    auto module = createModule();

    ListPtr<IDeviceInfo> deviceInfo;
    ASSERT_NO_THROW(deviceInfo = module.getAvailableDevices());
    ASSERT_EQ(deviceInfo.getCount(), static_cast<SizeT>(1));
}

TEST_F(CredentialDemoModuleTest, GetAvailableDeviceTypes)
{
    auto module = createModule();

    DictPtr<IString, IDeviceType> deviceTypes;
    ASSERT_NO_THROW(deviceTypes = module.getAvailableDeviceTypes());
    ASSERT_EQ(deviceTypes.getCount(), static_cast<SizeT>(1));
    ASSERT_TRUE(deviceTypes.hasKey("CredentialDemoDevice"));
}

TEST_F(CredentialDemoModuleTest, CreateDeviceConnectionStringNull)
{
    auto module = createModule();

    DevicePtr device;
    ASSERT_THROW(device = module.createDevice(nullptr, nullptr), ArgumentNullException);
}

TEST_F(CredentialDemoModuleTest, CreateDevice)
{
    auto module = createModule();

    DevicePtr device;
    ASSERT_NO_THROW(device = module.createDevice("daq.credential_demo://credential_demo_device", nullptr));
    ASSERT_TRUE(device.assigned());
}

TEST_F(CredentialDemoModuleTest, CreateDeviceAlreadyExists)
{
    auto module = createModule();

    DevicePtr device;
    ASSERT_NO_THROW(device = module.createDevice("daq.credential_demo://credential_demo_device", nullptr));
    ASSERT_THROW(module.createDevice("daq.credential_demo://credential_demo_device", nullptr), AlreadyExistsException);
}
