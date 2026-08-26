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
#include <opendaq/authentication_config.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @brief Extends `IAuthenticationConfig` with the nested per-streaming-type authentication configs
 * relevant to a mirrored device - the thing `MirroredDeviceBase`/`StreamingSourceManager` consult to
 * decide which of the device's discovered streaming capabilities to auto-attach, and with which
 * authentication. An authentication config nested under a streaming type (i.e. one of the values in
 * `getStreamingAuthenticationConfigs`'s own dictionary) is an ordinary `IAuthenticationConfig` and does
 * not need to expose this itself.
 */
DECLARE_OPENDAQ_INTERFACE(IAuthenticationConfigMirrored, IAuthenticationConfig)
{
    /*!
     * @brief Gets the authentication configs nested under this one, accumulated via
     * `IAuthenticationConfigBuilder::addStreamingAuthenticationConfig` - so a single authentication config,
     * formed for connecting to a device, can also carry the settings needed to authenticate a streaming
     * source attached to that device. Each is an ordinary authentication config in its own right, keyed by
     * the streaming type's own id.
     * @param[out] streamingAuthenticationConfigs The streaming type id -> authentication config dictionary.
     */
    // [templateType(streamingAuthenticationConfigs, IString, IAuthenticationConfig)]
    virtual ErrCode INTERFACE_FUNC getStreamingAuthenticationConfigs(IDict** streamingAuthenticationConfigs) = 0;
};

END_NAMESPACE_OPENDAQ
