#include <credential_demo_module/credential_demo_device_impl.h>

#include <opendaq/device_info_factory.h>
#include <opendaq/device_type_factory.h>
#include <opendaq/component_type_builder_factory.h>
#include <opendaq/credential_request_factory.h>
#include <opendaq/server_capability_config.h>
#include <opendaq/device_info_internal.h>
#include <opendaq/streaming_ptr.h>
#include <coreobjects/property_factory.h>
#include <fmt/format.h>
#include <string_view>

BEGIN_NAMESPACE_CREDENTIAL_DEMO_MODULE

static constexpr std::string_view GenericDeviceAddress = "credential_demo_device";

static void PopulateCommonMetaData(const CredentialRequestBuilderPtr& builder, const DeviceTypePtr& deviceType, bool verbose)
{
    builder.setComponentType(deviceType);
    builder.addMetaDataProperty(StringPropertyBuilder("DeviceTypeName", deviceType.getName()).setDescription("The openDAQ device type name").build());

    if (verbose)
    {
        builder.addMetaDataProperty(StringPropertyBuilder("DeviceTypeId", deviceType.getId()).setDescription("The openDAQ device type ID").build());
        builder.addMetaDataProperty(StringPropertyBuilder("DeviceTypeDescription", deviceType.getDescription()).setDescription("The openDAQ device type description").build());
    }
}

CredentialDemoDeviceImpl::CredentialDemoDeviceImpl(const PropertyObjectPtr& config,
                                                   const ContextPtr& ctx,
                                                   const ComponentPtr& parent,
                                                   const DeviceInfoPtr& info,
                                                   bool authenticated,
                                                   const StringPtr& payloadId,
                                                   const CredentialPayloadPtr& credentials)
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

    return devInfo;
}

DeviceTypePtr CredentialDemoDeviceImpl::CreateType()
{
    auto userNamePasswordDescriptor = authentication::BuildUserNamePasswordDescriptor(/*hidePassword*/ true);
    auto pinDescriptor = authentication::BuildPinDescriptor(/*hidePin*/ true);
    auto privateKeyDescriptor = authentication::BuildPrivateKeyFileDescriptor();
    auto privateKeyBlobDescriptor = authentication::BuildPrivateKeyBlobDescriptor();

    auto userNamePasswordConfig = authentication::BuildAdditionalConfig(UserNamePasswordPayloadId);
    auto pinConfig = authentication::BuildAdditionalConfig(PinPayloadId);
    auto privateKeyConfig = authentication::BuildAdditionalConfig(PrivateKeyFilePayloadId);
    auto privateKeyBlobConfig = authentication::BuildAdditionalConfig(PrivateKeyBlobPayloadId);

    return DeviceTypeBuilder()
        .setId("CredentialDemoDevice")
        .setName("Credential demo device")
        .setDescription("openDAQ authentication/credential framework showcase device")
        .setConnectionStringPrefix("daq.credential_demo")
        .addSupportedAuthenticationConfig(UserNamePasswordPayloadId, userNamePasswordDescriptor, userNamePasswordConfig)
        .addSupportedAuthenticationConfig(PinPayloadId, pinDescriptor, pinConfig)
        .addSupportedAuthenticationConfig(PrivateKeyFilePayloadId, privateKeyDescriptor, privateKeyConfig)
        .addSupportedAuthenticationConfig(PrivateKeyBlobPayloadId, privateKeyBlobDescriptor, privateKeyBlobConfig)
        .setDefaultAuthenticationConfigId(UserNamePasswordPayloadId)
        .build();
}

CredentialRequestPtr CredentialDemoDeviceImpl::CreateCredentialRequest(const StringPtr& payloadId,
                                                                        const StringPtr& connectionString,
                                                                        const StringPtr& manufacturer,
                                                                        const StringPtr& serialNumber,
                                                                        const PropertyObjectPtr& additionalConfig,
                                                                        bool verbose)
{
    const std::string payloadIdStr = payloadId.toStdString();

    if (payloadIdStr == PinPayloadId)
        return CreatePinCredentialRequest(connectionString, manufacturer, serialNumber, additionalConfig, verbose);

    if (payloadIdStr == PrivateKeyFilePayloadId)
        return CreatePrivateKeyFileCredentialRequest(connectionString, manufacturer, serialNumber, additionalConfig, verbose);

    if (payloadIdStr == PrivateKeyBlobPayloadId)
        return CreatePrivateKeyBlobCredentialRequest(connectionString, manufacturer, serialNumber, additionalConfig, verbose);

    if (payloadIdStr == UserNamePasswordPayloadId)
        return CreateUserNamePasswordCredentialRequest(connectionString, manufacturer, serialNumber, additionalConfig, verbose);

    DAQ_THROW_EXCEPTION(InvalidParameterException, "Unknown authentication payload id \"{}\"", payloadId);
}

