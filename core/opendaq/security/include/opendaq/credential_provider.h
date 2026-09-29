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
#include <opendaq/authentication_method.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [interfaceLibrary(IInteger, "coretypes")]
 * [interfaceSmartPtr(IInteger, IntegerPtr, "<coretypes/integer.h>")]
 * [interfaceLibrary(IPropertyObject, "coreobjects")]
 * [interfaceSmartPtr(IPropertyObject, PropertyObjectPtr, "<coreobjects/property_object.h>")]
 */

/*!
 * @brief Supplies the credentials requested via a `ICredentialRequest` - e.g. by prompting the user, reading
 * from a file, or fetching from a credential store.
 */
DECLARE_OPENDAQ_INTERFACE(ICredentialProvider, IBaseObject)
{
    /*!
     * @brief Gets a human-readable description of the credential provider.
     * @param[out] description The provider's description.
     */
    virtual ErrCode INTERFACE_FUNC getDescription(IString** description) = 0;

    /*!
     * @brief Requests credentials for the given request, in the format described by its authentication method.
     * @param request The credential request to obtain credentials for.
     * @param[out] credentials The obtained credential - a property object built from the request's
     * authentication method's `createEmptyCredential` template, filled in with the obtained value(s).
     */
    virtual ErrCode INTERFACE_FUNC requestCredentials(ICredentialRequest* request, IPropertyObject** credentials) = 0;

    /*!
     * @brief Accepts a credential already known in advance - e.g. supplied directly via `IAuthenticationConfig`'s
     * `"SuppliedCredential"` property - so an implementation that caches values it obtains interactively caches
     * this one the same way. A later interactive `requestCredentials` call for the same context then reuses
     * it instead of prompting again. Does not itself produce a credential - the caller already has
     * it and uses it directly. Implementations for which caching doesn't apply may treat this as a
     * no-op.
     * @param request The credential request the credential is being supplied for.
     * @param credential The credential, shaped like the request's authentication method's
     * `createEmptyCredential` template - a property object filled in with the actual value(s).
     */
    virtual ErrCode INTERFACE_FUNC cacheCredentials(ICredentialRequest* request, IPropertyObject* credential) = 0;

    // [elementType(formats, IInteger)]
    /*!
     * @brief Gets a list of the credential formats this provider can provide. Used for
     * format-matching against a device / streaming type's supported formats.
     * @param[out] formats The list of supported formats.
     */
    virtual ErrCode INTERFACE_FUNC getSupportedFormats(IList** formats) = 0;
};

/*!
 * @brief Creates a `ICredentialProvider` that prompts the user for credentials via the command line - supporting
 * every `CredentialFormat`, including `FilePath` (validated locally - retried if the entered path isn't
 * accessible - and cached in-memory per `(manufacturer, serialNumber)` for the active session).
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, CmdLineCredentialProvider, ICredentialProvider)

END_NAMESPACE_OPENDAQ
