#include <opendaq/file_credential_provider_impl.h>
#include <opendaq/credential_payload_factory.h>
#include <coretypes/listobject_factory.h>
#include <iostream>

BEGIN_NAMESPACE_OPENDAQ

static const std::string FileCredentialProviderName = "FileCredentialProvider";

FileCredentialProviderImpl::FileCredentialProviderImpl()
{
}

ErrCode FileCredentialProviderImpl::getName(IString** name)
{
    OPENDAQ_PARAM_NOT_NULL(name);

    *name = String(FileCredentialProviderName).detach();
    return OPENDAQ_SUCCESS;
}

ErrCode FileCredentialProviderImpl::getSupportedPayloadFormats(IList** formats)
{
    OPENDAQ_PARAM_NOT_NULL(formats);

    auto supportedFormats = List<IInteger>();
    supportedFormats.pushBack(static_cast<Int>(CredentialPayloadFormat::FilePath));

    *formats = supportedFormats.detach();
    return OPENDAQ_SUCCESS;
}

ErrCode FileCredentialProviderImpl::requestCredentials(ICredentialRequest* request, ICredentialPayload** credentials)
{
    OPENDAQ_PARAM_NOT_NULL(credentials);
    OPENDAQ_PARAM_NOT_NULL(request);

    const auto requestPtr = CredentialRequestPtr::Borrow(request);
    const auto descriptor = requestPtr.getPayloadDescriptor();
    if (!descriptor.assigned())
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "Credential request has no payload descriptor set");

    if (descriptor.getFormat() != CredentialPayloadFormat::FilePath)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOT_SUPPORTED, "Unsupported credential payload format");

    auto callback = Function(
        [requestPtr, descriptor]()
        {
            printRequestDetails(requestPtr);
            return readFilePath(descriptor);
        });

    *credentials = StringCredentialPayload(callback).detach();
    return OPENDAQ_SUCCESS;
}

StringPtr FileCredentialProviderImpl::readFilePath(const CredentialPayloadDescriptorPtr& descriptor)
{
    const StringPtr description = descriptor.getDescription();

    std::cout << (description.assigned() ? description.toStdString() : "File path") << ": ";
    std::cout.flush();

    std::string value;
    if (!std::getline(std::cin, value))
        throw std::runtime_error("Input cancelled");

    return String(value);
}

void FileCredentialProviderImpl::printRequestDetails(const CredentialRequestPtr& request)
{
    std::cout << '\n';
    std::cout << "============================================================\n";
    std::cout << "Authentication required\n";
    std::cout << "============================================================\n\n";

    if (const auto type = request.getComponentType(); type.assigned())
        std::cout << "Component type : " << type.getName() << '\n';

    if (const auto connectionString = request.getConnectionString(); connectionString.assigned() && connectionString.getLength() > 0)
        std::cout << "Connection string : " << connectionString << '\n';

    if (const auto metaData = request.getMetaData(); metaData.assigned())
    {
        for (const auto& property : metaData.getAllProperties())
            std::cout << property.getDescription() << " (" << property.getName() << ") : " << metaData.getPropertyValue(property.getName()) << '\n';
    }

    std::cout << '\n';
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, FileCredentialProvider, ICredentialProvider)

END_NAMESPACE_OPENDAQ
