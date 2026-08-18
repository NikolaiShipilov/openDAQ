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
#include <coretypes/function.h>

BEGIN_NAMESPACE_OPENDAQ

DECLARE_OPENDAQ_INTERFACE(ICredentialPayload, IBaseObject)
{
    /*!
     * @brief Gets the secret(s) carried by the payload. The concrete type depends on the payload format
     * the payload was obtained for - `IString` for a `String`- or `FilePath`-format payload,
     * `IDict<IString, IString>` for a `KeyValuePairs`-format payload, `IBinaryData` for a `BinaryBlob`-format
     * payload (its raw bytes and size obtained via `IBinaryData::getAddress`/`getSize`). Callers are expected
     * to know the format (e.g. from the IAuthenticationConfig and ICredentialPayloadDescriptor used to request
     * the credential) and cast accordingly.
     * @param[out] secrets The secret(s) carried by the payload.
     */
    virtual ErrCode INTERFACE_FUNC getSecrets(IBaseObject** secrets) = 0;
};

/*!
 * @brief Creates a `KeyValuePairs`-format `ICredentialPayload`. Its secrets are returned by `getSecrets`
 * as an `IDict<IString, IString>`, keyed the same as the `"Keys"` parameter of the `ICredentialPayloadDescriptor`
 * the payload was obtained for.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, KeyValueCredentialPayload, ICredentialPayload, IFunction*, getValuesCb)

/*!
 * @brief Creates a `String`-format `ICredentialPayload`. Its single secret is returned directly by
 * `getSecrets`, as an `IString`.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, StringCredentialPayload, ICredentialPayload, IFunction*, getSecretCb)

/*!
 * @brief Creates a `BinaryBlob`-format `ICredentialPayload`. Its single secret is returned by `getSecrets`
 * as an `IBinaryData`, giving the raw bytes and size of the blob via `getAddress`/`getSize`.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, BinaryBlobCredentialPayload, ICredentialPayload, IFunction*, getBlobCb)

END_NAMESPACE_OPENDAQ
