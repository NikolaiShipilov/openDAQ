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
#include <opendaq/credential_descriptor_ptr.h>
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
 * @brief Name of the `IPropertyObjectClass` backing the empty secret `createEmptySecret()` builds for
 * `StandardUserNamePasswordCredentialDescriptor`.
 */
inline constexpr const char* UserNamePasswordCredentialSecretClassName = "UserNamePasswordCredentialSecret";

inline PropertyObjectClassPtr UserNamePasswordCredentialSecretClass()
{
    return PropertyObjectClassBuilder(UserNamePasswordCredentialSecretClassName)
        .addProperty(StringPropertyBuilder("UserName", "").setDescription("The username.").build())
        .addProperty(StringPropertyBuilder("Password", "").setDescription("The password.").build())
        .build();
}

/*!
 * @brief Name of the `IPropertyObjectClass` backing the empty secret `createEmptySecret()` builds for
 * `StandardPinCredentialDescriptor`.
 */
inline constexpr const char* PinCredentialSecretClassName = "PinCredentialSecret";

inline PropertyObjectClassPtr PinCredentialSecretClass()
{
    return PropertyObjectClassBuilder(PinCredentialSecretClassName)
        .addProperty(StringPropertyBuilder("Pin", "").setDescription("The PIN code.").build())
        .build();
}

/*!
 * @brief Name of the `IPropertyObjectClass` backing the empty secret `createEmptySecret()` builds for
 * `StandardPrivateKeyFileCredentialDescriptor`.
 */
inline constexpr const char* PrivateKeyFileCredentialSecretClassName = "PrivateKeyFileCredentialSecret";

inline PropertyObjectClassPtr PrivateKeyFileCredentialSecretClass()
{
    return PropertyObjectClassBuilder(PrivateKeyFileCredentialSecretClassName)
        .addProperty(StringPropertyBuilder("PrivateKeyFilePath", "").setDescription("Path to the PEM-encoded private key file.").build())
        .build();
}

/*!
 * @brief The `IStructType` backing a `KeyValuePairs`-format descriptor's nested `"Parameters"` field - a
 * single `"Keys"` dict field. The single source of truth for this shape - `CredentialDescriptorImpl`
 * and `Context` (see `RegisterCredentialDescriptorTypes`) both build it from here, never redefine it.
 */
inline StructTypePtr KeyValueDescriptorParametersStructType()
{
    return StructType("KeyValueDescriptorParameters", List<IString>("Keys"), List<IType>(SimpleType(ctDict)));
}

/*!
 * @brief The `IStructType` backing a `String`-format descriptor's nested `"Parameters"` field - a single
 * `"Hidden"` bool field.
 */
inline StructTypePtr StringDescriptorParametersStructType()
{
    return StructType("StringDescriptorParameters", List<IString>("Hidden"), List<IType>(SimpleType(ctBool)));
}

/*!
 * @brief The `IStructType` backing a `KeyValuePairs`-format `CredentialDescriptor`.
 */
inline StructTypePtr KeyValueDescriptorStructType()
{
    return StructType("KeyValueDescriptor",
                      List<IString>("AuthenticationMethodId", "Description", "Parameters"),
                      List<IType>(SimpleType(ctString), SimpleType(ctString), KeyValueDescriptorParametersStructType()));
}

/*!
 * @brief The `IStructType` backing a `String`-format `CredentialDescriptor`.
 */
inline StructTypePtr StringDescriptorStructType()
{
    return StructType("StringDescriptor",
                      List<IString>("AuthenticationMethodId", "Description", "Parameters"),
                      List<IType>(SimpleType(ctString), SimpleType(ctString), StringDescriptorParametersStructType()));
}

/*!
 * @brief The `IStructType` backing a `FilePath`-format `CredentialDescriptor` - has no `"Parameters"`
 * field, since the format has no format-specific parameters.
 */
inline StructTypePtr FilePathDescriptorStructType()
{
    return StructType("FilePathDescriptor",
                      List<IString>("AuthenticationMethodId", "Description"),
                      List<IType>(SimpleType(ctString), SimpleType(ctString)));
}

/*!
 * @brief The `IStructType` backing a `None`-format `CredentialDescriptor` - has no `"Parameters"`
 * field, since the format has no format-specific parameters.
 */
