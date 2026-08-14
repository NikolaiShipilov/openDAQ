#include <opendaq/userpass_credential_payload_impl.h>
#include <coretypes/dictobject_factory.h>

BEGIN_NAMESPACE_OPENDAQ

UserPasswordCredentialPayloadImpl::UserPasswordCredentialPayloadImpl(const FunctionPtr& getUserPassCb)
    : getUsernameAndPasswordCallback(getUserPassCb)
{
    if (!getUsernameAndPasswordCallback.assigned())
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Function callback to obtain username and password must be assigned on corresponding credential payload creation");
}

ErrCode UserPasswordCredentialPayloadImpl::getSecrets(IBaseObject** secrets)
{
    OPENDAQ_PARAM_NOT_NULL(secrets);

    return daqTry([&]
    {
        const BaseObjectPtr result = getUsernameAndPasswordCallback();

        auto userNameAndPassword = result.asPtrOrNull<IDict, DictPtr<IString, IString>>();
        if (!userNameAndPassword.assigned())
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDTYPE, "Credential provider's callback did not return a UserName/Password dictionary");

        *secrets = userNameAndPassword.detach();
        return OPENDAQ_SUCCESS;
    });
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, UserPasswordCredentialPayload, ICredentialPayload, IFunction*, getUserPassCb)

END_NAMESPACE_OPENDAQ
