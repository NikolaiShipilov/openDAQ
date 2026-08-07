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
 */

DECLARE_OPENDAQ_INTERFACE(ICredentialRequestBuilder, IBaseObject)
{
virtual ErrCode INTERFACE_FUNC build(ICredentialRequest** request) = 0;

// [returnSelf]
virtual ErrCode INTERFACE_FUNC setConnectionString(IString* connectionString) = 0;

virtual ErrCode INTERFACE_FUNC getConnectionString(IString** connectionString) = 0;

// [returnSelf]
virtual ErrCode INTERFACE_FUNC setManufacturer(IString* manufacturer) = 0;

virtual ErrCode INTERFACE_FUNC getManufacturer(IString** manufacturer) = 0;

// [returnSelf]
virtual ErrCode INTERFACE_FUNC setSerialNumber(IString* serialNumber) = 0;

virtual ErrCode INTERFACE_FUNC getSerialNumber(IString** serialNumber) = 0;

// [returnSelf]
virtual ErrCode INTERFACE_FUNC addMetaDataProperty(IProperty* property) = 0;

virtual ErrCode INTERFACE_FUNC getMetaData(IPropertyObject** property) = 0;

// [returnSelf]
virtual ErrCode INTERFACE_FUNC setPayloadId(IString* payloadId) = 0;

virtual ErrCode INTERFACE_FUNC getPayloadId(IString** payloadId) = 0;

// [returnSelf]
virtual ErrCode INTERFACE_FUNC setPayloadDescriptor(ICredentialPayloadDescriptor* descriptor) = 0;

virtual ErrCode INTERFACE_FUNC getPayloadDescriptor(ICredentialPayloadDescriptor** descriptor) = 0;
};

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, CredentialRequestBuilder, ICredentialRequestBuilder
)

END_NAMESPACE_OPENDAQ
