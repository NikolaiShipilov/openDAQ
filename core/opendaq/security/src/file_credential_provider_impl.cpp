#include <opendaq/file_credential_provider_impl.h>
#include <opendaq/credential_payload_factory.h>
#include <coreobjects/exceptions.h>
#include <coretypes/listobject_factory.h>
#include <coretypes/binarydata_factory.h>
#include <iostream>
#include <fstream>
#include <vector>

BEGIN_NAMESPACE_OPENDAQ

static const std::string FileCredentialProviderName = "FileCredentialProvider";
static constexpr int MaxFilePathAttempts = 3;

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
    supportedFormats.pushBack(static_cast<Int>(CredentialPayloadFormat::BinaryBlob));

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

    switch (descriptor.getFormat())
    {
        case CredentialPayloadFormat::FilePath:
        {
            auto callback = Function(
                [requestPtr, descriptor]()
                {
                    printRequestDetails(requestPtr);
                    return readFilePath(descriptor);
                });

            *credentials = StringCredentialPayload(callback).detach();
            return OPENDAQ_SUCCESS;
        }
        case CredentialPayloadFormat::BinaryBlob:
        {
            auto callback = Function(
                [requestPtr, descriptor]()
                {
                    printRequestDetails(requestPtr);
                    return readFileBlob(descriptor);
                });

            *credentials = BinaryBlobCredentialPayload(callback).detach();
            return OPENDAQ_SUCCESS;
        }
        default:
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOT_SUPPORTED, "Unsupported credential payload format");
    }
}

StringPtr FileCredentialProviderImpl::readFilePath(const CredentialPayloadDescriptorPtr& descriptor)
{
    const StringPtr description = descriptor.getDescription();
    const std::string prompt = (description.assigned() ? description.toStdString() : "File path") + ": ";

    for (int attempt = 1; attempt <= MaxFilePathAttempts; ++attempt)
    {
        std::cout << prompt;
        std::cout.flush();

        std::string value;
        if (!std::getline(std::cin, value))
            throw std::runtime_error("Input cancelled");

        if (isFileAccessible(value))
            return String(value);

        const int attemptsLeft = MaxFilePathAttempts - attempt;
        std::cout << "File \"" << value << "\" does not exist or is not accessible.";
        if (attemptsLeft > 0)
            std::cout << " " << attemptsLeft << " attempt(s) left.\n";
        else
            std::cout << '\n';
    }

    DAQ_THROW_EXCEPTION(AuthenticationFailedException,
                         "Credential provider could not obtain an accessible file path after {} attempts", MaxFilePathAttempts);
}

BinaryDataPtr FileCredentialProviderImpl::readFileBlob(const CredentialPayloadDescriptorPtr& descriptor)
{
    const std::string path = readFilePath(descriptor).toStdString();

    std::ifstream file(path, std::ios::binary | std::ios::ate);
    const std::streamsize size = file.tellg();
    if (!file || size <= 0)
        DAQ_THROW_EXCEPTION(AuthenticationFailedException, "Credential provider could not read file \"{}\"", path);

    std::vector<char> buffer(static_cast<size_t>(size));
    file.seekg(0);
    if (!file.read(buffer.data(), size))
        DAQ_THROW_EXCEPTION(AuthenticationFailedException, "Credential provider could not read file \"{}\"", path);

    return BinaryData(buffer.data(), static_cast<SizeT>(size));
}

bool FileCredentialProviderImpl::isFileAccessible(const std::string& path)
{
    return std::ifstream(path).good();
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
