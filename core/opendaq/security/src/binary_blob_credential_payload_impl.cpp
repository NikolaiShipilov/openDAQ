#include <opendaq/binary_blob_credential_payload_impl.h>
#include <coretypes/binarydata_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

BinaryBlobCredentialPayloadImpl::BinaryBlobCredentialPayloadImpl(const FunctionPtr& getBlobCb)
    : getBlobCallback(getBlobCb)
{
    if (!getBlobCallback.assigned())
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Function callback to obtain the blob must be assigned on corresponding credential payload creation");
}

ErrCode BinaryBlobCredentialPayloadImpl::getSecrets(IBaseObject** secrets)
{
    OPENDAQ_PARAM_NOT_NULL(secrets);

    return daqTry([&]
    {
        const BaseObjectPtr result = getBlobCallback();

        BinaryDataPtr blob = result.asPtrOrNull<IBinaryData>();
        if (!blob.assigned())
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDTYPE, "Credential provider's callback did not return a binary blob secret");

        *secrets = blob.detach();
        return OPENDAQ_SUCCESS;
    });
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, BinaryBlobCredentialPayload, ICredentialPayload, IFunction*, getBlobCb)

END_NAMESPACE_OPENDAQ
