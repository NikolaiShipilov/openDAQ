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
#include <opendaq/credential_payload_descriptor_ptr.h>
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
 * @brief Name of the `IPropertyObjectClass` backing the default payload `createDefaultPayload()` builds for
 * `StandardUserNamePasswordPayloadDescriptor`.
 */
inline constexpr const char* UserNamePasswordCredentialSecretPayloadClassName = "UserNamePasswordCredentialSecretPayload";

inline PropertyObjectClassPtr UserNamePasswordCredentialSecretPayloadClass()
{
    return PropertyObjectClassBuilder(UserNamePasswordCredentialSecretPayloadClassName)
        .addProperty(StringPropertyBuilder("UserName", "").setDescription("The username.").build())
        .addProperty(StringPropertyBuilder("Password", "").setDescription("The password.").build())
        .build();
}

/*!
 * @brief Name of the `IPropertyObjectClass` backing the default payload `createDefaultPayload()` builds for
 * `StandardPinPayloadDescriptor`.
 */
inline constexpr const char* PinCredentialSecretPayloadClassName = "PinCredentialSecretPayload";

inline PropertyObjectClassPtr PinCredentialSecretPayloadClass()
{
    return PropertyObjectClassBuilder(PinCredentialSecretPayloadClassName)
        .addProperty(StringPropertyBuilder("Pin", "").setDescription("The PIN code.").build())
        .build();
}

/*!
 * @brief Name of the `IPropertyObjectClass` backing the default payload `createDefaultPayload()` builds for
 * `StandardPrivateKeyFilePayloadDescriptor`.
 */
inline constexpr const char* PrivateKeyFileCredentialSecretPayloadClassName = "PrivateKeyFileCredentialSecretPayload";

inline PropertyObjectClassPtr PrivateKeyFileCredentialSecretPayloadClass()
{
    return PropertyObjectClassBuilder(PrivateKeyFileCredentialSecretPayloadClassName)
        .addProperty(StringPropertyBuilder("PrivateKeyFilePath", "").setDescription("Path to the PEM-encoded private key file.").build())
        .build();
}

/*!
 * @brief The `IStructType` backing a `KeyValuePairs`-format descriptor's nested `"Parameters"` field - a
 * single `"Keys"` dict field. The single source of truth for this shape - `CredentialPayloadDescriptorImpl`
 * and `Context` (see `RegisterCredentialPayloadDescriptorTypes`) both build it from here, never redefine it.
 */
inline StructTypePtr KeyValuePayloadDescriptorParametersStructType()
{
    return StructType("KeyValuePayloadDescriptorParameters", List<IString>("Keys"), List<IType>(SimpleType(ctDict)));
}

/*!
 * @brief The `IStructType` backing a `String`-format descriptor's nested `"Parameters"` field - a single
 * `"Hidden"` bool field.
 */
inline StructTypePtr StringPayloadDescriptorParametersStructType()
{
    return StructType("StringPayloadDescriptorParameters", List<IString>("Hidden"), List<IType>(SimpleType(ctBool)));
}

/*!
 * @brief The `IStructType` backing a `KeyValuePairs`-format `CredentialPayloadDescriptor`.
 */
inline StructTypePtr KeyValuePayloadDescriptorStructType()
{
    return StructType("KeyValuePayloadDescriptor",
                      List<IString>("Id", "Description", "Parameters"),
                      List<IType>(SimpleType(ctString), SimpleType(ctString), KeyValuePayloadDescriptorParametersStructType()));
}

/*!
 * @brief The `IStructType` backing a `String`-format `CredentialPayloadDescriptor`.
 */
inline StructTypePtr StringPayloadDescriptorStructType()
{
    return StructType("StringPayloadDescriptor",
                      List<IString>("Id", "Description", "Parameters"),
                      List<IType>(SimpleType(ctString), SimpleType(ctString), StringPayloadDescriptorParametersStructType()));
}

/*!
 * @brief The `IStructType` backing a `FilePath`-format `CredentialPayloadDescriptor` - has no `"Parameters"`
 * field, since the format has no format-specific parameters.
 */
inline StructTypePtr FilePathPayloadDescriptorStructType()
{
    return StructType("FilePathPayloadDescriptor",
                      List<IString>("Id", "Description"),
                      List<IType>(SimpleType(ctString), SimpleType(ctString)));
}

/*!
 * @brief The `IStructType` backing a `None`-format `CredentialPayloadDescriptor` - has no `"Parameters"`
 * field, since the format has no format-specific parameters.
 */
