# Credential Provider Framework — API Reference & Authentication Flow

Credentials are modelled by their **payload shape** (`CredentialPayloadFormat`: `KeyValuePairs`, `String`, or `FilePath`) and a **payload descriptor** (`ICredentialPayloadDescriptor`) carrying format-specific parameters and a human description. Authentication method selection happens through an `IAuthenticationConfig` object - a single, self-contained property object a component type hands back via `createDefaultAuthenticationConfig()`, listing every method the type supports (added via `IComponentTypeBuilder::addSupportedAuthenticationDescriptor`) as a candidate the caller selects among directly, tunes, and hands to `addAuthenticatedDevice`/`addStreaming` - or one assembled ad hoc via `IAuthenticationConfigBuilder`.

---

## 1. Core Interfaces

### `ICredentialPayloadDescriptor`

Describes the shape and presentation of the payload an authentication method expects.

| Member | Description |
|---|---|
| `getId(IString**)` | The id that uniquely identifies this authentication method within the module that offers it - the same id `IComponentTypeBuilder::setDefaultAuthenticationConfigId` targets, and that a caller matches against when selecting this method out of an `IAuthenticationConfig`'s `"PayloadDescriptor"` candidates (see below). |
| `getFormat(CredentialPayloadFormat*)` | The payload's format — `KeyValuePairs`, `String`, or `FilePath`. |
| `getParameters(IStruct**)` | The format's standard parameter set, as a Struct whose own Struct type is pinned to the format - for `KeyValuePairs`, a `"Keys"` dict field mapping each expected key to a hidden flag (e.g. `{"UserName": False, "Password": True}`); for `String`, a single `"Hidden"` bool field; for `FilePath`, no fields at all. |
| `getDescription(IString**)` | Human-readable description of the payload, e.g. *"PIN-code"*, *"username and password"*, *"Path to the SSH private key file"*. |
| `createDefaultPayload(IPropertyObject**)` | Builds an empty credential payload template matching this format: a property object with one empty (default `""`) String property per secret expected - one per key named in `getParameters()`'s `"Keys"` dict for `KeyValuePairs` (e.g. `"UserName"`, `"Password"`), or a single `"Secret"` property for `String`/`FilePath`. Meant to be filled in with the actual secret value(s) and used as the credential payload itself - see [§5](#5-credential-provider-selection--supplied-secrets). |

**Factories:** `KeyValuePayloadDescriptor(id, keys, description, typeManager)`, `StringPayloadDescriptor(id, description, hidden, typeManager)`, `FilePathPayloadDescriptor(id, description, typeManager)` - `typeManager` is optional; if assigned and it already has the format's struct type registered, the descriptor is built with that registered type instead of an independently-built, unregistered one. `Context` registers all three formats' struct types up front (`RegisterCredentialPayloadDescriptorTypes`, called from `ContextImpl::registerOpenDaqTypes`), so this is the case for any descriptor built with a real `Context`'s type manager.

```cpp
enum class CredentialPayloadFormat : EnumType
{
    KeyValuePairs,  // N string pairs — e.g. UserName / Password
    String,         // one string — token, API key, PIN
    FilePath        // one string — path to a file containing the secret, e.g. a private key
};
```

---

### `IAuthenticationConfig`

