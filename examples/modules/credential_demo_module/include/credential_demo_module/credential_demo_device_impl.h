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
#include <credential_demo_module/common.h>
#include <opendaq/device_impl.h>
#include <opendaq/credential_request_ptr.h>
#include <opendaq/credential_payload_ptr.h>

/*
 * Minimal device implementation with no signals or channels. Authenticates
 * via the credential framework using a username/password, as the first showcased auth method.
 */

BEGIN_NAMESPACE_CREDENTIAL_DEMO_MODULE

class CredentialDemoDeviceImpl final : public Device
{
public:
    explicit CredentialDemoDeviceImpl(const PropertyObjectPtr& config, const ContextPtr& ctx, const ComponentPtr& parent, const DeviceInfoPtr& info, const CredentialPayloadPtr& credentials);

    static DeviceInfoPtr CreateDeviceInfo(const DictPtr<IString, IBaseObject>& moduleOptions);
    static DeviceTypePtr CreateType();
    static CredentialRequestPtr CreateCredentialRequest(const StringPtr& connectionString, const PropertyObjectPtr& config);
    static void ValidateConnectionString(const StringPtr& connectionString, const DeviceInfoPtr& info);
};

END_NAMESPACE_CREDENTIAL_DEMO_MODULE
