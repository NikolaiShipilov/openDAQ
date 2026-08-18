#include <credential_demo_module/credential_demo_device_impl.h>

#include <opendaq/device_info_factory.h>
#include <opendaq/device_type_factory.h>
#include <opendaq/component_type_builder_factory.h>
#include <opendaq/credential_request_factory.h>
#include <opendaq/credential_payload_descriptor_factory.h>
#include <opendaq/server_capability_config.h>
#include <opendaq/device_info_internal.h>
#include <coretypes/dictobject_factory.h>
#include <coreobjects/property_factory.h>
#include <fmt/format.h>
#include <string_view>
#include <vector>
#include <memory>

#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rand.h>

BEGIN_NAMESPACE_CREDENTIAL_DEMO_MODULE

static constexpr std::string_view GenericDeviceAddress = "credential_demo_device";
static const std::string UserNamePasswordPayloadId = "UserNamePassword";
static const std::string PinPayloadId = "Pin";
static const std::string PrivateKeyPayloadId = "PrivateKey";

static CredentialPayloadDescriptorPtr BuildUserNamePasswordDescriptor(bool hidePassword)
{
    return KeyValuePayloadDescriptor(Dict<IString, IBoolean>({{"UserName", False}, {"Password", hidePassword}}), "Username and password");
}

static CredentialPayloadDescriptorPtr BuildPinDescriptor(bool hidePin)
{
    return StringPayloadDescriptor("PIN code", hidePin);
}

static CredentialPayloadDescriptorPtr BuildPrivateKeyDescriptor()
{
    return FilePathPayloadDescriptor("Path to the PEM-encoded private key file");
}

namespace
{
    using EvpPkeyPtr = std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)>;
    using EvpMdCtxPtr = std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)>;
    using BioPtr = std::unique_ptr<BIO, decltype(&BIO_free)>;

    EvpPkeyPtr ReadPemKeyFile(const std::string& path, bool isPrivateKey)
    {
        BioPtr bio(BIO_new_file(path.c_str(), "r"), &BIO_free);
        if (!bio)
            return EvpPkeyPtr(nullptr, &EVP_PKEY_free);

        EVP_PKEY* key = isPrivateKey
                            ? PEM_read_bio_PrivateKey(bio.get(), nullptr, nullptr, nullptr)
                            : PEM_read_bio_PUBKEY(bio.get(), nullptr, nullptr, nullptr);

        return EvpPkeyPtr(key, &EVP_PKEY_free);
    }

    // Proves that whoever supplied `privateKeyPath` holds the private key matching the module's known
    // public key, without the private key ever leaving the file it was read from: a random challenge is
    // signed with the claimed private key, then the signature is checked against the known public key.
    bool VerifyPrivateKeyChallenge(const std::string& privateKeyPath, const std::string& publicKeyPath)
    {
        auto privateKey = ReadPemKeyFile(privateKeyPath, /*isPrivateKey*/ true);
        if (!privateKey)
            return false;

        auto publicKey = ReadPemKeyFile(publicKeyPath, /*isPrivateKey*/ false);
        if (!publicKey)
            return false;

        unsigned char challenge[32];
        if (RAND_bytes(challenge, sizeof(challenge)) != 1)
            return false;

        EvpMdCtxPtr signCtx(EVP_MD_CTX_new(), &EVP_MD_CTX_free);
        if (!signCtx || EVP_DigestSignInit(signCtx.get(), nullptr, EVP_sha256(), nullptr, privateKey.get()) != 1)
            return false;

        size_t signatureLength = 0;
        if (EVP_DigestSign(signCtx.get(), nullptr, &signatureLength, challenge, sizeof(challenge)) != 1)
            return false;

        std::vector<unsigned char> signature(signatureLength);
        if (EVP_DigestSign(signCtx.get(), signature.data(), &signatureLength, challenge, sizeof(challenge)) != 1)
            return false;

        EvpMdCtxPtr verifyCtx(EVP_MD_CTX_new(), &EVP_MD_CTX_free);
        if (!verifyCtx || EVP_DigestVerifyInit(verifyCtx.get(), nullptr, EVP_sha256(), nullptr, publicKey.get()) != 1)
            return false;

        return EVP_DigestVerify(verifyCtx.get(), signature.data(), signatureLength, challenge, sizeof(challenge)) == 1;
    }
}

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

