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
#include <coretypes/struct.h>
#include <coretypes/type_manager.h>
#include <coreobjects/property_object_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @brief The shape of the secret(s) carried by a credential payload.
 */
enum class CredentialPayloadFormat : EnumType
{
    None = 0,       ///< No secret(s) at all - anonymous access, nothing to supply or verify.
    KeyValuePairs,  ///< N string pairs - e.g. UserName / Password.
    String,         ///< one string - token, API key, PIN.
    FilePath        ///< one string - path to a file containing the secret, e.g. a private key.
};

/*#
 * [interfaceLibrary(IStruct, CoreTypes)]
 * [interfaceLibrary(ITypeManager, "coretypes")]
 * [interfaceLibrary(IPropertyObject, "coreobjects")]
 * [interfaceSmartPtr(IPropertyObject, PropertyObjectPtr, "<coreobjects/property_object_ptr.h>")]
 */

/*!
 * @brief Describes the details of the payload required for an authentication method used by the module and produced by credential provider.
 *
 * A descriptor carries the payload's id, format, its format-specific parameter set (if any), and a
 * human-readable description. The id uniquely identifies the authentication method within the module that
 * offers it (e.g. `"UserNamePassword"`, `"Pin"`) - the same id the module's
 * `IModule::getSupportedAuthenticationMethods` uses to key the method. Where a format has a parameter set,
 * it is itself a Struct: for a `KeyValuePairs`-format payload, a `"Keys"` dict field maps each expected key
 * to its own hidden flag (e.g. `{"UserName": False, "Password": True}`); for a `String`-format payload, a
 * single `"Hidden"` bool field applies to the one secret. A `FilePath`-format payload has no format-specific
 * parameters. A `None`-format payload requires no secret(s) at all - for an authentication method that
 * needs no credentials, e.g. anonymous access - and likewise has no parameters.
 */
DECLARE_OPENDAQ_INTERFACE(ICredentialPayloadDescriptor, IBaseObject)
{
    /*!
     * @brief Gets the id that uniquely identifies the authentication method this payload belongs to,
     * within the module that offers it.
     * @param[out] id The payload id.
     */
    virtual ErrCode INTERFACE_FUNC getId(IString** id) = 0;

    /*!
     * @brief Gets the format of the described payload.
     * @param[out] format The payload format.
     */
    virtual ErrCode INTERFACE_FUNC getFormat(CredentialPayloadFormat* format) = 0;

    /*!
     * @brief Gets the format's standard parameter set, as a Struct - see the class description above for
     * which formats have one and what it carries.
     * @param[out] parameters The parameters, or an unassigned `IStruct` if the format has none.
     */
    virtual ErrCode INTERFACE_FUNC getParameters(IStruct** parameters) = 0;

    /*!
     * @brief Gets the description of the payload, for the user. States how the module interpretes it,
     * e.g. "PIN-code", "username and password", "Raw bytes of the SSH private key", "Path to file containing the SSH private key".
     * @param[out] description The payload description.
     */
    virtual ErrCode INTERFACE_FUNC getDescription(IString** description) = 0;

    /*!
     * @brief Builds an empty payload template matching this descriptor's shape - a property object with
     * one empty (default `""`) String property per secret the format expects: for `KeyValuePairs`, one
     * property per key named in `getParameters()`'s `"Keys"` dict (e.g. `"UserName"`, `"Password"`); for
     * `String` and `FilePath`, a single property, named and described by the descriptor's own registered
     * payload class.
     *
     * Meant to be filled in with the actual secret value(s) and used as the credential payload itself -
     * either by the caller, to supply a secret directly (`IAuthenticationConfig`'s `"SuppliedSecret"`
     * property), or by a credential provider, once it has obtained the secret(s) interactively.
     *
     * Not supported for `None` - a `None`-format authentication method requires no credentials at all, so
     * no payload is ever needed for it in the first place. Returns `OPENDAQ_ERR_NOT_SUPPORTED`.
     * @param[out] payload The empty payload template.
     */
    virtual ErrCode INTERFACE_FUNC createDefaultPayload(IPropertyObject** payload) = 0;
};

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, KeyValuePayloadDescriptor, ICredentialPayloadDescriptor,
    IString*, id, IDict*, keys, IString*, description, ITypeManager*, typeManager, IString*, payloadClassName
)

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, StringPayloadDescriptor, ICredentialPayloadDescriptor,
    IString*, id, IString*, description, Bool, hidden, ITypeManager*, typeManager, IString*, payloadClassName
)

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, FilePathPayloadDescriptor, ICredentialPayloadDescriptor,
    IString*, id, IString*, description, ITypeManager*, typeManager, IString*, payloadClassName
)

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, NonePayloadDescriptor, ICredentialPayloadDescriptor,
    IString*, id, IString*, description, ITypeManager*, typeManager
)

END_NAMESPACE_OPENDAQ
