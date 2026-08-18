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
#include <opendaq/credential_request.h>
#include <opendaq/credential_payload.h>
#include <opendaq/credential_payload_descriptor.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [interfaceLibrary(IInteger, "coretypes")]
 * [interfaceSmartPtr(IInteger, IntegerPtr, "<coretypes/integer.h>")]
 */

/*!
 * @brief Supplies the secrets requested via a `ICredentialRequest` - e.g. by prompting the user, reading
 * from a file, or fetching from a secret store.
 */
DECLARE_OPENDAQ_INTERFACE(ICredentialProvider, IBaseObject)
{
    /*!
     * @brief Gets the name of the credential provider.
     * @param[out] name The provider name.
     */
    virtual ErrCode INTERFACE_FUNC getName(IString** name) = 0;

    /*!
     * @brief Requests credentials for the given request, in the format described by its payload descriptor.
     * @param request The credential request to obtain credentials for.
     * @param[out] credentials The obtained credential payload.
     */
    virtual ErrCode INTERFACE_FUNC requestCredentials(ICredentialRequest* request, ICredentialPayload** credentials) = 0;

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
 * @brief Creates a `ICredentialProvider` dedicated to `FilePath`-format payloads. Currently prompts for
 * the file's path via the command line, the same way `CmdLineCredentialProvider` does for that format;
 * kept as its own provider so it can diverge from `CmdLineCredentialProvider` later - e.g. to hand back
 * the file's content directly instead of just its path.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, FileCredentialProvider, ICredentialProvider)

END_NAMESPACE_OPENDAQ
