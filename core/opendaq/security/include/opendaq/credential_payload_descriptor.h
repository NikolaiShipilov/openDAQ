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
#include <coretypes/string_ptr.h>
#include <coreobjects/property_object_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @brief The shape of the secret(s) carried by a credential payload.
 */
enum class CredentialPayloadFormat : EnumType
{
    KeyValuePairs,  ///< N string pairs - e.g. UserName / Password.
    String,         ///< one string - token, API key, PIN.
    FilePath,       ///< one string - path to a file containing the secret, e.g. a private key.
    BinaryBlob      ///< one raw byte buffer - pointer + size
};

/*#
 * [interfaceLibrary(IPropertyObject, "coreobjects")]
 * [interfaceSmartPtr(IPropertyObject, PropertyObjectPtr, "<coreobjects/property_object_ptr.h>")]
 */

/*!
 * @brief Describes the details of the payload required for an authentication method used by the module and produced by credential provider.
 *
 * A descriptor carries the payload's format, its format-specific parameter set, and a human-readable
 * description. In particular, the parameter set carries whether the payload's value(s) should be hidden as
 * they are entered: for a `KeyValuePairs`-format payload, a `"Keys"` dict property maps each expected key to
 * its own hidden flag (e.g. `{"UserName": False, "Password": True}`); for a `String`-format payload, a
 * single `"Hidden"` bool property applies to the one secret. `FilePath`- and `BinaryBlob`-format payloads
 * have no parameters.
 */
DECLARE_OPENDAQ_INTERFACE(ICredentialPayloadDescriptor, IBaseObject)
{
    /*!
     * @brief Gets the format of the described payload.
     * @param[out] format The payload format.
     */
    virtual ErrCode INTERFACE_FUNC getFormat(CredentialPayloadFormat* format) = 0;

    /*!
     * @brief Gets the format's standard parameter set.
     * @param[out] parameters The parameters.
     */
    virtual ErrCode INTERFACE_FUNC getParameters(IPropertyObject** parameters) = 0;

    /*!
     * @brief Gets the description of the payload, for the user. States how the module interpretes it,
     * e.g. "PIN-code", "username and password", "Raw bytes of the SSH private key", "Path to file containing the SSH private key".
     * @param[out] description The payload description.
     */
    virtual ErrCode INTERFACE_FUNC getDescription(IString** description) = 0;
};

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, KeyValuePayloadDescriptor, ICredentialPayloadDescriptor,
    IDict*, keys, IString*, description
)

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, StringPayloadDescriptor, ICredentialPayloadDescriptor,
    IString*, description, Bool, hidden
)

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, FilePathPayloadDescriptor, ICredentialPayloadDescriptor,
    IString*, description
)

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, BinaryBlobPayloadDescriptor, ICredentialPayloadDescriptor,
    IString*, description
)

END_NAMESPACE_OPENDAQ
