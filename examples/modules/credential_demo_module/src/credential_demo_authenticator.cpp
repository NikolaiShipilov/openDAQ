#include <credential_demo_module/credential_demo_authenticator.h>
#include <credential_demo_module/common.h>

#include <opendaq/credential_payload_descriptor_factory.h>
#include <opendaq/credential_request_factory.h>
#include <coreobjects/exceptions.h>
#include <coreobjects/property_factory.h>
#include <coreobjects/property_object_factory.h>
#include <coretypes/dictobject_factory.h>
#include <vector>
#include <memory>

#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rand.h>

BEGIN_NAMESPACE_CREDENTIAL_DEMO_MODULE

namespace crypto
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

    // Proves that whoever supplied `privateKey` holds the private key matching the module's known
    // public key: a random challenge is signed with the claimed private key, then the signature is
    // checked against the known public key.
    bool VerifyPrivateKeyChallenge(const EvpPkeyPtr& privateKey, const std::string& publicKeyPath)
    {
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

namespace authentication
{

CredentialPayloadDescriptorPtr BuildUserNamePasswordDescriptor(bool hidePassword)
{
    return KeyValuePayloadDescriptor(Dict<IString, IBoolean>({{"UserName", False}, {"Password", hidePassword}}), "Username and password");
}

CredentialPayloadDescriptorPtr BuildPinDescriptor(bool hidePin)
{
    return StringPayloadDescriptor("PIN code", hidePin);
}

CredentialPayloadDescriptorPtr BuildPrivateKeyFileDescriptor()
{
    return FilePathPayloadDescriptor("Path to the PEM-encoded private key file");
}

PropertyObjectPtr BuildAdditionalConfig(const StringPtr& payloadId)
{
    auto config = PropertyObject();
    config.addProperty(BoolProperty("VerboseCredentialRequest", False));

    const std::string payloadIdStr = payloadId.toStdString();
    if (payloadIdStr == UserNamePasswordPayloadId)
        config.addProperty(BoolProperty("HidePasswordInput", True));
    else if (payloadIdStr == PinPayloadId)
        config.addProperty(BoolProperty("HidePinInput", True));

    return config;
}

static void PopulateCommonMetaData(const CredentialRequestBuilderPtr& builder, const ComponentTypePtr& componentType, bool verbose)
{
    builder.setComponentType(componentType);
    builder.addMetaDataProperty(StringPropertyBuilder("ComponentTypeName", componentType.getName()).setDescription("The openDAQ component type name").build());

    if (verbose)
    {
        builder.addMetaDataProperty(StringPropertyBuilder("ComponentTypeId", componentType.getId()).setDescription("The openDAQ component type ID").build());
        builder.addMetaDataProperty(StringPropertyBuilder("ComponentTypeDescription", componentType.getDescription()).setDescription("The openDAQ component type description").build());
    }
}

static CredentialRequestPtr CreateUserNamePasswordCredentialRequest(const StringPtr& connectionString,
                                                                     const StringPtr& manufacturer,
                                                                     const StringPtr& serialNumber,
                                                                     const PropertyObjectPtr& additionalConfig,
                                                                     bool verbose,
                                                                     const ComponentTypePtr& componentType)
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
    PopulateCommonMetaData(builder, componentType, verbose);

    return builder.build();
}

static CredentialRequestPtr CreatePinCredentialRequest(const StringPtr& connectionString,
                                                        const StringPtr& manufacturer,
                                                        const StringPtr& serialNumber,
                                                        const PropertyObjectPtr& additionalConfig,
                                                        bool verbose,
                                                        const ComponentTypePtr& componentType)
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
    PopulateCommonMetaData(builder, componentType, verbose);

    return builder.build();
}

static CredentialRequestPtr CreatePrivateKeyFileCredentialRequest(const StringPtr& connectionString,
                                                                   const StringPtr& manufacturer,
                                                                   const StringPtr& serialNumber,
                                                                   const PropertyObjectPtr& /*additionalConfig*/,
                                                                   bool verbose,
                                                                   const ComponentTypePtr& componentType)
{
    const auto payloadDescriptor = BuildPrivateKeyFileDescriptor();

    auto builder = CredentialRequestBuilder();
    builder.setConnectionString(connectionString);
    builder.setManufacturer(manufacturer);
    builder.setSerialNumber(serialNumber);
    builder.setPayloadId(PrivateKeyFilePayloadId);
    builder.setPayloadDescriptor(payloadDescriptor);
    PopulateCommonMetaData(builder, componentType, verbose);

    return builder.build();
}

