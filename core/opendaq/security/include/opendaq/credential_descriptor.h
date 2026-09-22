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
 * @brief The shape of the secret(s) a credential descriptor describes.
 */
enum class CredentialFormat : EnumType
{
    None = 0,       ///< No secret(s) at all - typically for anonymous access, nothing to supply or verify.
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
 * @brief Describes the shape of the secret(s) required by an authentication method used by the module and
 * produced by a credential provider.
 *
 * A descriptor carries the method's id, format, its format-specific parameter set (if any), and a
 * human-readable description. The id uniquely identifies the authentication method at least within the module that
 * offers it (e.g. `"UserNamePassword"`, `"Pin"`) - the same id the module's
 * `IModule::createDefaultAuthenticationConfig` keys the resulting config's `"AuthenticationMethod"` candidates
 * by. In practice the id is often unique system-wide, deliberately reused across modules: the `Standard*CredentialDescriptor`
 * factories below key off shared, well-known ids and resolve their Struct/secret class from the one
 * `ITypeManager` shared by the whole `Context`, so any two modules using the same standard id (with the same
 * `Context`) produce identically-shaped descriptors. Where a format has a parameter set,
 * it is itself a Struct: for a `KeyValuePairs` format, a `"Keys"` dict field maps each expected key
 * to its own hidden flag (e.g. `{"UserName": False, "Password": True}`); for a `String` format, a
 * single `"Hidden"` bool field applies to the one secret. A `FilePath` format has no format-specific
 * parameters. A `None` format requires no secret(s) at all - for an authentication method that
 * needs no credentials, e.g. anonymous access - and has no parameters.
 */
DECLARE_OPENDAQ_INTERFACE(ICredentialDescriptor, IBaseObject)
{
    /*!
     * @brief Gets the id that uniquely identifies the authentication method this descriptor belongs to,
     * at least within the module that offers it.
     * @param[out] authenticationMethodId The authentication method id.
     */
    virtual ErrCode INTERFACE_FUNC getAuthenticationMethodId(IString** authenticationMethodId) = 0;

    /*!
     * @brief Gets the format of the described secret(s).
     * @param[out] format The secret format.
     */
    virtual ErrCode INTERFACE_FUNC getFormat(CredentialFormat* format) = 0;

    /*!
     * @brief Gets the format's standard parameter set, as a Struct - see the class description above for
     * which formats have one and what it carries.
     * @param[out] parameters The parameters, or an unassigned `IStruct` if the format has none.
     */
    virtual ErrCode INTERFACE_FUNC getParameters(IStruct** parameters) = 0;

    /*!
     * @brief Gets the description of the authentication method, for the user. States how the module
     * interpretes it, e.g. "PIN-code", "username and password", "Path to file containing the SSH private key".
     * @param[out] description The method description.
     */
    virtual ErrCode INTERFACE_FUNC getDescription(IString** description) = 0;

    /*!
     * @brief Builds an empty secret matching this descriptor's shape - a property object with
     * one empty (default `""`) String property per secret value the format expects: for `KeyValuePairs`, one
     * property per key named in `getParameters()`'s `"Keys"` dict (e.g. `"UserName"`, `"Password"`); for
     * `String` and `FilePath`, a single property, named and described by the descriptor's own registered
     * secret class.
     *
     * Meant to be filled in with the actual secret value(s) and used as the credential itself -
     * either by the caller, to supply a secret directly (`IAuthenticationConfig`'s `"SuppliedSecret"`
     * property), or by a credential provider, once it has obtained the secret(s) interactively.
     *
     * Not supported for `None` - a `None`-format authentication method requires no credentials at all, so
     * no secret is ever needed for it in the first place; therefore returns `OPENDAQ_ERR_NOT_SUPPORTED`.
     * @param[out] secret The empty secret.
     */
    virtual ErrCode INTERFACE_FUNC createEmptySecret(IPropertyObject** secret) = 0;
};

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, KeyValueDescriptor, ICredentialDescriptor,
    IString*, id, IDict*, keys, IString*, description, ITypeManager*, typeManager, IString*, secretClassName
)

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, StringDescriptor, ICredentialDescriptor,
    IString*, id, IString*, description, Bool, hidden, ITypeManager*, typeManager, IString*, secretClassName
)

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, FilePathDescriptor, ICredentialDescriptor,
    IString*, id, IString*, description, ITypeManager*, typeManager, IString*, secretClassName
)

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, NoneDescriptor, ICredentialDescriptor,
    IString*, id, IString*, description
)

END_NAMESPACE_OPENDAQ
