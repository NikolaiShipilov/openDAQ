#include <credential_demo_module/credential_demo_device_impl.h>

#include <opendaq/device_info_factory.h>
#include <opendaq/device_type_factory.h>
#include <opendaq/component_type_builder_factory.h>
#include <opendaq/credential_request_factory.h>
#include <opendaq/credential_payload_descriptor_factory.h>
#include <opendaq/server_capability_config.h>
#include <opendaq/device_info_internal.h>
#include <coretypes/listobject_factory.h>
#include <coreobjects/property_factory.h>
#include <fmt/format.h>
#include <string_view>

BEGIN_NAMESPACE_CREDENTIAL_DEMO_MODULE

static constexpr std::string_view GenericDeviceAddress = "credential_demo_device";
static const std::string UserNamePasswordPayloadId = "UserNamePassword";
static const std::string PinPayloadId = "Pin";

void CredentialDemoDeviceImpl::authenticate(const CredentialPayloadPtr& credentials, const StringPtr& payloadId)
{
    if (!credentials.assigned())
    {
        DAQ_THROW_EXCEPTION(AuthenticationFailedException, "Failed to authenticate device - no credentials provided");
    }

    const BaseObjectPtr secrets = credentials.getSecrets();
    const std::string payloadIdStr = payloadId.toStdString();

    if (payloadIdStr == PinPayloadId)
    {
        const StringPtr pin = secrets.asPtrOrNull<IString>();
        if (!pin.assigned() || pin != "1234")
        {
            DAQ_THROW_EXCEPTION(AuthenticationFailedException, "Failed to authenticate device - wrong pin-code");
        }
    }
    else
    {
        const auto userNameAndPassword = secrets.asPtrOrNull<IDict, DictPtr<IString, IString>>(true);
        if (!userNameAndPassword.assigned() ||
            !userNameAndPassword.hasKey("UserName") || userNameAndPassword.get("UserName") != "user" ||
            !userNameAndPassword.hasKey("Password") || userNameAndPassword.get("Password") != "pass")
        {
            DAQ_THROW_EXCEPTION(AuthenticationFailedException, "Failed to authenticate device - wrong username or password");
        }
    }
}

CredentialDemoDeviceImpl::CredentialDemoDeviceImpl(const PropertyObjectPtr& config,
                                                   const ContextPtr& ctx,
                                                   const ComponentPtr& parent,
                                                   const DeviceInfoPtr& info,
                                                   bool authenticated,
                                                   const StringPtr& payloadId,
                                                   const CredentialPayloadPtr& credentials)
    : Device(ctx, parent, fmt::format("{}_{}", info.getManufacturer(), info.getSerialNumber()), nullptr, info.getName())
{
    if (authenticated)
        authenticate(credentials, payloadId);

    this->deviceInfo = info;
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
    auto userNamePasswordConfig = PropertyObject();
    userNamePasswordConfig.addProperty(BoolProperty("VerboseCredentialRequest", False));
    userNamePasswordConfig.addProperty(BoolProperty("HideSecretInput", True));

    auto pinConfig = PropertyObject();
    pinConfig.addProperty(BoolProperty("VerboseCredentialRequest", False));
    pinConfig.addProperty(BoolProperty("HideSecretInput", True));

    auto userNamePasswordDescriptor = KeyValuePayloadDescriptor(List<IString>("UserName", "Password"), "Username and password");
    auto pinDescriptor = StringPayloadDescriptor("PIN code");

    return DeviceTypeBuilder()
        .setId("CredentialDemoDevice")
        .setName("Credential demo device")
        .setDescription("openDAQ authentication/credential framework showcase device")
        .setConnectionStringPrefix("daq.credential_demo")
        .addSupportedAuthenticationConfig(UserNamePasswordPayloadId, userNamePasswordDescriptor, userNamePasswordConfig)
        .addSupportedAuthenticationConfig(PinPayloadId, pinDescriptor, pinConfig)
        .setDefaultAuthenticationConfigId(UserNamePasswordPayloadId)
        .build();
}

CredentialRequestPtr CredentialDemoDeviceImpl::CreateCredentialRequest(const StringPtr& connectionString,
                                                                       const StringPtr& manufacturer,
                                                                       const StringPtr& serialNumber,
                                                                       const StringPtr& payloadId,
                                                                       const CredentialPayloadDescriptorPtr& payloadDescriptor,
                                                                       bool verbose,
                                                                       bool hideSecretInput)
{
    auto deviceType = CreateType();

    auto builder = CredentialRequestBuilder();
    builder.setConnectionString(connectionString);
    builder.setManufacturer(manufacturer);
    builder.setSerialNumber(serialNumber);
    builder.setPayloadId(payloadId);
    builder.setPayloadDescriptor(payloadDescriptor);
    builder.addMetaDataProperty(StringPropertyBuilder("DeviceTypeName", deviceType.getName()).setDescription("The openDAQ device type name").build());
    builder.addMetaDataProperty(
        BoolPropertyBuilder("HideSecretInput", hideSecretInput)
            .setDescription("Whether the credential provider should mask secret input as it is entered")
            .build());
    if (verbose)
    {
        builder.addMetaDataProperty(StringPropertyBuilder("DeviceTypeId", deviceType.getId()).setDescription("The openDAQ device type ID").build());
        builder.addMetaDataProperty(StringPropertyBuilder("DeviceTypeDescription", deviceType.getDescription()).setDescription("The openDAQ device type description").build());
    }

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
