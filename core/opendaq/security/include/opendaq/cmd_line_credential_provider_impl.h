/*
 * Copyright 2022-2026 openDAQ d.o.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <coretypes/impl.h>
#include <opendaq/credential_provider.h>
#include <opendaq/credential_request_ptr.h>
#include <opendaq/credential_descriptor_ptr.h>
#include <coreobjects/property_object_ptr.h>
#include <map>
#include <utility>

BEGIN_NAMESPACE_OPENDAQ

class CmdLineCredentialProviderImpl : public ImplementationOf<ICredentialProvider>
{
public:
    explicit CmdLineCredentialProviderImpl();

    ErrCode INTERFACE_FUNC getId(IString** id) override;
    ErrCode INTERFACE_FUNC requestCredentials(ICredentialRequest* request, IPropertyObject** credentials) override;
    ErrCode INTERFACE_FUNC cacheCredentials(ICredentialRequest* request, IPropertyObject* secret) override;
    ErrCode INTERFACE_FUNC getSupportedFormats(IList** formats) override;

private:
    using CacheKey = std::pair<std::string, std::string>;

    static void printRequestDetails(const CredentialRequestPtr& request);
    static PropertyObjectPtr readKeyValuePairs(const CredentialDescriptorPtr& descriptor);
    static PropertyObjectPtr readStringSecret(const CredentialDescriptorPtr& descriptor);
    static std::string readLine(const std::string& prompt, bool hide);
    static CacheKey MakeFilePathCacheKey(const CredentialRequestPtr& request);

    // FilePath secrets only, cached in-memory for the lifetime of this provider (i.e. for the active
    // session) - keyed by (manufacturer, serialNumber), so re-authenticating a second connection to the
    // same device (e.g. attaching streaming after the device itself) reuses the path already entered
    // instead of prompting again.
    PropertyObjectPtr readFilePathSecretCached(const CredentialRequestPtr& request, const CredentialDescriptorPtr& descriptor);

    std::map<CacheKey, std::string> filePathSecretCache;
};

END_NAMESPACE_OPENDAQ
