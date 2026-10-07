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
#include <coreobjects/property_object_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @brief The shape of the credential value(s) an authentication method describes.
 */
enum class CredentialFormat : EnumType
{
    None = 0,       ///< No credential value(s) at all - typically for anonymous access, nothing to supply or verify.
    KeyValuePairs,  ///< N string pairs - e.g. UserName / Password.
    String,         ///< one string - token, API key, PIN.
    FilePath        ///< one string - path to a file containing the credential value, e.g. a private key.
};

/*#
 * [interfaceLibrary(IStruct, CoreTypes)]
 * [interfaceLibrary(IPropertyObject, "coreobjects")]
 * [interfaceSmartPtr(IPropertyObject, PropertyObjectPtr, "<coreobjects/property_object_ptr.h>")]
 */

/*!
 * @brief Describes the shape of the credential value(s) required by an authentication method used by the
 * module and produced by a credential provider.
 *
 * An authentication method carries its own id, format, format-specific parameter set (if any), and a
 * human-readable description. The id uniquely identifies the authentication method at least within the module that
 * offers it (e.g. `"UserNamePassword"`, `"Pin"`) - the same id `AuthenticationConfig` keys the resulting
 * config's `"AuthenticationMethod"` candidates by. In practice the id is often unique system-wide, deliberately
 * reused across modules: the `Standard*AuthenticationMethod` factories below key off shared, well-known ids,
 * so any two modules using the same standard id produce identically-shaped authentication methods. Not
 * registered with any `ITypeManager` - a plain object, not a Struct. Where a format has a parameter set,
 * it is itself a Struct: for a `KeyValuePairs` format, a `"Keys"` dict field maps each expected key
 * to its own hidden flag (e.g. `{"UserName": False, "Password": True}`); for a `String` format, a
 * single `"Hidden"` bool field applies to the one value. A `FilePath` format has no format-specific
 * parameters. A `None` format requires no credential value(s) at all - for an authentication method that
 * needs no credentials, e.g. anonymous access - and has no parameters.
 */
DECLARE_OPENDAQ_INTERFACE(IAuthenticationMethod, IBaseObject)
{
    /*!
     * @brief Gets the id that uniquely identifies this authentication method, at least within the module
     * that offers it.
     * @param[out] id The authentication method id.
     */
    virtual ErrCode INTERFACE_FUNC getId(IString** id) = 0;

    /*!
     * @brief Gets the format of the described credential value(s).
     * @param[out] format The credential format.
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
     * @brief Builds an empty credential matching this authentication method's shape - a property object with
     * one empty (default `""`) String property per value the format expects: for `KeyValuePairs`, one
     * property per key named in `getParameters()`'s `"Keys"` dict (e.g. `"UserName"`, `"Password"`); for
     * `String` and `FilePath`, a single property, named per the method's own `valuePropertyName` (e.g.
     * `"Pin"`, `"PrivateKeyFilePath"`).
     *
     * Meant to be filled in with the actual value(s) and used as the credential itself -
     * either by the caller, to supply one directly (`IAuthenticationConfig`'s `"SuppliedCredential"`
     * property), or by a credential provider, once it has obtained it interactively.
     *
     * Not supported for `None` - a `None`-format authentication method requires no credentials at all, so
     * none is ever needed for it in the first place; therefore returns `OPENDAQ_ERR_NOT_SUPPORTED`.
     * @param[out] credential The empty credential.
     */
    virtual ErrCode INTERFACE_FUNC createEmptyCredential(IPropertyObject** credential) = 0;
};

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, KeyValueAuthenticationMethod, IAuthenticationMethod,
    IString*, id, IDict*, keys, IString*, description
)

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, StringAuthenticationMethod, IAuthenticationMethod,
    IString*, id, IString*, description, Bool, hidden, IString*, valuePropertyName
)

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, FilePathAuthenticationMethod, IAuthenticationMethod,
    IString*, id, IString*, description, IString*, valuePropertyName
)

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, NoneAuthenticationMethod, IAuthenticationMethod,
    IString*, id, IString*, description
)

END_NAMESPACE_OPENDAQ
