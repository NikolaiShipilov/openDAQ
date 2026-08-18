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
#include <coreobjects/property.h>
#include <opendaq/credential_request.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [interfaceLibrary(IPropertyObject, "coreobjects")]
 * [interfaceSmartPtr(IPropertyObject, PropertyObjectPtr, "<coreobjects/property_object.h>")]
 * [interfaceLibrary(IProperty, "coreobjects")]
 * [interfaceSmartPtr(IProperty, PropertyPtr, "<coreobjects/property_ptr.h>")]
 * [interfaceLibrary(ICredentialPayloadDescriptor, "opendaq")]
 * [interfaceSmartPtr(ICredentialPayloadDescriptor, CredentialPayloadDescriptorPtr, "<opendaq/credential_payload_descriptor_ptr.h>")]
 * [interfaceLibrary(IComponentType, "opendaq")]
 * [interfaceSmartPtr(IComponentType, GenericComponentTypePtr, "<opendaq/component_type_ptr.h>")]
 */

/*!
 * @brief Builds `ICredentialRequest` objects - used by a device/module to form the request it hands to
 * `ICredentialProvider::requestCredentials` when authentication is required for a connection attempt.
 */
DECLARE_OPENDAQ_INTERFACE(ICredentialRequestBuilder, IBaseObject)
{
/*!
 * @brief Builds and returns a `CredentialRequest` using the currently configured values.
 * @param[out] request The built credential request.
 */
virtual ErrCode INTERFACE_FUNC build(ICredentialRequest** request) = 0;

/*!
 * @brief Sets the type of the component the request is being built for.
 * @param componentType The component type.
 */
// [returnSelf]
// [templateType(componentType, IComponentType)]
virtual ErrCode INTERFACE_FUNC setComponentType(IComponentType* componentType) = 0;

/*!
 * @brief Gets the type of the component the request is being built for.
 * @param[out] componentType The component type.
 */
// [templateType(componentType, IComponentType)]
virtual ErrCode INTERFACE_FUNC getComponentType(IComponentType** componentType) = 0;

/*!
 * @brief Sets the connection string used for the connection attempt the request is being built for.
 * @param connectionString The connection string.
 */
// [returnSelf]
virtual ErrCode INTERFACE_FUNC setConnectionString(IString* connectionString) = 0;

/*!
 * @brief Gets the connection string used for the connection attempt the request is being built for.
 * @param[out] connectionString The connection string.
 */
virtual ErrCode INTERFACE_FUNC getConnectionString(IString** connectionString) = 0;

/*!
 * @brief Sets the manufacturer of the device the request is being built for.
 * @param manufacturer The device manufacturer.
 */
// [returnSelf]
virtual ErrCode INTERFACE_FUNC setManufacturer(IString* manufacturer) = 0;

/*!
 * @brief Gets the manufacturer of the device the request is being built for.
 * @param[out] manufacturer The device manufacturer.
 */
virtual ErrCode INTERFACE_FUNC getManufacturer(IString** manufacturer) = 0;

/*!
 * @brief Sets the serial number of the device the request is being built for.
 * @param serialNumber The device serial number.
 */
// [returnSelf]
virtual ErrCode INTERFACE_FUNC setSerialNumber(IString* serialNumber) = 0;

/*!
 * @brief Gets the serial number of the device the request is being built for.
 * @param[out] serialNumber The device serial number.
 */
virtual ErrCode INTERFACE_FUNC getSerialNumber(IString** serialNumber) = 0;

/*!
 * @brief Adds a property to the request's metadata, describing additional, request-specific information
 * for the credential provider to present to the user (e.g. device type name/id/description).
 * @param property The metadata property to add.
 */
// [returnSelf]
virtual ErrCode INTERFACE_FUNC addMetaDataProperty(IProperty* property) = 0;

/*!
 * @brief Gets the metadata property object accumulated via `addMetaDataProperty`.
 * @param[out] property The metadata property object.
 */
virtual ErrCode INTERFACE_FUNC getMetaData(IPropertyObject** property) = 0;

/*!
 * @brief Sets the id of the negotiated payload - obtained from `IAuthenticationConfig` - serialized on
 * save & replayed on load.
 * @param payloadId The payload id.
 */
// [returnSelf]
virtual ErrCode INTERFACE_FUNC setPayloadId(IString* payloadId) = 0;

/*!
 * @brief Gets the id of the negotiated payload - obtained from `IAuthenticationConfig` - serialized on
 * save & replayed on load.
 * @param[out] payloadId The payload id.
 */
virtual ErrCode INTERFACE_FUNC getPayloadId(IString** payloadId) = 0;

/*!
 * @brief Sets the descriptor of the payload the provider must provide - serialized on save or re-attached
 * from the device type on load.
 * @param descriptor The payload descriptor.
 */
// [returnSelf]
virtual ErrCode INTERFACE_FUNC setPayloadDescriptor(ICredentialPayloadDescriptor* descriptor) = 0;

/*!
 * @brief Gets the descriptor of the payload the provider must provide - serialized on save or re-attached
 * from the device type on load.
 * @param[out] descriptor The payload descriptor.
 */
virtual ErrCode INTERFACE_FUNC getPayloadDescriptor(ICredentialPayloadDescriptor** descriptor) = 0;
};

/*!
 * @brief Creates a `CredentialRequestBuilder` with no values set.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, CredentialRequestBuilder, ICredentialRequestBuilder
)

END_NAMESPACE_OPENDAQ
