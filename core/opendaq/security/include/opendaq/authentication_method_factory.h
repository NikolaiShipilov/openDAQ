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
#include <coretypes/type_manager_ptr.h>
#include <coretypes/struct_type_factory.h>
#include <coretypes/simple_type_factory.h>
#include <coretypes/listobject_factory.h>
#include <coretypes/ctutils.h>
#include <coreobjects/property_object_class_factory.h>
#include <coreobjects/property_factory.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @brief Name of the `IPropertyObjectClass` backing the empty credential `createEmptyCredential()` builds for
 * `StandardUserNamePasswordAuthenticationMethod`.
 */
inline constexpr const char* UserNamePasswordCredentialClassName = "UserNamePasswordCredential";

inline PropertyObjectClassPtr UserNamePasswordCredentialClass()
{
    return PropertyObjectClassBuilder(UserNamePasswordCredentialClassName)
        .addProperty(StringPropertyBuilder("UserName", "").setDescription("The username.").build())
        .addProperty(StringPropertyBuilder("Password", "").setDescription("The password.").build())
        .build();
}

/*!
 * @brief Name of the `IPropertyObjectClass` backing the empty credential `createEmptyCredential()` builds for
 * `StandardPinAuthenticationMethod`.
 */
inline constexpr const char* PinCredentialClassName = "PinCredential";

inline PropertyObjectClassPtr PinCredentialClass()
{
    return PropertyObjectClassBuilder(PinCredentialClassName)
        .addProperty(StringPropertyBuilder("Pin", "").setDescription("The PIN code.").build())
        .build();
}

/*!
 * @brief Name of the `IPropertyObjectClass` backing the empty credential `createEmptyCredential()` builds for
 * `StandardPrivateKeyFileAuthenticationMethod`.
 */
inline constexpr const char* PrivateKeyFileCredentialClassName = "PrivateKeyFileCredential";

inline PropertyObjectClassPtr PrivateKeyFileCredentialClass()
{
    return PropertyObjectClassBuilder(PrivateKeyFileCredentialClassName)
        .addProperty(StringPropertyBuilder("PrivateKeyFilePath", "").setDescription("Path to the PEM-encoded private key file.").build())
        .build();
}

/*!
 * @brief The `IStructType` backing a `KeyValuePairs`-format authentication method's nested `"Parameters"` field - a
 * single `"Keys"` dict field. The single source of truth for this shape - `AuthenticationMethodImpl`
 * and `Context` (see `RegisterAuthenticationMethodTypes`) both build it from here, never redefine it.
 */
inline StructTypePtr KeyValueAuthenticationMethodParametersStructType()
{
    return StructType("KeyValueAuthenticationMethodParameters", List<IString>("Keys"), List<IType>(SimpleType(ctDict)));
}

/*!
 * @brief The `IStructType` backing a `String`-format authentication method's nested `"Parameters"` field - a single
 * `"Hidden"` bool field.
 */
inline StructTypePtr StringAuthenticationMethodParametersStructType()
{
    return StructType("StringAuthenticationMethodParameters", List<IString>("Hidden"), List<IType>(SimpleType(ctBool)));
}

/*!
 * @brief The `IStructType` backing a `KeyValuePairs`-format `AuthenticationMethod`.
 */
inline StructTypePtr KeyValueAuthenticationMethodStructType()
{
    return StructType("KeyValueAuthenticationMethod",
                      List<IString>("AuthenticationMethodId", "Description", "Parameters"),
                      List<IType>(SimpleType(ctString), SimpleType(ctString), KeyValueAuthenticationMethodParametersStructType()));
}

/*!
 * @brief The `IStructType` backing a `String`-format `AuthenticationMethod`.
 */
inline StructTypePtr StringAuthenticationMethodStructType()
{
    return StructType("StringAuthenticationMethod",
                      List<IString>("AuthenticationMethodId", "Description", "Parameters"),
                      List<IType>(SimpleType(ctString), SimpleType(ctString), StringAuthenticationMethodParametersStructType()));
}