Carries the authentication settings for a single connection attempt. Lives alongside the base add-component config, never serialized as part of it - though a component created with authentication may persist the whole config it was authenticated with alongside itself instead (see `IComponentPrivate::setAuthenticationConfig`, [§7](#7-application-level-usage)).

`IAuthenticationConfig` extends `IPropertyObject` (the same way `IDeviceInfo` does) - the settings below are backed by ordinary properties, so besides the typed getters/builder setters, they can equally be read (and, for a builder-built instance, set) through the generic `IPropertyObject` interface:

| Property | Description |
|---|---|
| `"PayloadDescriptor"` (Selection) | The payload id/descriptor pair, bound together as one property so they can never be set out of sync: its selection value is the `ICredentialPayloadDescriptor` Struct itself, and the payload id is simply that Struct's own `getId()`. A config returned by `IComponentType::createDefaultAuthenticationConfig()` (see [§2](#2-extensions-to-existing-interfaces)) is self-contained - every authentication method the component type supports is a selection candidate here, not just one - so the caller switches methods by changing this property's selection, without fetching a different config object. A config built via `IAuthenticationConfigBuilder` has exactly one candidate, itself. |
| `"CredentialProviderId"` (String) | The id of a specifically selected credential provider, or empty (the default) if none was chosen — in which case the module auto-selects a registered provider supporting the payload descriptor's format. |
| `"SuppliedSecret"` (Object, optional) | A secret supplied directly by the caller, to be used instead of a provider obtaining it. Present only when one was actually supplied — check with `hasProperty("SuppliedSecret")` — since an Object-type property cannot itself hold `nullptr`. See [§5](#5-credential-provider-selection--supplied-secrets). |

| Member | Description |
|---|---|
| `getCredentialPayloadId(IString**)` | Convenience getter for the selected `"PayloadDescriptor"` value's own id. |
| `getCredentialPayloadDescriptor(ICredentialPayloadDescriptor**)` | Convenience getter for the `"PayloadDescriptor"` property's current selection value. |
| `getCredentialProviderId(IString**)` | Convenience getter for the `"CredentialProviderId"` property, translating an empty string back to `nullptr`. |

There is no "additional config" on `IAuthenticationConfig` itself - settings like whether to hide secret input as it's typed travel via the component's own, generic config object instead (the same one `addDevice`/`addStreaming` always take), read by the module from its own `config` parameter alongside the authentication config.

**Factories:**
- `AuthenticationConfig(payloadDescriptors, defaultPayloadId)` — builds a config listing every one of `payloadDescriptors` (a dict keyed by each descriptor's own id) as a `"PayloadDescriptor"` candidate, defaulting to the one named by `defaultPayloadId`. A single-method config is just the one-entry case of this - there's no separate single-descriptor factory, since it would add nothing this one doesn't already cover. None of the builder-only settings below (provider id, supplied secret) are set. This is what `IComponentType::createDefaultAuthenticationConfig()` uses internally to build its self-contained, every-method config.
- `AuthenticationConfigBuilder()` — see `IAuthenticationConfigBuilder` below; the only way to set a provider id or a supplied secret, for a single-method config.

---

### `IAuthenticationConfigBuilder`

Builds `IAuthenticationConfig` objects, exposing every setting the plain factory doesn't.

| Member | Description |
|---|---|
| `build(IAuthenticationConfig**)` | Builds and returns an `AuthenticationConfig` from the currently configured values. |
| `setPayloadDescriptor` / `getPayloadDescriptor` | The descriptor of the payload the selected method uses - its own `getId()` becomes the built config's payload id. |
| `setCredentialProviderId` / `getCredentialProviderId` | Selects a specific registered provider by id, bypassing format-based auto-selection. `nullptr` (the default) leaves auto-selection in place. |
| `setSuppliedSecret` / `getSuppliedSecret` | Supplies the secret directly - a property object built from `setPayloadDescriptor`'s `createDefaultPayload` template and filled in with the actual secret value(s). `nullptr` (the default) leaves the module to obtain it from a provider. |

**Factory:** `AuthenticationConfigBuilder()` — starts with no values set.

---

### `ICredentialRequest`

Carries the non-secret details of a credential request, handed to `ICredentialProvider::requestCredentials`/`cacheCredentials`. Built via `ICredentialRequestBuilder`, or reconstructed on load. Never carries actual secrets.

| Member | Description |
|---|---|
| `getComponentType(IComponentType**)` | The type of component the request is for. |
| `getConnectionString(IString**)` | The connection string used for this connection attempt. |
| `getMetaData(IPropertyObject**)` | Additional metadata for the provider to present to the user (e.g. device type name/id/description). |
| `getManufacturer(IString**)` | The manufacturer of the device the request is for. |
| `getSerialNumber(IString**)` | The serial number of the device the request is for. |
| `getPayloadId(IString**)` | The id of the negotiated payload (from `IAuthenticationConfig`) — serialized on save, replayed on load. |
| `getPayloadDescriptor(ICredentialPayloadDescriptor**)` | The descriptor of the payload the provider must provide — serialized on save, or re-attached from the device type on load. |

**Factory:** `CredentialRequestFromBuilder(builder)` — hidden factory, built from a `ICredentialRequestBuilder`.

---

### `ICredentialRequestBuilder`

Builds `ICredentialRequest` objects.

| Member | Description |
|---|---|
| `build(ICredentialRequest**)` | Builds and returns a `CredentialRequest` from the currently configured values. |
| `setComponentType` / `getComponentType` | The component type the request is being built for. |
| `setConnectionString` / `getConnectionString` | The connection string for this attempt. |
| `setManufacturer` / `getManufacturer` | The device manufacturer. |
| `setSerialNumber` / `getSerialNumber` | The device serial number. |
| `addMetaDataProperty(IProperty*)` | Adds a metadata property, for the provider to present to the user. |
| `getMetaData(IPropertyObject**)` | The accumulated metadata property object. |
| `setPayloadId` / `getPayloadId` | The id of the negotiated payload. |
| `setPayloadDescriptor` / `getPayloadDescriptor` | The descriptor of the payload the provider must supply. |

**Factory:** `CredentialRequestBuilder()`

---

### Credential payload

There is no dedicated payload interface - a credential payload is simply an `IPropertyObject`, built from the payload descriptor's `createDefaultPayload()` template and filled in with the actual secret value(s). For a `KeyValuePairs`-format payload it has one String property per key (e.g. `"UserName"`, `"Password"`); for `String`/`FilePath` it has a single `"Secret"` String property. Both a credential provider's `requestCredentials` and a caller directly supplying a secret (`IAuthenticationConfigBuilder::setSuppliedSecret`) produce/consume this exact same shape - see [§5](#5-credential-provider-selection--supplied-secrets).

---

### `ICredentialProvider`

Supplies the secrets requested via an `ICredentialRequest` — by prompting the user, reading a file, or fetching from a secret store.

| Member | Description |
|---|---|
| `getId(IString**)` | The provider's id. |
| `requestCredentials(ICredentialRequest*, IPropertyObject**)` | Requests credentials for the given request, in the format described by its payload descriptor — obtaining them interactively (prompting, reading a file, etc.) unless a cached value from an earlier `cacheCredentials`/`requestCredentials` call for the same context already covers it. Returns a property object built from the descriptor's `createDefaultPayload` template, filled in with the obtained secret(s). |
| `cacheCredentials(ICredentialRequest*, IPropertyObject* secret)` | Accepts a secret already known in advance (see `IAuthenticationConfig`'s `"SuppliedSecret"` property), shaped like the request's payload descriptor's `createDefaultPayload` template, so an implementation that would otherwise cache a value obtained interactively caches this one the same way — a later `requestCredentials` call for the same context then reuses it instead of prompting. Produces no payload itself; the caller already has the secret and uses it directly. Implementations for which caching doesn't apply (or doesn't apply to the request's format) may treat this as a no-op. |
| `getSupportedPayloadFormats(IList**)` | The list of `CredentialPayloadFormat` values this provider can supply — used for format-matching against a device type's supported formats. |

**Factories:**
- `CmdLineCredentialProvider()` — prompts the user for secrets via the command line. Caches `FilePath`-format secrets in-memory for its own lifetime (i.e. for the active session), keyed by `(manufacturer, serialNumber)` — a second interactive request for the same device and format reuses the path already entered (or supplied via `cacheCredentials`) instead of prompting again. `String`/`KeyValuePairs` secrets are never cached.
- `FileCredentialProvider()` — dedicated to file-backed secrets. Prompts for the file's path via the command line, the same way `CmdLineCredentialProvider` does, and hands back the path itself for a `FilePath`-format request. Retries the path prompt up to 3 times if the given path isn't accessible, then fails authentication. Never caches anything — `cacheCredentials` is a no-op.

---

## 2. Extensions to Existing Interfaces

### `IComponentType`

| New member | Description |
|---|---|
| `createDefaultAuthenticationConfig(IAuthenticationConfig**)` | Builds and returns a new, self-contained authentication config; a new object on each call, same as `createDefaultConfig`. Its `"PayloadDescriptor"` property has every payload descriptor added to the type's builder as a selection candidate, defaulting to the one set via `setDefaultAuthenticationConfigId`. Returns `OPENDAQ_ERR_NOT_SUPPORTED` if the type doesn't support authentication (no default authentication config id set on its builder). |
| `isAuthenticationSupported(Bool*)` | `True` if at least one payload descriptor was added and a matching default id was set — in which case `createDefaultAuthenticationConfig` is guaranteed to succeed. |

### `IComponentTypeBuilder`

| New member | Description |
|---|---|
| `setDefaultAuthenticationConfigId(IString*)` | Sets which added method (by payload id) is selected by default in the config `createDefaultAuthenticationConfig` returns. Left unset ⇒ the built type doesn't support authentication. |
| `getDefaultAuthenticationConfigId(IString**)` | Gets the id set above, or `nullptr`. |
| `addSupportedAuthenticationDescriptor(ICredentialPayloadDescriptor*)` | Adds a supported authentication method's payload descriptor - its own `getId()` is the id it's known by. Every descriptor added this way becomes a selection candidate of the one, self-contained config `createDefaultAuthenticationConfig` returns - there is no separate config built per method. |
| `getSupportedAuthenticationDescriptors(IList**)` | The payload descriptors accumulated so far. |

**Validation on build:** if descriptors were added but no default id was set (or vice versa), or the default id doesn't match any added descriptor's own id, `build()` fails with `OPENDAQ_ERR_INVALIDPARAMETER`.

### `IDevice`

| New member | Description |
|---|---|
| `addAuthenticatedDevice(IDevice**, IString* connectionString, IPropertyObject* config = nullptr, IAuthenticationConfig* authenticationConfig = nullptr)` | Connects to a device using the given authentication configuration. |
| `addStreaming(IStreaming**, IString* connectionString, IPropertyObject* config = nullptr, IAuthenticationConfig* authenticationConfig = nullptr)` | Attaches a streaming connection, optionally authenticated — a `nullptr` config (the default) uses the plain, unauthenticated path. See [§4](#4-streaming-authentication). |

### `IModuleManagerUtils`

| New member | Description |
|---|---|
| `createAuthenticatedDevice(IDevice**, IString* connectionString, IComponent* parent, IPropertyObject* config, IAuthenticationConfig* authenticationConfig)` | Iterates loaded modules, creating a device with the first one accepting the connection string and supporting authentication. Manufacturer/serial number are resolved from discovery info only for smart (`daq://`) connection strings; otherwise left unset. |
| `createStreaming(IStreaming**, IString* connectionString, IPropertyObject* config = nullptr, IAuthenticationConfig* authenticationConfig = nullptr, IString* manufacturer = nullptr, IString* serialNumber = nullptr)` | Iterates loaded modules, creating a streaming connection with the first one accepting the connection string. Unlike `createAuthenticatedDevice`, there is no smart-string/discovery resolution here — streaming connection strings are always concrete, protocol-specific ones — so `manufacturer`/`serialNumber` are simply forwarded as given. |

### `IModule`

| New member | Description |
|---|---|
| `createAuthenticatedDevice(IDevice**, IString* connectionString, IString* manufacturer, IString* serialNumber, IComponent* parent, IPropertyObject* config, IAuthenticationConfig* authenticationConfig)` | Module-level counterpart — receives manufacturer/serial resolved by the module manager, in addition to the authentication config. |
| `createStreaming(IStreaming**, IString* connectionString, IPropertyObject* config, IAuthenticationConfig* authenticationConfig, IString* manufacturer, IString* serialNumber)` | Module-level counterpart of `IModuleManagerUtils::createStreaming`. |

### `IInstanceBuilder`

| New member | Description |
|---|---|
| `getCredentialProviders(IDict**)` | The registered providers, keyed by id. |
| `addCredentialProvider(IString* providerId, ICredentialProvider*)` | Registers a provider under a unique id. |

### `IContext`

Extended with an additional `credentialProviders` parameter (`DictPtr<IString, ICredentialProvider>`) on the `Context` factory, and `getCredentialProviders(IDict**)` to retrieve them — providers registered on the instance builder flow through to the context, from which modules resolve them at authentication time.

---

## 3. Two Ways to Add a Device

The device can be added either **without authentication** (the plain, anonymous path) or **with authentication**. Both paths exist side by side — a device type that supports authentication doesn't lose its plain connection option, and the two are chosen simply by which method is called.

### The plain path (no authentication)

```cpp
auto device = instance.addDevice("daq://openDAQ_1234");
```

At the application level, this is a single call — no credential provider needs to be registered, and no authentication config is involved at all.

At the module level, this resolves to `Module::createDevice` (unchanged from before the credential framework existed). The device is constructed with `authenticated = false`, and the module's own secret-verification step is skipped entirely — no payload, no provider lookup, no challenge or comparison of any kind.

### The authenticated path

```cpp
auto device = instance.addAuthenticatedDevice("daq://openDAQ_1234", nullptr, authenticationConfig);
```

The same connection string is used, but an `IAuthenticationConfig` is supplied, and everything described in the rest of this document — payload negotiation, provider lookup, credential retrieval, verification — is triggered as a result.

A device type only exposes this path if it supports authentication at all (`IComponentType::isAuthenticationSupported`); calling `addAuthenticatedDevice` against a type that doesn't returns `OPENDAQ_ERR_NOT_SUPPORTED`.

---

## 4. Streaming Authentication

Attaching a streaming connection is authenticated **independently** of however the device itself got connected — a device authenticated via PIN can have a streaming source attached to it that goes through its own, entirely separate credential request. `IDevice::addStreaming`'s last parameter is the authentication config, exactly mirroring `addAuthenticatedDevice`: a `nullptr` config uses the plain, unauthenticated path; a supplied one triggers the same provider-lookup/credential-request machinery described for devices.

```cpp
// Plain, unauthenticated streaming attach:
device.addStreaming("daq.credential_demo_streaming://credential_demo_device");

// Authenticated streaming attach, with its own independent authentication config:
auto streamingAuthConfig = streamingType.createDefaultAuthenticationConfig();
SelectAuthenticationMethod(streamingAuthConfig, "PrivateKeyFile"); // see §7 - switches the selection generically
device.addStreaming("daq.credential_demo_streaming://credential_demo_device", nullptr, streamingAuthConfig);
```

### Auto-attach is always unauthenticated

`StreamingSourceManager` (the implementation behind the `PrioritizedStreamingProtocols`/`AutomaticallyConnectStreaming` add-device config) auto-attaches streaming sources for a device once it's added, always via the plain, unauthenticated path — regardless of whether the device itself was authenticated (`addAuthenticatedDevice`) or not. A streaming source that needs authentication must be attached manually, via `addStreaming`'s own authentication config parameter, as shown above.

![Streaming authentication — manual attach (unauthenticated or independently authenticated), and unauthenticated auto-attach](credential_flow_diagram_streaming.png)
*Diagram 2 — manual `addStreaming` (unauthenticated or independently authenticated), and `StreamingSourceManager`'s always-unauthenticated auto-attach. The authenticated manual path hands off to the same credential resolution shown in Diagram 1 ([§6](#6-authentication-flow)).*

---

## 5. Credential Provider Selection & Supplied Secrets

Two independent, combinable settings on `IAuthenticationConfig`/`IAuthenticationConfigBuilder` let a caller take over parts of the credential-provider machinery that would otherwise happen automatically.

### Explicit provider selection

By default, the module auto-selects the first registered provider whose `getSupportedPayloadFormats()` includes the required format. `setCredentialProviderId` overrides this — the module looks the given id up directly among the registered providers instead:

```cpp
auto config = AuthenticationConfigBuilder()
                   .setPayloadDescriptor(privateKeyFileConfig.getCredentialPayloadDescriptor())
                   .setCredentialProviderId(cmdLineCredentialProvider.getId())
                   .build();
```

This only makes an observable difference for a format more than one registered provider supports — e.g. `FilePath`, supported by both `FileCredentialProvider` and `CmdLineCredentialProvider`. An id naming no registered provider, or one that doesn't support the required format, fails authentication immediately with a message identifying the problem — it never silently falls back to auto-selection.

Since `IAuthenticationConfig` is itself a property object (like `IDeviceInfo`), the same thing can be done without the builder at all - `setPropertyValue`/`getPropertyValue` work directly on any `IAuthenticationConfig` instance, including the self-contained one `createDefaultAuthenticationConfig()` returns (a new, independent object on every call - safe to mutate directly, nothing else shares it):

```cpp
auto config = deviceType.createDefaultAuthenticationConfig();
SelectAuthenticationMethod(config, "PrivateKeyFile"); // see §7 for this helper
config.setPropertyValue("CredentialProviderId", cmdLineCredentialProvider.getId());
std::cout << config.getPropertyValue("CredentialProviderId") << std::endl;
```

### Supplying the secret directly

`setSuppliedSecret` hands the module a secret it already has — a property object built from the config's payload descriptor's `createDefaultPayload()` template and filled in with the actual value(s) — instead of having a provider obtain one interactively:

```cpp
auto userNamePasswordConfig = deviceType.createDefaultAuthenticationConfig(); // UserNamePassword is the default method

auto payload = userNamePasswordConfig.getCredentialPayloadDescriptor().createDefaultPayload();
payload.setPropertyValue("UserName", "user");
payload.setPropertyValue("Password", "pass");

auto config = AuthenticationConfigBuilder()
                   .setPayloadDescriptor(userNamePasswordConfig.getCredentialPayloadDescriptor())
                   .setSuppliedSecret(payload)
                   .build();
auto device = instance.addAuthenticatedDevice("daq://openDAQ_1234", nullptr, config);
```

- **No provider id set:** no provider is looked up at all — the module uses the supplied payload directly and authenticates with it, with no prompt of any kind.
- **A provider id is also set:** the module still uses the supplied payload directly for this connection, but first hands both it and the credential request to that specific provider via `ICredentialProvider::cacheCredentials`, so the provider can remember it the same way it would one obtained interactively.

The second combination is what makes it possible for a *later*, ordinary `requestCredentials` call — e.g. authenticating a streaming connection attached to the same device — to be served from the provider's cache instead of prompting, provided the provider actually caches for that format (see `CmdLineCredentialProvider`'s `FilePath` caching) and the two requests share the same `(manufacturer, serialNumber)`:

```cpp
auto devicePrivateKeyFileConfig = deviceType.createDefaultAuthenticationConfig();
SelectAuthenticationMethod(devicePrivateKeyFileConfig, "PrivateKeyFile");

auto devicePayload = devicePrivateKeyFileConfig.getCredentialPayloadDescriptor().createDefaultPayload();
devicePayload.setPropertyValue("Secret", "/path/to/private_key.pem");

auto deviceConfig = AuthenticationConfigBuilder()
                         .setPayloadDescriptor(devicePrivateKeyFileConfig.getCredentialPayloadDescriptor())
                         .setCredentialProviderId(cmdLineCredentialProvider.getId())
                         .setSuppliedSecret(devicePayload)
                         .build();
auto device = instance.addAuthenticatedDevice("daq://openDAQ_1234", nullptr, deviceConfig);

// Same device, same FilePath-format method, same provider - no path prompt this time; the provider
// serves it from the cache `cacheCredentials` populated above.
auto streamingPrivateKeyFileConfig = streamingType.createDefaultAuthenticationConfig();
SelectAuthenticationMethod(streamingPrivateKeyFileConfig, "PrivateKeyFile");
auto streamingConfig = AuthenticationConfigBuilder()
                           .setPayloadDescriptor(streamingPrivateKeyFileConfig.getCredentialPayloadDescriptor())
                           .setCredentialProviderId(cmdLineCredentialProvider.getId())
                           .build();
device.addStreaming("daq.credential_demo_streaming://credential_demo_device", nullptr, streamingConfig);
```

For the credential-demo module specifically, the device's own manufacturer/serial number are what `MirroredDeviceBase::onAddStreaming` resolves (from `IDeviceInfo`) and forwards into the streaming connection attempt, so a manually-attached streaming connection for the same device naturally lands in the same cache bucket.

---

## 6. Authentication Flow

Both ways of adding a device converge on the same entry point at the application level — a connection string and a config object — and diverge based on whether an `IAuthenticationConfig` is supplied.

Without one, the call resolves straight down to the module's plain device-construction path: the module manager locates the appropriate device type from the connection string, and the module builds the device with no further involvement from the credential framework at all.

With one, the module manager first confirms the target device type actually supports authentication, then hands off to the module together with the authentication config. From there, the module works out which payload it needs, resolves the credentials for it — a provider (auto-selected or explicitly chosen by id) obtaining them interactively, or a directly-supplied secret used instead (see [§5](#5-credential-provider-selection--supplied-secrets)) — and obtains a credential request, either reusing one carried over from a previous save (on reload) or building a fresh one. The module then verifies the obtained credentials and constructs the device. Whether the request was fresh or reused, the resulting credential request is stored on the device afterward, so a future reload can repeat this same process rather than needing the original secrets to be saved anywhere.

![Adding a device — with vs without authentication, and credential resolution (API-level flow)](credential_flow_diagram_device.png)
*Diagram 1 — the plain vs. authenticated add-device paths, and the credential-resolution branching (provider auto-selection vs. explicit id, interactive `requestCredentials` vs. a directly-supplied secret). The same resolution applies verbatim to authenticating a streaming connection (Diagram 2, [§4](#4-streaming-authentication)) — only the surrounding API call differs. The pink steps (provider selection policy, secret-wrapping) are how one module (the credential-demo prototype) chose to implement this — not something the core interfaces prescribe; see the note at the top of [§8](#8-module-level-implementation).*

---

## 7. Application-Level Usage

### Registering credential providers

```cpp
auto credentialProvider = CmdLineCredentialProvider();

auto instanceBuilder = InstanceBuilder();
instanceBuilder.addCredentialProvider(credentialProvider.getId(), credentialProvider);
auto instance = instanceBuilder.build();
```

Provider registration happens once, at instance-build time, and is **never serialized** — a reloaded instance must register its own providers independently, since provider setup is platform-/host-specific. Multiple providers can be registered; when more than one supports the same payload format, the first one registered is enumerated first by the module during auto-selection — unless a specific provider id is set on the authentication config, which bypasses that enumeration entirely (see [§5](#5-credential-provider-selection--supplied-secrets)).

### Selecting an authentication method

`createDefaultAuthenticationConfig()` returns one **self-contained** config listing every method the device type supports as a candidate of its `"PayloadDescriptor"` selection property (see [§1](#1-core-interfaces)) - not a separate config per method. The application either accepts the type's default selection, or switches it to a different supported method by matching payload ids, entirely through plain property object calls:

```cpp
// Selects one of the config's supported methods by payload id - manipulating the "PayloadDescriptor"
// selection property directly. No `ICredentialPayloadDescriptor` cast needed for the id comparison itself,
// only to read `getId()` off each candidate.
void SelectAuthenticationMethod(const AuthenticationConfigPtr& authConfig, const StringPtr& payloadId)
{
    ListPtr<IStruct> candidates = authConfig.getProperty("PayloadDescriptor").getSelectionValues();
    for (const auto& candidate : candidates)
    {
        if (candidate.asPtr<ICredentialPayloadDescriptor>().getId() == payloadId)
        {
            authConfig.setPropertySelectionValue("PayloadDescriptor", candidate);
            return;
        }
    }
    throw std::runtime_error("Unknown authentication method payload id: " + payloadId.toStdString());
}

auto deviceType = instance.getAvailableDeviceTypes().get("CredentialDemoDevice");

// Use the type's default authentication method:
auto config = deviceType.createDefaultAuthenticationConfig();
auto device = instance.addAuthenticatedDevice("daq://openDAQ_1234", nullptr, config);

// Or explicitly select a specific supported method instead (e.g. "Pin"), on a config of its own:
auto pinConfig = deviceType.createDefaultAuthenticationConfig();
SelectAuthenticationMethod(pinConfig, "Pin");
auto device2 = instance.addAuthenticatedDevice("daq://openDAQ_1234", nullptr, pinConfig);
```

### Example: manipulating the config as a plain property object

`IAuthenticationConfig` extends `IPropertyObject` (see [§1](#1-core-interfaces)), so the application can equally configure and inspect it with plain property object calls instead of the typed getters/builder setters used elsewhere in this section. No `IAuthenticationConfigBuilder` needed either - `createDefaultAuthenticationConfig()` already returns a fresh, independent, self-contained config, ready to tune directly:

```cpp
auto authConfig = deviceType.createDefaultAuthenticationConfig();
SelectAuthenticationMethod(authConfig, "Pin");

StructPtr payloadDescriptor = authConfig.getPropertySelectionValue("PayloadDescriptor");
std::cout << "Payload id: " << payloadDescriptor.get("Id") << std::endl; // "Pin" - no ICredentialPayloadDescriptor cast needed

authConfig.setPropertyValue("CredentialProviderId", credentialProvider.getId());
std::cout << "Credential provider id: " << authConfig.getPropertyValue("CredentialProviderId") << std::endl;

auto device = instance.addAuthenticatedDevice("daq://openDAQ_1234", nullptr, authConfig);
```

See `demoAuthenticationConfigAsPropertyObject` in the `credential_providers` example app for the full, runnable version of this.

### Example: private-key challenge authentication

```cpp
auto fileCredentialProvider = FileCredentialProvider();
auto cmdLineCredentialProvider = CmdLineCredentialProvider();

auto instanceBuilder = InstanceBuilder();
// Registered first, so it — not CmdLineCredentialProvider — is picked for FilePath requests.
instanceBuilder.addCredentialProvider(fileCredentialProvider.getId(), fileCredentialProvider);
instanceBuilder.addCredentialProvider(cmdLineCredentialProvider.getId(), cmdLineCredentialProvider);
auto instance = instanceBuilder.build();

auto deviceType = instance.getAvailableDeviceTypes().get("CredentialDemoDevice");

// FilePath variant — module reads and parses the PEM file itself.
auto privateKeyFileConfig = deviceType.createDefaultAuthenticationConfig();
SelectAuthenticationMethod(privateKeyFileConfig, "PrivateKeyFile");
auto device = instance.addAuthenticatedDevice("daq://openDAQ_1234", nullptr, privateKeyFileConfig);
```

See [§4](#4-streaming-authentication) for authenticating a streaming connection and [§5](#5-credential-provider-selection--supplied-secrets) for selecting a specific provider or supplying a secret directly.

### Save and reload

Saving an instance persists the connected device's whole `AuthenticationConfig` (payload descriptor, credential provider id, and - if one was set - the supplied secret itself) as part of the tree. Reloading that saved configuration into a new instance re-authenticates the device from scratch, using the reconstructed config exactly as saved: if it carries a supplied secret, that secret is reused directly (no provider is asked); otherwise the new instance must have its own compatible credential provider registered, or the reload fails.

---

## 8. Module-Level Implementation

This section describes what a module does internally when `Module::createAuthenticatedDevice`/`createStreaming` is called — none of this is visible to, or called directly by, application code.

> **Note:** the steps below describe how the credential-demo prototype implements this — provider lookup (currently: first registered provider whose supported formats match, or a direct lookup by id when one was explicitly selected) and retry/fallback behavior on failure may differ in a production implementation, since none of it is prescribed by the core interfaces themselves.

1. **Resolve the payload to provide.** The module reads the payload id and descriptor off the supplied `IAuthenticationConfig` (`getCredentialPayloadId`, `getCredentialPayloadDescriptor`).

2. **Build the credential request.** The module builds a new `ICredentialRequest` via `ICredentialRequestBuilder`, populating connection string, manufacturer/serial number (if resolved), and metadata for the provider to present to the user - the same way whether this is a fresh connection or a reload, since the whole `IAuthenticationConfig` (not just a request formed from it) is what gets persisted and reconstructed on reload (see step 5).

3. **Resolve the credentials.** The module reads `getCredentialProviderId()` off the authentication config, checks whether a `"SuppliedSecret"` property is present (`hasProperty`/`getPropertyValue` - there is no dedicated getter, since it's the one setting exposed only as a plain property), and branches:
   - **A secret is supplied:** no provider obtains anything — the supplied property object (already shaped like the payload descriptor's `createDefaultPayload` template) is used directly as the credential payload. If a provider id was *also* set, that specific provider is first looked up and handed the secret via `provider.cacheCredentials(request, secret)`, so it can remember it the same way it would one obtained interactively — but the payload used for *this* connection is always the one the caller supplied, regardless of whether caching succeeds.
   - **No secret supplied:** the module finds a matching provider — the one named by `getCredentialProviderId()` if set (failing immediately, with a message identifying the problem, if that id names no registered provider or one that doesn't support the required format), otherwise the first registered provider whose `getSupportedPayloadFormats()` includes the required format. Either way, `provider.requestCredentials(request)` is then called, obtaining a credential payload - a property object built from the descriptor's `createDefaultPayload` template - interactively, or served from that provider's own cache if a matching entry exists (e.g. from an earlier `cacheCredentials` call, or an earlier interactive request for the same context).

4. **Construct the device (or streaming) and authenticate.** The component is constructed and its authentication step runs — reading the credential payload's property values and verifying them against whatever the specific method requires (a fixed value, a signed challenge, etc.). A mismatch throws `AuthenticationFailedException`, and construction fails.

5. **Persist the authentication config for later reload (devices only).** On success, the module stores the whole authentication config it was given on the newly created device (`IComponentPrivate::setAuthenticationConfig`) — this is what a future reload reconstructs and passes back in as-is (see [Save and reload](#save-and-reload)), secrets included if any were supplied.

![Application / Module / Credential Provider — sequence view of the same steps](credential_flow_diagram_sequence.png)
*Diagram 3 — the same steps as above, as a sequence diagram across the three parties involved: the Application first obtains the self-contained `authConfig` (`createDefaultAuthenticationConfig`) and, if needed, selects a non-default supported method on it; the Module then either resolves credentials through a Credential Provider (querying `getSupportedPayloadFormats()` to find or validate one, then `requestCredentials`/`cacheCredentials`) or uses a supplied secret directly — matching the branching in Diagram 1. As there, the pink notes are prototype-specific policy, not core-mandated behavior.*