inline StructTypePtr NonePayloadDescriptorStructType()
{
    return StructType("NonePayloadDescriptor",
                      List<IString>("Id", "Description"),
                      List<IType>(SimpleType(ctString), SimpleType(ctString)));
}

/*!
 * @brief Registers the four payload formats' backing `IStructType`s, plus the standard descriptors' own
 * default-payload `IPropertyObjectClass`es, with `typeManager`. Called once by `Context` up front.
 * @param typeManager The type manager to register the payload descriptor types with.
 */
inline void RegisterCredentialPayloadDescriptorTypes(const TypeManagerPtr& typeManager)
{
    for (const auto& type : {KeyValuePayloadDescriptorStructType(),
                             KeyValuePayloadDescriptorParametersStructType(),
                             StringPayloadDescriptorStructType(),
                             StringPayloadDescriptorParametersStructType(),
                             FilePathPayloadDescriptorStructType(),
                             NonePayloadDescriptorStructType()})
    {
        checkErrorInfoExcept(typeManager->addType(type), OPENDAQ_ERR_ALREADYEXISTS);
    }

    for (const auto& payloadClass : {UserNamePasswordCredentialSecretPayloadClass(),
                                     PinCredentialSecretPayloadClass(),
                                     PrivateKeyFileCredentialSecretPayloadClass()})
    {
        checkErrorInfoExcept(typeManager->addType(payloadClass), OPENDAQ_ERR_ALREADYEXISTS);
    }
}

/*!
 * @brief Creates a `CredentialPayloadDescriptor` describing a `KeyValuePairs`-format payload.
 * @param id The id that uniquely identifies this authentication method within the module that offers it.
 * @param keys The expected keys, mapped to whether the corresponding value should be hidden as it is
 * entered (e.g. `{"UserName": False, "Password": True}`).
 * @param description A human-readable description of the payload, for the user.
 * @param typeManager Must already have a `"KeyValuePayloadDescriptor"` type registered (see
 * `RegisterCredentialPayloadDescriptorTypes`) - a real `Context` always registers it up front. Throws
 * otherwise.
 * @param payloadClassName Must be assigned and registered with `typeManager` - `createDefaultPayload()`
 * builds the returned payload from this `IPropertyObjectClass`. Throws otherwise.
 */
inline CredentialPayloadDescriptorPtr KeyValuePayloadDescriptor(const StringPtr& id,
                                                                const DictPtr<IString, IBoolean>& keys,
                                                                const StringPtr& description,
                                                                const TypeManagerPtr& typeManager,
                                                                const StringPtr& payloadClassName)
{
    CredentialPayloadDescriptorPtr obj(KeyValuePayloadDescriptor_Create(id, keys, description, typeManager, payloadClassName));
    return obj;
}

/*!
 * @brief Creates a `CredentialPayloadDescriptor` describing a `String`-format payload - a single secret,
 * e.g. a PIN, token, or API key.
 * @param id The id that uniquely identifies this authentication method within the module that offers it.
 * @param description A human-readable description of the payload, for the user.
 * @param hidden Whether the secret should be hidden as it is entered.
 * @param typeManager Must already have a `"StringPayloadDescriptor"` type registered (see
 * `RegisterCredentialPayloadDescriptorTypes`) - a real `Context` always registers it up front. Throws
 * otherwise.
 * @param payloadClassName Must be assigned and registered with `typeManager` - `createDefaultPayload()`
 * builds the returned payload from this `IPropertyObjectClass`. Throws otherwise.
 */
inline CredentialPayloadDescriptorPtr StringPayloadDescriptor(const StringPtr& id,
                                                               const StringPtr& description,
                                                               Bool hidden,
                                                               const TypeManagerPtr& typeManager,
                                                               const StringPtr& payloadClassName)
{
    CredentialPayloadDescriptorPtr obj(StringPayloadDescriptor_Create(id, description, hidden, typeManager, payloadClassName));
    return obj;
}

/*!
 * @brief Creates a `CredentialPayloadDescriptor` describing a `FilePath`-format payload - a single secret
 * stating that the secret is a path to a file (e.g. a private key) rather than the value itself.
 * @param id The id that uniquely identifies this authentication method within the module that offers it.
 * @param description A human-readable description of the payload, for the user.
 * @param typeManager Must already have a `"FilePathPayloadDescriptor"` type registered (see
 * `RegisterCredentialPayloadDescriptorTypes`) - a real `Context` always registers it up front. Throws
 * otherwise.
 * @param payloadClassName Must be assigned and registered with `typeManager` - `createDefaultPayload()`
 * builds the returned payload from this `IPropertyObjectClass`. Throws otherwise.
 */