/*!
 * @brief The `IStructType` backing a `FilePath`-format `AuthenticationMethod` - has no `"Parameters"`
 * field, since the format has no format-specific parameters.
 */
inline StructTypePtr FilePathAuthenticationMethodStructType()
{
    return StructType("FilePathAuthenticationMethod",
                      List<IString>("AuthenticationMethodId", "Description"),
                      List<IType>(SimpleType(ctString), SimpleType(ctString)));
}

/*!
 * @brief The `IStructType` backing a `None`-format `AuthenticationMethod` - has no `"Parameters"`
 * field, since the format has no format-specific parameters.
 */
inline StructTypePtr NoneAuthenticationMethodStructType()
{
    return StructType("NoneAuthenticationMethod",
                      List<IString>("AuthenticationMethodId", "Description"),
                      List<IType>(SimpleType(ctString), SimpleType(ctString)));
}

/*!
 * @brief Registers the `KeyValuePairs`/`String`/`FilePath` formats' backing `IStructType`s, plus the
 * standard authentication methods' own empty-credential `IPropertyObjectClass`es, with `typeManager`. Called once by
 * `Context` up front. The `None` format's `IStructType` is never registered with any type manager - see
 * `NoneAuthenticationMethod`.
 * @param typeManager The type manager to register the authentication method types with.
 */
inline void RegisterAuthenticationMethodTypes(const TypeManagerPtr& typeManager)
{
    for (const auto& type : {KeyValueAuthenticationMethodStructType(),
                             KeyValueAuthenticationMethodParametersStructType(),
                             StringAuthenticationMethodStructType(),
                             StringAuthenticationMethodParametersStructType(),
                             FilePathAuthenticationMethodStructType()})
    {
        checkErrorInfoExcept(typeManager->addType(type), OPENDAQ_ERR_ALREADYEXISTS);
    }

    for (const auto& credentialClass : {UserNamePasswordCredentialClass(),
                                        PinCredentialClass(),
                                        PrivateKeyFileCredentialClass()})
    {
        checkErrorInfoExcept(typeManager->addType(credentialClass), OPENDAQ_ERR_ALREADYEXISTS);
    }
}

/*!
 * @brief Creates a `AuthenticationMethod` describing a `KeyValuePairs`-format credential.
 * @param id The id that uniquely identifies this authentication method at least within the module that offers it.
 * @param keys The expected keys, mapped to whether the corresponding value should be hidden as it is
 * entered (e.g. `{"UserName": False, "Password": True}`).
 * @param description A human-readable description of the authentication method, for the user.
 * @param typeManager Must already have a `"KeyValueAuthenticationMethod"` type registered (see
 * `RegisterAuthenticationMethodTypes`) - a real `Context` always registers it up front. Throws
 * otherwise.
 * @param credentialClassName Must be assigned and registered with `typeManager` - `createEmptyCredential()`
 * builds the returned credential from this `IPropertyObjectClass`. Throws otherwise.
 */
inline AuthenticationMethodPtr KeyValueAuthenticationMethod(const StringPtr& id,
                                                   const DictPtr<IString, IBoolean>& keys,
                                                   const StringPtr& description,
                                                   const TypeManagerPtr& typeManager,
                                                   const StringPtr& credentialClassName)
{
    AuthenticationMethodPtr obj(KeyValueAuthenticationMethod_Create(id, keys, description, typeManager, credentialClassName));
    return obj;
}

/*!
 * @brief Creates a `AuthenticationMethod` describing a `String`-format credential - a single value,
 * e.g. a PIN, token, or API key.
 * @param id The id that uniquely identifies this authentication method at least within the module that offers it.
 * @param description A human-readable description of the authentication method, for the user.
 * @param hidden Whether the value should be hidden as it is entered.
 * @param typeManager Must already have a `"StringAuthenticationMethod"` type registered (see
 * `RegisterAuthenticationMethodTypes`) - a real `Context` always registers it up front. Throws
 * otherwise.
 * @param credentialClassName Must be assigned and registered with `typeManager` - `createEmptyCredential()`
 * builds the returned credential from this `IPropertyObjectClass`. Throws otherwise.
 */
