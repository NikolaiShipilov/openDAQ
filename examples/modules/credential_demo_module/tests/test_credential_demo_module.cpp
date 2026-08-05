#include <testutils/testutils.h>
#include <credential_demo_module/module_dll.h>
#include <credential_demo_module/version.h>

#include <opendaq/module_ptr.h>
#include <coretypes/common.h>
#include <coretypes/dictobject_factory.h>

#include <opendaq/context_factory.h>

using CredentialDemoModuleTest = testing::Test;
using namespace daq;

static ContextPtr contextWithModuleOptions(const DictPtr<IString, IBaseObject>& moduleOptions)
{
    auto options = Dict<IString, IBaseObject>();
    if (moduleOptions.assigned() && moduleOptions.getCount() > 0)
    {
        auto modules = Dict<IString, IBaseObject>();
        modules.set("CredentialDemoModule", moduleOptions);
        options.set("Modules", modules);
    }

    return NullContext(Logger(), TypeManager(), options);
}

static ModulePtr createModule(const ContextPtr& context = NullContext())
{
    ModulePtr module;
    createModule(&module, context);
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
    ASSERT_EQ(deviceInfo.getCount(), 1u);

    ASSERT_EQ(deviceInfo[0].getManufacturer(), "openDAQ");
    ASSERT_EQ(deviceInfo[0].getSerialNumber(), "0");
    ASSERT_EQ(deviceInfo[0].getConnectionString(), "daq.credential_demo://openDAQ_0");
}

TEST_F(CredentialDemoModuleTest, EnumerateDevicesWithModuleOptions)
{
    auto moduleOptions = Dict<IString, IBaseObject>({{"Manufacturer", "TestManufacturer"}, {"SerialNumber", "42"}});
    auto module = createModule(contextWithModuleOptions(moduleOptions));

    ListPtr<IDeviceInfo> deviceInfo;
    ASSERT_NO_THROW(deviceInfo = module.getAvailableDevices());
    ASSERT_EQ(deviceInfo.getCount(), 1u);

    ASSERT_EQ(deviceInfo[0].getManufacturer(), "TestManufacturer");
    ASSERT_EQ(deviceInfo[0].getSerialNumber(), "42");
    ASSERT_EQ(deviceInfo[0].getConnectionString(), "daq.credential_demo://TestManufacturer_42");
}

TEST_F(CredentialDemoModuleTest, GetAvailableDeviceTypes)
{
    auto module = createModule();

    DictPtr<IString, IDeviceType> deviceTypes;
    ASSERT_NO_THROW(deviceTypes = module.getAvailableDeviceTypes());
    ASSERT_EQ(deviceTypes.getCount(), 1u);
    ASSERT_TRUE(deviceTypes.hasKey("CredentialDemoDevice"));
}

TEST_F(CredentialDemoModuleTest, CreateDeviceConnectionStringNull)
{
    auto module = createModule();

    DevicePtr device;
    ASSERT_THROW(device = module.createDevice(nullptr, nullptr), ArgumentNullException);
}
