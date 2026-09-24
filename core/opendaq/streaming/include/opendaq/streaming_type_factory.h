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
#include <opendaq/streaming_type_ptr.h>
#include <coretypes/struct_type_factory.h>
#include <coretypes/simple_type_factory.h>
#include <coreobjects/property_object_factory.h>
#include <opendaq/component_type_builder_factory.h>
#include <opendaq/credential_descriptor_factory.h>
#include <coretypes/dictobject_factory.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_streaming_type
 * @addtogroup opendaq_streaming_type_factories Factories
 * @{
 */

/*!
 * @brief Creates a StreamingType pointer, with the id, name, description and optional defaultConfig.
 * @param id The unique type ID of the Streaming.
 * @param name The name of the Streaming. Eg. OpenDAQNativeStreaming.
 * @param description A short description of the function block and its behaviour.
 * @param prefix The prefix of the connection string used to add this streaming connection to a device.
 * @param defaultConfig The property object, to be cloned and returned, each time user creates default
 * configuration object. This way each instance of the Streaming has its own configuration object.
 * @param supportedAuthenticationMethods The credential descriptors this streaming type supports authenticating
 * with, keyed by their own id. Left unset, the streaming type defaults to supporting only the
 * standard `"Anonymous"` method - no credentials required.
 * @param defaultAuthenticationMethodId The id of the authentication method this streaming type supports by
 * default. Left unset, defaults to the standard `"Anonymous"` id.
 */
inline StreamingTypePtr StreamingType(const StringPtr& id,
                                              const StringPtr& name,
                                              const StringPtr& description,
                                              const StringPtr& prefix,
                                              const PropertyObjectPtr& defaultConfig = PropertyObject(),
                                              const DictPtr<IString, ICredentialDescriptor>& supportedAuthenticationMethods = AnonymousOnlySupportedAuthenticationMethods(),
                                              const StringPtr& defaultAuthenticationMethodId = StandardAnonymousId)
{
    StreamingTypePtr obj(
        StreamingType_Create(id, name, description, prefix, defaultConfig, supportedAuthenticationMethods, defaultAuthenticationMethodId));
    return obj;
}

/*!
 * @brief Creates the Struct type object that defines the Function block type struct.
 */
inline StructTypePtr StreamingTypeStructType()
{
    return StructType("StreamingType",
                      List<IString>("Id", "Name", "Description", "Prefix", "SupportedAuthenticationMethods", "DefaultAuthenticationMethodId"),
                      List<IBaseObject>("", "", "", "", Dict<IString, IBaseObject>(), ""),
                      List<IType>(SimpleType(ctString),
                                 SimpleType(ctString),
                                 SimpleType(ctString),
                                 SimpleType(ctString),
                                 SimpleType(ctDict),
                                 SimpleType(ctString)));
}

/*!@}*/

END_NAMESPACE_OPENDAQ