inline AuthenticationMethodPtr StringAuthenticationMethod(const StringPtr& id,
                                                 const StringPtr& description,
                                                 Bool hidden,
                                                 const TypeManagerPtr& typeManager,
                                                 const StringPtr& credentialClassName)
{
    AuthenticationMethodPtr obj(StringAuthenticationMethod_Create(id, description, hidden, typeManager, credentialClassName));
    return obj;
}

/*!
 * @brief Creates a `AuthenticationMethod` describing a `FilePath`-format credential - a single value
 * stating that the credential is a path to a file (e.g. a private key) rather than the value itself.
 * @param id The id that uniquely identifies this authentication method at least within the module that offers it.
 * @param description A human-readable description of the authentication method, for the user.
 * @param typeManager Must already have a `"FilePathAuthenticationMethod"` type registered (see
 * `RegisterAuthenticationMethodTypes`) - a real `Context` always registers it up front. Throws
 * otherwise.
 * @param credentialClassName Must be assigned and registered with `typeManager` - `createEmptyCredential()`
 * builds the returned credential from this `IPropertyObjectClass`. Throws otherwise.
 */
inline AuthenticationMethodPtr FilePathAuthenticationMethod(const StringPtr& id,
                                                   const StringPtr& description,
                                                   const TypeManagerPtr& typeManager,
                                                   const StringPtr& credentialClassName)
{
    AuthenticationMethodPtr obj(FilePathAuthenticationMethod_Create(id, description, typeManager, credentialClassName));
    return obj;
}

/*!
 * @brief Creates a `AuthenticationMethod` describing a `None`-format method - no credential value(s) at all,
 * for an authentication method that requires no credentials, e.g. typically an anonymous access. Unlike the other
 * formats, there is no credential to build, so no credential class is involved - `createEmptyCredential()` is not
 * supported for it, and returns `OPENDAQ_ERR_NOT_SUPPORTED`. Unlike the other formats, its `IStructType` is
 * never registered with any `ITypeManager` - a fixed, well-known shape regardless of which `Context` (if
 * any) is involved - so, unlike the other three factories, this one needs no type manager at all.
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
 * @param typeManager See `KeyValueAuthenticationMethod`.
 */
inline AuthenticationMethodPtr StandardUserNamePasswordAuthenticationMethod(const TypeManagerPtr& typeManager)
{
    return KeyValueAuthenticationMethod(StandardUserNamePasswordId,
                              Dict<IString, IBoolean>({{"UserName", False}, {"Password", True}}),
                              "Username and password",
                              typeManager,
                              UserNamePasswordCredentialClassName);
}

/*!
 * @brief The authentication method for the standard PIN authentication method - a `String`-format
 * credential, hidden as typed.
 * @param typeManager See `StringAuthenticationMethod`.
 */
inline AuthenticationMethodPtr StandardPinAuthenticationMethod(const TypeManagerPtr& typeManager)
{
    return StringAuthenticationMethod(StandardPinId, "PIN code", True, typeManager, PinCredentialClassName);
}

/*!
 * @brief The authentication method for the standard private-key-file authentication method - a
 * `FilePath`-format credential.
 * @param typeManager See `FilePathAuthenticationMethod`.
 */
inline AuthenticationMethodPtr StandardPrivateKeyFileAuthenticationMethod(const TypeManagerPtr& typeManager)
{
    return FilePathAuthenticationMethod(
        StandardPrivateKeyFileId, "Path to the PEM-encoded private key file", typeManager, PrivateKeyFileCredentialClassName);
}

/*!
 * @brief The authentication method for the standard anonymous authentication method - a `None`-format
 * method, requiring no credentials at all. Unlike the other three standard authentication methods, needs no type
 * manager - see `NoneAuthenticationMethod`.
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
