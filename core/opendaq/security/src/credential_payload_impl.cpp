#include <opendaq/credential_payload_impl.h>

BEGIN_NAMESPACE_OPENDAQ

template <typename SecretInterface, typename SecretPtr>
CredentialPayloadImpl<SecretInterface, SecretPtr>::CredentialPayloadImpl(const FunctionPtr& getSecretCb)
    : getSecretCallback(getSecretCb)
{
    if (!getSecretCallback.assigned())
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Function callback to obtain the secret must be assigned on corresponding credential payload creation");
}

template <typename SecretInterface, typename SecretPtr>
ErrCode CredentialPayloadImpl<SecretInterface, SecretPtr>::getSecrets(IBaseObject** secrets)
{
    OPENDAQ_PARAM_NOT_NULL(secrets);

    return daqTry([&]
    {
        const BaseObjectPtr result = getSecretCallback();

        SecretPtr secret = result.asPtrOrNull<SecretInterface, SecretPtr>();
        if (!secret.assigned())
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDTYPE, "Credential provider's callback did not return a secret of the expected type");

        *secrets = secret.detach();
        return OPENDAQ_SUCCESS;
    });
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, KeyValueCredentialPayload, ICredentialPayload, IFunction*, getValuesCb)
OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, StringCredentialPayload, ICredentialPayload, IFunction*, getSecretCb)
OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, BinaryBlobCredentialPayload, ICredentialPayload, IFunction*, getBlobCb)

END_NAMESPACE_OPENDAQ