CredentialRequestPtr CredentialDemoDeviceImpl::CreateUserNamePasswordCredentialRequest(const StringPtr& connectionString,
                                                                                       const StringPtr& manufacturer,
                                                                                       const StringPtr& serialNumber,
                                                                                       const PropertyObjectPtr& additionalConfig,
                                                                                       bool verbose)
{
    const bool hidePassword = additionalConfig.assigned() && additionalConfig.hasProperty("HidePasswordInput")
                                   ? (bool) additionalConfig.getPropertyValue("HidePasswordInput")
                                   : true;
    const auto payloadDescriptor = authentication::BuildUserNamePasswordDescriptor(hidePassword);

    auto builder = CredentialRequestBuilder();
    builder.setConnectionString(connectionString);
    builder.setManufacturer(manufacturer);
    builder.setSerialNumber(serialNumber);
    builder.setPayloadId(UserNamePasswordPayloadId);
    builder.setPayloadDescriptor(payloadDescriptor);
    PopulateCommonMetaData(builder, CreateType(), verbose);

    return builder.build();
}

CredentialRequestPtr CredentialDemoDeviceImpl::CreatePinCredentialRequest(const StringPtr& connectionString,
                                                                          const StringPtr& manufacturer,
                                                                          const StringPtr& serialNumber,
                                                                          const PropertyObjectPtr& additionalConfig,
                                                                          bool verbose)
{
    const bool hidePin = additionalConfig.assigned() && additionalConfig.hasProperty("HidePinInput")
                              ? (bool) additionalConfig.getPropertyValue("HidePinInput")
                              : true;
    const auto payloadDescriptor = authentication::BuildPinDescriptor(hidePin);

    auto builder = CredentialRequestBuilder();
    builder.setConnectionString(connectionString);
    builder.setManufacturer(manufacturer);
    builder.setSerialNumber(serialNumber);
    builder.setPayloadId(PinPayloadId);
    builder.setPayloadDescriptor(payloadDescriptor);
    PopulateCommonMetaData(builder, CreateType(), verbose);

    return builder.build();
}

CredentialRequestPtr CredentialDemoDeviceImpl::CreatePrivateKeyFileCredentialRequest(const StringPtr& connectionString,
                                                                                      const StringPtr& manufacturer,
                                                                                      const StringPtr& serialNumber,
                                                                                      const PropertyObjectPtr& additionalConfig,
                                                                                      bool verbose)
{
    const auto payloadDescriptor = authentication::BuildPrivateKeyFileDescriptor();

    auto builder = CredentialRequestBuilder();
    builder.setConnectionString(connectionString);
    builder.setManufacturer(manufacturer);
    builder.setSerialNumber(serialNumber);
    builder.setPayloadId(PrivateKeyFilePayloadId);
    builder.setPayloadDescriptor(payloadDescriptor);
    PopulateCommonMetaData(builder, CreateType(), verbose);

    return builder.build();
}

CredentialRequestPtr CredentialDemoDeviceImpl::CreatePrivateKeyBlobCredentialRequest(const StringPtr& connectionString,
                                                                                      const StringPtr& manufacturer,
                                                                                      const StringPtr& serialNumber,
                                                                                      const PropertyObjectPtr& additionalConfig,
                                                                                      bool verbose)
{
    const auto payloadDescriptor = authentication::BuildPrivateKeyBlobDescriptor();

    auto builder = CredentialRequestBuilder();
    builder.setConnectionString(connectionString);
    builder.setManufacturer(manufacturer);
    builder.setSerialNumber(serialNumber);
    builder.setPayloadId(PrivateKeyBlobPayloadId);
    builder.setPayloadDescriptor(payloadDescriptor);
    PopulateCommonMetaData(builder, CreateType(), verbose);

    return builder.build();
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