void CredentialDemoDeviceImpl::authenticate(const CredentialPayloadPtr& credentials, const StringPtr& payloadId, const StringPtr& publicKeyPath)
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
    else if (payloadIdStr == PrivateKeyPayloadId)
    {
        const StringPtr privateKeyPath = secrets.asPtrOrNull<IString>();
        if (!privateKeyPath.assigned() || privateKeyPath.getLength() == 0)
        {
            DAQ_THROW_EXCEPTION(AuthenticationFailedException, "Failed to authenticate device - no private key file path provided");
        }

        if (!publicKeyPath.assigned() || publicKeyPath.getLength() == 0)
        {
            DAQ_THROW_EXCEPTION(AuthenticationFailedException,
                                 "Failed to authenticate device - module has no public key configured (set the \"PublicKeyPath\" module option)");
        }

        if (!VerifyPrivateKeyChallenge(privateKeyPath.toStdString(), publicKeyPath.toStdString()))
        {
            DAQ_THROW_EXCEPTION(AuthenticationFailedException, "Failed to authenticate device - private key challenge verification failed");
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
                                                   const CredentialPayloadPtr& credentials,
                                                   const StringPtr& publicKeyPath)
    : Device(ctx, parent, fmt::format("{}_{}", info.getManufacturer(), info.getSerialNumber()), nullptr, info.getName())
{
    if (authenticated)
        authenticate(credentials, payloadId, publicKeyPath);

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
    auto userNamePasswordDescriptor = BuildUserNamePasswordDescriptor(/*hidePassword*/ true);
    auto pinDescriptor = BuildPinDescriptor(/*hidePin*/ true);
    auto privateKeyDescriptor = BuildPrivateKeyDescriptor();

    auto userNamePasswordConfig = PropertyObject();
    userNamePasswordConfig.addProperty(BoolProperty("VerboseCredentialRequest", False));
    userNamePasswordConfig.addProperty(BoolProperty("HidePasswordInput", True));

    auto pinConfig = PropertyObject();
    pinConfig.addProperty(BoolProperty("VerboseCredentialRequest", False));
    pinConfig.addProperty(BoolProperty("HidePinInput", True));

    auto privateKeyConfig = PropertyObject();
    privateKeyConfig.addProperty(BoolProperty("VerboseCredentialRequest", False));

    return DeviceTypeBuilder()
        .setId("CredentialDemoDevice")
        .setName("Credential demo device")
        .setDescription("openDAQ authentication/credential framework showcase device")
        .setConnectionStringPrefix("daq.credential_demo")
        .addSupportedAuthenticationConfig(UserNamePasswordPayloadId, userNamePasswordDescriptor, userNamePasswordConfig)
        .addSupportedAuthenticationConfig(PinPayloadId, pinDescriptor, pinConfig)
        .addSupportedAuthenticationConfig(PrivateKeyPayloadId, privateKeyDescriptor, privateKeyConfig)
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

    if (payloadIdStr == PrivateKeyPayloadId)
        return CreatePrivateKeyCredentialRequest(connectionString, manufacturer, serialNumber, additionalConfig, verbose);

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
    const auto payloadDescriptor = BuildUserNamePasswordDescriptor(hidePassword);

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
    const auto payloadDescriptor = BuildPinDescriptor(hidePin);

    auto builder = CredentialRequestBuilder();
    builder.setConnectionString(connectionString);
    builder.setManufacturer(manufacturer);
    builder.setSerialNumber(serialNumber);
    builder.setPayloadId(PinPayloadId);
    builder.setPayloadDescriptor(payloadDescriptor);
    PopulateCommonMetaData(builder, CreateType(), verbose);

    return builder.build();
}

CredentialRequestPtr CredentialDemoDeviceImpl::CreatePrivateKeyCredentialRequest(const StringPtr& connectionString,
                                                                                  const StringPtr& manufacturer,
                                                                                  const StringPtr& serialNumber,
                                                                                  const PropertyObjectPtr& additionalConfig,
                                                                                  bool verbose)
{
    const auto payloadDescriptor = BuildPrivateKeyDescriptor();

    auto builder = CredentialRequestBuilder();
    builder.setConnectionString(connectionString);
    builder.setManufacturer(manufacturer);
    builder.setSerialNumber(serialNumber);
    builder.setPayloadId(PrivateKeyPayloadId);
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