inline StructTypePtr NoneDescriptorStructType()
{
    return StructType("NoneDescriptor",
                      List<IString>("AuthenticationMethodId", "Description"),
                      List<IType>(SimpleType(ctString), SimpleType(ctString)));
}

/*!
 * @brief Registers the `KeyValuePairs`/`String`/`FilePath` formats' backing `IStructType`s, plus the
 * standard descriptors' own empty-secret `IPropertyObjectClass`es, with `typeManager`. Called once by
 * `Context` up front. The `None` format's `IStructType` is never registered with any type manager - see
 * `NoneDescriptor`.
 * @param typeManager The type manager to register the credential descriptor types with.
 */
inline void RegisterCredentialDescriptorTypes(const TypeManagerPtr& typeManager)
{
    for (const auto& type : {KeyValueDescriptorStructType(),
                             KeyValueDescriptorParametersStructType(),
                             StringDescriptorStructType(),
                             StringDescriptorParametersStructType(),
                             FilePathDescriptorStructType()})
    {
        checkErrorInfoExcept(typeManager->addType(type), OPENDAQ_ERR_ALREADYEXISTS);
    }

    for (const auto& secretClass : {UserNamePasswordCredentialSecretClass(),
                                    PinCredentialSecretClass(),
                                    PrivateKeyFileCredentialSecretClass()})
    {
        checkErrorInfoExcept(typeManager->addType(secretClass), OPENDAQ_ERR_ALREADYEXISTS);
    }
}

/*!
 * @brief Creates a `CredentialDescriptor` describing a `KeyValuePairs`-format secret.
 * @param id The id that uniquely identifies this authentication method at least within the module that offers it.
 * @param keys The expected keys, mapped to whether the corresponding value should be hidden as it is
 * entered (e.g. `{"UserName": False, "Password": True}`).
 * @param description A human-readable description of the authentication method, for the user.
 * @param typeManager Must already have a `"KeyValueDescriptor"` type registered (see
 * `RegisterCredentialDescriptorTypes`) - a real `Context` always registers it up front. Throws
 * otherwise.
 * @param secretClassName Must be assigned and registered with `typeManager` - `createEmptySecret()`
 * builds the returned secret from this `IPropertyObjectClass`. Throws otherwise.
 */
inline CredentialDescriptorPtr KeyValueDescriptor(const StringPtr& id,
                                                   const DictPtr<IString, IBoolean>& keys,
                                                   const StringPtr& description,
                                                   const TypeManagerPtr& typeManager,
                                                   const StringPtr& secretClassName)
{
    CredentialDescriptorPtr obj(KeyValueDescriptor_Create(id, keys, description, typeManager, secretClassName));
    return obj;
}

/*!
 * @brief Creates a `CredentialDescriptor` describing a `String`-format secret - a single value,
 * e.g. a PIN, token, or API key.
 * @param id The id that uniquely identifies this authentication method at least within the module that offers it.
 * @param description A human-readable description of the authentication method, for the user.
 * @param hidden Whether the secret should be hidden as it is entered.
 * @param typeManager Must already have a `"StringDescriptor"` type registered (see
 * `RegisterCredentialDescriptorTypes`) - a real `Context` always registers it up front. Throws
 * otherwise.
 * @param secretClassName Must be assigned and registered with `typeManager` - `createEmptySecret()`
 * builds the returned secret from this `IPropertyObjectClass`. Throws otherwise.
 */
inline CredentialDescriptorPtr StringDescriptor(const StringPtr& id,
                                                 const StringPtr& description,
                                                 Bool hidden,
                                                 const TypeManagerPtr& typeManager,
                                                 const StringPtr& secretClassName)
{
    CredentialDescriptorPtr obj(StringDescriptor_Create(id, description, hidden, typeManager, secretClassName));
    return obj;
}

/*!
 * @brief Creates a `CredentialDescriptor` describing a `FilePath`-format secret - a single value
 * stating that the secret is a path to a file (e.g. a private key) rather than the value itself.
 * @param id The id that uniquely identifies this authentication method at least within the module that offers it.
 * @param description A human-readable description of the authentication method, for the user.
 * @param typeManager Must already have a `"FilePathDescriptor"` type registered (see
 * `RegisterCredentialDescriptorTypes`) - a real `Context` always registers it up front. Throws
 * otherwise.
 * @param secretClassName Must be assigned and registered with `typeManager` - `createEmptySecret()`
 * builds the returned secret from this `IPropertyObjectClass`. Throws otherwise.
 */
