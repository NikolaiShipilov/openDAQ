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
#include <coretypes/baseobject.h>
#include <coreobjects/property_object.h>
#include <opendaq/credential_request.h>
#include <opendaq/credential_payload_descriptor.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [interfaceLibrary(IInteger, "coretypes")]
 * [interfaceSmartPtr(IInteger, IntegerPtr, "<coretypes/integer.h>")]
 * [interfaceLibrary(IPropertyObject, "coreobjects")]
 * [interfaceSmartPtr(IPropertyObject, PropertyObjectPtr, "<coreobjects/property_object.h>")]
 */

/*!
 * @brief Supplies the secrets requested via a `ICredentialRequest` - e.g. by prompting the user, reading
 * from a file, or fetching from a secret store.
 */
DECLARE_OPENDAQ_INTERFACE(ICredentialProvider, IBaseObject)
{
    /*!
     * @brief Gets the id that uniquely identifies the credential provider - the same id used to key it
     * within `IInstanceBuilder::addCredentialProvider`/`IContext::getCredentialProviders`, and that
     * `IAuthenticationConfig`'s `"CredentialProviderId"` property names when a caller selects one explicitly.
     * @param[out] id The provider id.
     */
    virtual ErrCode INTERFACE_FUNC getId(IString** id) = 0;

    /*!
     * @brief Requests credentials for the given request, in the format described by its payload descriptor.
     * @param request The credential request to obtain credentials for.
     * @param[out] credentials The obtained credential payload - a property object built from the request's
     * payload descriptor's `createDefaultPayload` template, filled in with the obtained secret(s).
     */
    virtual ErrCode INTERFACE_FUNC requestCredentials(ICredentialRequest* request, IPropertyObject** credentials) = 0;

    /*!
     * @brief Accepts a secret already known in advance - e.g. supplied directly via
     * `IAuthenticationConfig`'s `"SuppliedSecret"` property - so an implementation that would otherwise cache a value
     * it obtained interactively (e.g. `CmdLineCredentialProvider`'s in-session caching of `FilePath`-format
     * secrets, keyed by (manufacturer, serialNumber)) caches this one the same way. A later interactive
     * `requestCredentials` call for the same context then reuses it instead of prompting again. Does not
     * itself produce a credential payload - the caller already has the secret and uses it directly.
     * @param request The credential request the secret is being supplied for.
     * @param secret The secret, shaped like the request's payload descriptor's `createDefaultPayload`
     * template - a property object filled in with the actual secret value(s).
     */
    virtual ErrCode INTERFACE_FUNC cacheCredentials(ICredentialRequest* request, IPropertyObject* secret) = 0;

    // [elementType(formats, IInteger)]
    /*!
     * @brief Gets a list of the credential payload formats this provider can provide. Used for
     * format-matching against a device type's supported payload formats.
     * @param[out] formats The list of supported payload formats.
     */
    virtual ErrCode INTERFACE_FUNC getSupportedPayloadFormats(IList** formats) = 0;
};

/*!
 * @brief Creates a `ICredentialProvider` that prompts the user for secrets via the command line.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, CmdLineCredentialProvider, ICredentialProvider)

/*!
 * @brief Creates a `ICredentialProvider` dedicated to file-backed secrets. Prompts for the file's path
 * via the command line, the same way `CmdLineCredentialProvider` does, and hands back the path itself for
 * a `FilePath`-format request.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, FileCredentialProvider, ICredentialProvider)

END_NAMESPACE_OPENDAQ
