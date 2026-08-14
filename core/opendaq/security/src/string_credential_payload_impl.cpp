#include <opendaq/string_credential_payload_impl.h>
#include <coretypes/string_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

StringCredentialPayloadImpl::StringCredentialPayloadImpl(const FunctionPtr& getSecretCb)
    : getSecretCallback(getSecretCb)
{
    if (!getSecretCallback.assigned())
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Function callback to obtain the secret must be assigned on corresponding credential payload creation");
}

ErrCode StringCredentialPayloadImpl::getSecrets(IBaseObject** secrets)
{
    OPENDAQ_PARAM_NOT_NULL(secrets);

    return daqTry([&]
    {
        const BaseObjectPtr result = getSecretCallback();

        StringPtr secret = result.asPtrOrNull<IString>();
        if (!secret.assigned())
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDTYPE, "Credential provider's callback did not return a string secret");

        *secrets = secret.detach();
        return OPENDAQ_SUCCESS;
    });
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, StringCredentialPayload, ICredentialPayload, IFunction*, getSecretCb)

END_NAMESPACE_OPENDAQ