inline CredentialDescriptorPtr FilePathDescriptor(const StringPtr& id,
                                                   const StringPtr& description,
                                                   const TypeManagerPtr& typeManager,
                                                   const StringPtr& secretClassName)
{
    CredentialDescriptorPtr obj(FilePathDescriptor_Create(id, description, typeManager, secretClassName));
    return obj;
}

/*!
 * @brief Creates a `CredentialDescriptor` describing a `None`-format method - no secret(s) at all,
 * for an authentication method that requires no credentials, e.g. typically an anonymous access. Unlike the other
 * formats, there is no secret to build, so no secret class is involved - `createEmptySecret()` is not
 * supported for it, and returns `OPENDAQ_ERR_NOT_SUPPORTED`. Unlike the other formats, its `IStructType` is
 * never registered with any `ITypeManager` - a fixed, well-known shape regardless of which `Context` (if
 * any) is involved - so, unlike the other three factories, this one needs no type manager at all.
 * @param id The id that uniquely identifies this authentication method at least within the module that offers it.
 * @param description A human-readable description of the authentication method, for the user.
 */
inline CredentialDescriptorPtr NoneDescriptor(const StringPtr& id, const StringPtr& description)
{
    CredentialDescriptorPtr obj(NoneDescriptor_Create(id, description));
    return obj;
}

/*!
 * @brief Ids of the four standard authentication methods below - shared, well-known ids every
 * module can build the exact same descriptor for, instead of each one inventing its own shape/id for the
 * same method.
 */
inline constexpr const char* StandardUserNamePasswordId = "UserNamePassword";
inline constexpr const char* StandardPinId = "Pin";
inline constexpr const char* StandardPrivateKeyFileId = "PrivateKeyFile";
inline constexpr const char* StandardAnonymousId = "Anonymous";

/*!
 * @brief The credential descriptor for the standard `UserName`/`Password` authentication method - a
 * `KeyValuePairs`-format secret with the password hidden as typed.
 * @param typeManager See `KeyValueDescriptor`.
 */
inline CredentialDescriptorPtr StandardUserNamePasswordCredentialDescriptor(const TypeManagerPtr& typeManager)
{
    return KeyValueDescriptor(StandardUserNamePasswordId,
                              Dict<IString, IBoolean>({{"UserName", False}, {"Password", True}}),
                              "Username and password",
                              typeManager,
                              UserNamePasswordCredentialSecretClassName);
}

/*!
 * @brief The credential descriptor for the standard PIN authentication method - a `String`-format
 * secret, hidden as typed.
 * @param typeManager See `StringDescriptor`.
 */
inline CredentialDescriptorPtr StandardPinCredentialDescriptor(const TypeManagerPtr& typeManager)
{
    return StringDescriptor(StandardPinId, "PIN code", True, typeManager, PinCredentialSecretClassName);
}

/*!
 * @brief The credential descriptor for the standard private-key-file authentication method - a
 * `FilePath`-format secret.
 * @param typeManager See `FilePathDescriptor`.
 */
inline CredentialDescriptorPtr StandardPrivateKeyFileCredentialDescriptor(const TypeManagerPtr& typeManager)
{
    return FilePathDescriptor(
        StandardPrivateKeyFileId, "Path to the PEM-encoded private key file", typeManager, PrivateKeyFileCredentialSecretClassName);
}

/*!
 * @brief The credential descriptor for the standard anonymous authentication method - a `None`-format
 * method, requiring no credentials at all. Unlike the other three standard descriptors, needs no type
 * manager - see `NoneDescriptor`.
 */
inline CredentialDescriptorPtr StandardAnonymousCredentialDescriptor()
{
    return NoneDescriptor(StandardAnonymousId, "No credentials required");
}

inline DictPtr<IString, ICredentialDescriptor> AnonymousOnlySupportedAuthenticationMethods()
{
    const auto anonymous = StandardAnonymousCredentialDescriptor();
    return Dict<IString, ICredentialDescriptor>({{anonymous.getAuthenticationMethodId(), anonymous}});
}

END_NAMESPACE_OPENDAQ