CredentialRequestPtr CreateCredentialRequest(const StringPtr& payloadId,
                                              const StringPtr& connectionString,
                                              const StringPtr& manufacturer,
                                              const StringPtr& serialNumber,
                                              const PropertyObjectPtr& additionalConfig,
                                              bool verbose,
                                              const ComponentTypePtr& componentType)
{
    const std::string payloadIdStr = payloadId.toStdString();

    if (payloadIdStr == PinPayloadId)
        return CreatePinCredentialRequest(connectionString, manufacturer, serialNumber, additionalConfig, verbose, componentType);

    if (payloadIdStr == PrivateKeyFilePayloadId)
        return CreatePrivateKeyFileCredentialRequest(connectionString, manufacturer, serialNumber, additionalConfig, verbose, componentType);

    if (payloadIdStr == UserNamePasswordPayloadId)
        return CreateUserNamePasswordCredentialRequest(connectionString, manufacturer, serialNumber, additionalConfig, verbose, componentType);

    DAQ_THROW_EXCEPTION(InvalidParameterException, "Unknown authentication payload id \"{}\"", payloadId);
}

void Authenticate(const ContextPtr& ctx, const PropertyObjectPtr& credentials, const StringPtr& payloadId)
{
    if (!credentials.assigned())
    {
        DAQ_THROW_EXCEPTION(AuthenticationFailedException, "Failed to authenticate - no credentials provided");
    }

    const std::string payloadIdStr = payloadId.toStdString();

    if (payloadIdStr == PinPayloadId)
    {
        const StringPtr pin = credentials.hasProperty("Secret") ? credentials.getPropertyValue("Secret") : nullptr;
        if (!pin.assigned() || pin != "1234")
        {
            DAQ_THROW_EXCEPTION(AuthenticationFailedException, "Failed to authenticate - wrong pin-code");
        }
    }
    else if (payloadIdStr == PrivateKeyFilePayloadId)
    {
        const StringPtr privateKeyPath = credentials.hasProperty("Secret") ? credentials.getPropertyValue("Secret") : nullptr;
        if (!privateKeyPath.assigned() || privateKeyPath.getLength() == 0)
        {
            DAQ_THROW_EXCEPTION(AuthenticationFailedException, "Failed to authenticate - no private key file path provided");
        }

        crypto::EvpPkeyPtr privateKey = crypto::ReadPemKeyFile(privateKeyPath.toStdString(), /*isPrivateKey*/ true);

        if (!privateKey)
        {
            DAQ_THROW_EXCEPTION(AuthenticationFailedException, "Failed to authenticate - could not parse the supplied private key");
        }

        const auto moduleOptions = ctx.getModuleOptions(CREDENTIAL_DEMO_MODULE_ID);
        const StringPtr publicKeyPath = moduleOptions.hasKey("PublicKeyPath") ? moduleOptions.get("PublicKeyPath").asPtr<IString>() : nullptr;
        if (!publicKeyPath.assigned() || publicKeyPath.getLength() == 0)
        {
            DAQ_THROW_EXCEPTION(AuthenticationFailedException,
                                 "Failed to authenticate - module has no public key configured (set the \"PublicKeyPath\" module option)");
        }

        if (!crypto::VerifyPrivateKeyChallenge(privateKey, publicKeyPath.toStdString()))
        {
            DAQ_THROW_EXCEPTION(AuthenticationFailedException, "Failed to authenticate - private key challenge verification failed");
        }
    }
    else
    {
        const StringPtr userName = credentials.hasProperty("UserName") ? credentials.getPropertyValue("UserName") : nullptr;
        const StringPtr password = credentials.hasProperty("Password") ? credentials.getPropertyValue("Password") : nullptr;
        if (!userName.assigned() || userName != "user" || !password.assigned() || password != "pass")
        {
            DAQ_THROW_EXCEPTION(AuthenticationFailedException, "Failed to authenticate - wrong username or password");
        }
    }
}

}

END_NAMESPACE_CREDENTIAL_DEMO_MODULE
