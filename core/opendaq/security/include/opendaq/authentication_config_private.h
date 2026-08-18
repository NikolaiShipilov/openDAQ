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

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [interfaceLibrary(ICredentialRequest, "opendaq")]
 * [interfaceSmartPtr(ICredentialRequest, CredentialRequestPtr, "<opendaq/credential_request_ptr.h>")]
 */

/*!
 * @brief Provides access to private methods of an authentication config.
 */
DECLARE_OPENDAQ_INTERFACE(IAuthenticationConfigPrivate, IBaseObject)
{
    /*!
     * @brief Gets the previously formed credential request this config was reconstructed from, if any.
     * @param[out] request The previously formed credential request, or `nullptr` for a config built for a
     * live connection attempt.
     *
     * Only assigned when this config was reconstructed while reloading a saved device that had previously
     * been added with authentication. When assigned, a module should use this request as-is - via
     * `ICredentialProvider::requestCredentials` - instead of forming a new one from the payload descriptor and
     * additional config; it already carries the resolved, non-secret shape (payload id/descriptor, connection
     * details, metadata) of what was originally requested from the credential provider.
     */
    virtual ErrCode INTERFACE_FUNC getCredentialRequest(ICredentialRequest** request) = 0;
};

END_NAMESPACE_OPENDAQ
