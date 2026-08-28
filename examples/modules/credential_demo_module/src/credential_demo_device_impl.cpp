#include <credential_demo_module/credential_demo_device_impl.h>
#include <credential_demo_module/credential_demo_streaming_impl.h>

#include <opendaq/device_info_factory.h>
#include <opendaq/device_type_factory.h>
#include <opendaq/component_type_builder_factory.h>
#include <opendaq/credential_request_factory.h>
#include <opendaq/credential_payload_descriptor_factory.h>
#include <opendaq/server_capability_config.h>
#include <opendaq/device_info_internal.h>
#include <opendaq/streaming_ptr.h>
#include <fmt/format.h>
#include <string_view>

BEGIN_NAMESPACE_CREDENTIAL_DEMO_MODULE

static constexpr std::string_view GenericDeviceAddress = "credential_demo_device";

CredentialDemoDeviceImpl::CredentialDemoDeviceImpl(const PropertyObjectPtr& config,
                                                   const ContextPtr& ctx,
                                                   const ComponentPtr& parent,
                                                   const DeviceInfoPtr& info,
                                                   bool authenticated,
                                                   const StringPtr& payloadId,
                                                   const PropertyObjectPtr& credentials)
    : MirroredDevice(ctx, parent, fmt::format("{}_{}", info.getManufacturer(), info.getSerialNumber()), nullptr, info.getName())
{
    if (authenticated)
    {
        authentication::Authenticate(ctx, credentials, payloadId);
    }

    this->deviceInfo = info;
}

StringPtr CredentialDemoDeviceImpl::onGetRemoteId() const
{
    // No real remote counterpart exists for this in-process demo device - its own local id (the same
    // id it's incorporated into the local component tree under) doubles as its remote id.
    return localId;
}

bool CredentialDemoDeviceImpl::isAddedToLocalComponentTree()
{
    return true;
}

DeviceInfoPtr CredentialDemoDeviceImpl::CreateDeviceInfo(const DictPtr<IString, IBaseObject>& moduleOptions)
{
    const StringPtr manufacturer = moduleOptions.get("Manufacturer");
    const StringPtr serialNumber = moduleOptions.get("SerialNumber");

    auto connectionString = fmt::format("daq.credential_demo://{}", GenericDeviceAddress);
    auto devInfo = DeviceInfo(connectionString);
    devInfo.setName("Credential demo device");
    devInfo.setManufacturer(manufacturer);
    devInfo.setModel("Credential demo device");
    devInfo.setSerialNumber(serialNumber);
    devInfo.setDeviceType(CreateType());

    auto capability = ServerCapability("CredentialDemo", "Credential Demo", ProtocolType::Configuration)
                           .setPrefix(CreateType().getConnectionStringPrefix())
                           .setConnectionString(connectionString);
    devInfo.asPtr<IDeviceInfoInternal>(true).addServerCapability(capability);

    // The`addDevice`'s automatic streaming attach (`PrioritizedStreamingProtocols`)
    // can pick "CredentialDemoStreaming" up for this device without any manual `addStreaming` call.
    auto streamingConnectionString =
        fmt::format("{}://{}", CredentialDemoStreamingImpl::CreateType().getConnectionStringPrefix(), GenericDeviceAddress);
    auto streamingCapability = ServerCapability("CredentialDemoStreaming", "Credential Demo Streaming", ProtocolType::Streaming)
                                    .setPrefix(CredentialDemoStreamingImpl::CreateType().getConnectionStringPrefix())
                                    .setConnectionString(streamingConnectionString);
    devInfo.asPtr<IDeviceInfoInternal>(true).addServerCapability(streamingCapability);

    return devInfo;
}

DeviceTypePtr CredentialDemoDeviceImpl::CreateType(const TypeManagerPtr& typeManager)
{
    auto userNamePasswordDescriptor = StandardUserNamePasswordPayloadDescriptor(typeManager);
    auto pinDescriptor = StandardPinPayloadDescriptor(typeManager);
    auto privateKeyDescriptor = StandardPrivateKeyFilePayloadDescriptor(typeManager);

    return DeviceTypeBuilder()
        .setId("CredentialDemoDevice")
        .setName("Credential demo device")
        .setDescription("openDAQ authentication/credential framework showcase device")
        .setConnectionStringPrefix("daq.credential_demo")
        .addSupportedAuthenticationDescriptor(userNamePasswordDescriptor)
        .addSupportedAuthenticationDescriptor(pinDescriptor)
        .addSupportedAuthenticationDescriptor(privateKeyDescriptor)
        .setDefaultAuthenticationConfigId(StandardUserNamePasswordPayloadId)
        .build();
}

void CredentialDemoDeviceImpl::ValidateConnectionString(const StringPtr& connectionString)
{
    const std::string prefix = fmt::format("{}://", CreateType().getConnectionStringPrefix());
    const std::string connStr = connectionString;
    if (connStr.find(prefix) != 0)
    {
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Invalid connection string \"{}\", no prefix", connectionString);
    }

    const auto address = connStr.substr(prefix.size());
    if (address != GenericDeviceAddress)
    {
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Invalid connection string \"{}\", unknown device address", connectionString);
    }
}

END_NAMESPACE_CREDENTIAL_DEMO_MODULE
