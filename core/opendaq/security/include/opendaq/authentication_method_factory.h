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
#include <opendaq/authentication_method_ptr.h>
#include <coretypes/dictobject_factory.h>
#include <coretypes/boolean_factory.h>
#include <coretypes/struct_type_factory.h>
#include <coretypes/simple_type_factory.h>
#include <coretypes/listobject_factory.h>
#include <coretypes/ctutils.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @brief The `IStructType` backing a `KeyValuePairs`-format authentication method's nested `"Parameters"`
 * field - a single `"Keys"` dict field. Never registered with any `ITypeManager` - built fresh wherever
 * needed, purely as a local value shape for `AuthenticationMethodImpl`'s own `getParameters()`.
 */
inline StructTypePtr KeyValueAuthenticationMethodParametersStructType()
{
    return StructType("KeyValueAuthenticationMethodParameters", List<IString>("Keys"), List<IType>(SimpleType(ctDict)));
}

/*!
 * @brief The `IStructType` backing a `String`-format authentication method's nested `"Parameters"` field - a
 * single `"Hidden"` bool field. Never registered with any `ITypeManager` (see
 * `KeyValueAuthenticationMethodParametersStructType`).
 */
inline StructTypePtr StringAuthenticationMethodParametersStructType()
{
    return StructType("StringAuthenticationMethodParameters", List<IString>("Hidden"), List<IType>(SimpleType(ctBool)));
}

/*!
 * @brief Creates a `AuthenticationMethod` describing a `KeyValuePairs`-format credential.
 * @param id The id that uniquely identifies this authentication method at least within the module that offers it.
 * @param keys The expected keys, mapped to whether the corresponding value should be hidden as it is
 * entered (e.g. `{"UserName": False, "Password": True}`) - also names the properties `createEmptyCredential()`
 * builds, one per key.
 * @param description A human-readable description of the authentication method, for the user.
 */
inline AuthenticationMethodPtr KeyValueAuthenticationMethod(const StringPtr& id, const DictPtr<IString, IBoolean>& keys, const StringPtr& description)
{
    AuthenticationMethodPtr obj(KeyValueAuthenticationMethod_Create(id, keys, description));
    return obj;
}

/*!
 * @brief Creates a `AuthenticationMethod` describing a `String`-format credential - a single value,
 * e.g. a PIN, token, or API key.
 * @param id The id that uniquely identifies this authentication method at least within the module that offers it.
 * @param description A human-readable description of the authentication method, for the user.
 * @param hidden Whether the value should be hidden as it is entered.
 * @param valuePropertyName The name `createEmptyCredential()` gives the single property it builds (e.g. `"Pin"`).
 */
inline AuthenticationMethodPtr StringAuthenticationMethod(const StringPtr& id, const StringPtr& description, Bool hidden, const StringPtr& valuePropertyName)
{
    AuthenticationMethodPtr obj(StringAuthenticationMethod_Create(id, description, hidden, valuePropertyName));
    return obj;
}

/*!
 * @brief Creates a `AuthenticationMethod` describing a `FilePath`-format credential - a single value
 * stating that the credential is a path to a file (e.g. a private key) rather than the value itself.
 * @param id The id that uniquely identifies this authentication method at least within the module that offers it.
 * @param description A human-readable description of the authentication method, for the user.
 * @param valuePropertyName The name `createEmptyCredential()` gives the single property it builds (e.g. `"PrivateKeyFilePath"`).
 */
inline AuthenticationMethodPtr FilePathAuthenticationMethod(const StringPtr& id, const StringPtr& description, const StringPtr& valuePropertyName)
{
    AuthenticationMethodPtr obj(FilePathAuthenticationMethod_Create(id, description, valuePropertyName));
    return obj;
}

/*!
 * @brief Creates a `AuthenticationMethod` describing a `None`-format method - no credential value(s) at all,
 * for an authentication method that requires no credentials, e.g. typically an anonymous access. There is no
 * credential to build - `createEmptyCredential()` is not supported for it, and returns `OPENDAQ_ERR_NOT_SUPPORTED`.
 * @param id The id that uniquely identifies this authentication method at least within the module that offers it.
 * @param description A human-readable description of the authentication method, for the user.
 */
inline AuthenticationMethodPtr NoneAuthenticationMethod(const StringPtr& id, const StringPtr& description)
{
    AuthenticationMethodPtr obj(NoneAuthenticationMethod_Create(id, description));
    return obj;
}

/*!
 * @brief Ids of the four standard authentication methods below - shared, well-known ids every
 * module can build the exact same authentication method for, instead of each one inventing its own shape/id for the
 * same method.
 */
inline constexpr const char* StandardUserNamePasswordId = "UserNamePassword";
inline constexpr const char* StandardPinId = "Pin";
inline constexpr const char* StandardPrivateKeyFileId = "PrivateKeyFile";
inline constexpr const char* StandardAnonymousId = "Anonymous";

/*!
 * @brief The authentication method for the standard `UserName`/`Password` authentication method - a
 * `KeyValuePairs`-format credential with the password hidden as typed.
 */
inline AuthenticationMethodPtr StandardUserNamePasswordAuthenticationMethod()
{
    return KeyValueAuthenticationMethod(
        StandardUserNamePasswordId, Dict<IString, IBoolean>({{"UserName", False}, {"Password", True}}), "Username and password");
}

/*!
 * @brief The authentication method for the standard PIN authentication method - a `String`-format
 * credential, hidden as typed.
 */
inline AuthenticationMethodPtr StandardPinAuthenticationMethod()
{
    return StringAuthenticationMethod(StandardPinId, "PIN code", True, "Pin");
}

/*!
 * @brief The authentication method for the standard private-key-file authentication method - a
 * `FilePath`-format credential.
 */
inline AuthenticationMethodPtr StandardPrivateKeyFileAuthenticationMethod()
{
    return FilePathAuthenticationMethod(StandardPrivateKeyFileId, "Path to the PEM-encoded private key file", "PrivateKeyFilePath");
}

/*!
 * @brief The authentication method for the standard anonymous authentication method - a `None`-format
 * method, requiring no credentials at all.
 */
inline AuthenticationMethodPtr StandardAnonymousAuthenticationMethod()
{
    return NoneAuthenticationMethod(StandardAnonymousId, "No credentials required");
}

inline DictPtr<IString, IAuthenticationMethod> AnonymousOnlySupportedAuthenticationMethods()
{
    const auto anonymous = StandardAnonymousAuthenticationMethod();
    return Dict<IString, IAuthenticationMethod>({{anonymous.getId(), anonymous}});
}

END_NAMESPACE_OPENDAQ