inline CredentialPayloadDescriptorPtr FilePathPayloadDescriptor(const StringPtr& id,
                                                                 const StringPtr& description,
                                                                 const TypeManagerPtr& typeManager,
                                                                 const StringPtr& payloadClassName)
{
    CredentialPayloadDescriptorPtr obj(FilePathPayloadDescriptor_Create(id, description, typeManager, payloadClassName));
    return obj;
}

/*!
 * @brief Creates a `CredentialPayloadDescriptor` describing a `None`-format payload - no secret(s) at all,
 * for an authentication method that requires no credentials, e.g. anonymous access. Unlike the other
 * formats, there is no payload to build, so no payload class is involved - `createDefaultPayload()` simply
 * returns an empty property object.
 * @param id The id that uniquely identifies this authentication method within the module that offers it.
 * @param description A human-readable description of the payload, for the user.
 * @param typeManager Must already have a `"NonePayloadDescriptor"` type registered (see
 * `RegisterCredentialPayloadDescriptorTypes`) - a real `Context` always registers it up front. Throws
 * otherwise.
 */
inline CredentialPayloadDescriptorPtr NonePayloadDescriptor(const StringPtr& id,
                                                             const StringPtr& description,
                                                             const TypeManagerPtr& typeManager)
{
    CredentialPayloadDescriptorPtr obj(NonePayloadDescriptor_Create(id, description, typeManager));
    return obj;
}

/*!
 * @brief Ids of the four standard authentication methods below - shared, well-known payload ids every
 * module can build the exact same descriptor for, instead of each one inventing its own shape/id for the
 * same method.
 */
inline constexpr const char* StandardUserNamePasswordPayloadId = "UserNamePassword";
inline constexpr const char* StandardPinPayloadId = "Pin";
inline constexpr const char* StandardPrivateKeyFilePayloadId = "PrivateKeyFile";
inline constexpr const char* StandardAnonymousPayloadId = "Anonymous";

/*!
 * @brief The standard `UserName`/`Password` authentication method's `CredentialPayloadDescriptor` - a
 * `KeyValuePairs`-format payload with the password hidden as typed.
 * @param typeManager See `KeyValuePayloadDescriptor`.
 */
inline CredentialPayloadDescriptorPtr StandardUserNamePasswordPayloadDescriptor(const TypeManagerPtr& typeManager)
{
    return KeyValuePayloadDescriptor(StandardUserNamePasswordPayloadId,
                                     Dict<IString, IBoolean>({{"UserName", False}, {"Password", True}}),
                                     "Username and password",
                                     typeManager,
                                     UserNamePasswordCredentialSecretPayloadClassName);
}

/*!
 * @brief The standard PIN authentication method's `CredentialPayloadDescriptor` - a `String`-format
 * payload, hidden as typed.
 * @param typeManager See `StringPayloadDescriptor`.
 */
inline CredentialPayloadDescriptorPtr StandardPinPayloadDescriptor(const TypeManagerPtr& typeManager)
{
    return StringPayloadDescriptor(StandardPinPayloadId, "PIN code", True, typeManager, PinCredentialSecretPayloadClassName);
}

/*!
 * @brief The standard private-key-file authentication method's `CredentialPayloadDescriptor` - a
 * `FilePath`-format payload.
 * @param typeManager See `FilePathPayloadDescriptor`.
 */
inline CredentialPayloadDescriptorPtr StandardPrivateKeyFilePayloadDescriptor(const TypeManagerPtr& typeManager)
{
    return FilePathPayloadDescriptor(
        StandardPrivateKeyFilePayloadId, "Path to the PEM-encoded private key file", typeManager, PrivateKeyFileCredentialSecretPayloadClassName);
}

/*!
 * @brief The standard anonymous authentication method's `CredentialPayloadDescriptor` - a `None`-format
 * payload, requiring no credentials at all.
 * @param typeManager See `NonePayloadDescriptor`.
 */
inline CredentialPayloadDescriptorPtr StandardAnonymousPayloadDescriptor(const TypeManagerPtr& typeManager)
{
    return NonePayloadDescriptor(StandardAnonymousPayloadId, "No credentials required", typeManager);
}

END_NAMESPACE_OPENDAQ
