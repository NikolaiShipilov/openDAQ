# Credential Provider Framework — API Reference & Authentication Flow

> **Stale pending a doc pass:** `addAuthenticatedDevice`/`createAuthenticatedDevice` and `addStreaming`/`createStreaming`'s
> `authenticationConfig` parameter, referenced throughout this document, have been removed - only the plain,
> config-only overloads (`addDevice`/`createDevice`, `addStreaming`/`createStreaming` with no such parameter)
> remain. As a temporary, hacky bridge until `IAuthenticationConfig` becomes a real part of the add-device config
> schema, a caller instead stashes the config onto the plain `config` property object under the
> `"__AuthenticationConfig"` property name (see `AuthenticationConfigConfigKey`/`InjectAuthenticationConfig`/
> `ExtractAuthenticationConfig` in `core/opendaq/modulemanager/include/opendaq/module_impl.h`, reused by
> `ModuleManagerImpl`; `GenericDevice`'s reload path and application code each keep their own small local copy of
> the same key, since sharing it across components isn't worth a public header for what's meant to be temporary).
> Every code snippet below using `addAuthenticatedDevice`/`createAuthenticatedDevice` needs updating accordingly -
> see `examples/applications/cpp/credential_providers/credential_providers.cpp`'s `WithAuthenticationConfig` helper
> for the current, correct pattern in the meantime.

Credentials are modelled by their **format** (`CredentialFormat`: `None`, `KeyValuePairs`, `String`, or `FilePath`) and a **authentication method** (`IAuthenticationMethod`) carrying format-specific parameters and a human description. Authentication method selection happens through an `IAuthenticationConfig` object - a single, self-contained property object built with `AuthenticationConfig(componentType)`, listing every method the named component type itself declares supporting (via `IComponentType::getSupportedAuthenticationMethods()`, set once when the module built the type) as a candidate the caller selects among directly, tunes, and hands to `addAuthenticatedDevice`/`addStreaming`.

At most one credential provider can ever be registered for an entire `Instance` - on `IInstanceBuilder`, at build time, before the instance's `Context` even exists (see [§2](#2-extensions-to-existing-interfaces)) - so there is no per-config provider selection at all: `IAuthenticationConfig` doesn't carry a provider id, and a module always uses whichever single provider (if any) `IContext::getCredentialProvider()` returns.

---

## 1. Core Interfaces

### `IAuthenticationMethod`

Describes the shape and presentation of the secret(s) an authentication method expects.

| Member | Description |
|---|---|
| `getId(IString**)` | The id that uniquely identifies this authentication method at least within the module that offers it - the same id the module declares as its default, and that a caller matches against when selecting this method out of an `IAuthenticationConfig`'s `"AuthenticationMethod"` candidates (see below). In practice often unique system-wide instead: the `Standard*AuthenticationMethod` factories key off shared, well-known ids and (but for the anonymous one, which needs no type manager at all) resolve their Struct/secret class from the one `ITypeManager` shared by the whole `Context`, so different modules using the same standard id (and the same `Context`) produce identically-shaped authentication methods, reusing the id deliberately rather than colliding by accident. |
| `getFormat(CredentialFormat*)` | The described secret(s)' format — `None`, `KeyValuePairs`, `String`, or `FilePath`. |
| `getParameters(IStruct**)` | The format's standard parameter set, as a Struct whose own Struct type is pinned to the format - for `KeyValuePairs`, a `"Keys"` dict field mapping each expected key to a hidden flag (e.g. `{"UserName": False, "Password": True}`); for `String`, a single `"Hidden"` bool field. A `FilePath`-format authentication method has no format-specific parameters - `getParameters()` returns an unassigned `IStruct`. A `None`-format authentication method likewise has no parameters, there being nothing to describe. |
| `getDescription(IString**)` | Human-readable description of the authentication method, e.g. *"PIN-code"*, *"username and password"*, *"Path to the SSH private key file"*. |
| `createEmptySecret(IPropertyObject**)` | Builds an empty secret matching this format: a property object with one empty (default `""`) String property per secret value expected - one per key named in `getParameters()`'s `"Keys"` dict for `KeyValuePairs` (e.g. `"UserName"`, `"Password"`), or a single property for `String`/`FilePath`, named and described by the authentication method's own registered secret class (e.g. `"Pin"`, `"PrivateKeyFilePath"`). Meant to be filled in with the actual secret value(s) and used as the credential itself - see [§5](#5-credential-provider-selection--supplied-secrets). Not supported for `None` - a `None`-format authentication method requires no credentials at all, so no secret is ever needed for it in the first place; therefore returns `OPENDAQ_ERR_NOT_SUPPORTED`. |

**Factories:** `KeyValueAuthenticationMethod(id, keys, description, typeManager, secretClassName)`, `StringAuthenticationMethod(id, description, hidden, typeManager, secretClassName)`, `FilePathAuthenticationMethod(id, description, typeManager, secretClassName)`, `NoneAuthenticationMethod(id, description)` - `typeManager` must already have the format's struct type registered, and (for every format but `None`, which has no secret class at all) `secretClassName` must already be a registered `IPropertyObjectClass` that `createEmptySecret()` builds the returned secret from; these throw otherwise. `Context` registers the three formats' struct types up front (`RegisterAuthenticationMethodTypes`, called from `ContextImpl::registerOpenDaqTypes`), so any authentication method built with a real `Context`'s type manager already satisfies the struct-type half of this. `NoneAuthenticationMethod` is the exception - its struct type is a fixed, well-known shape never registered with any type manager, so it takes no `typeManager` parameter at all.

**Standard authentication methods:** `StandardUserNamePasswordAuthenticationMethod(typeManager)`, `StandardPinAuthenticationMethod(typeManager)`, `StandardPrivateKeyFileAuthenticationMethod(typeManager)`, and `StandardAnonymousAuthenticationMethod()` build ready-made authentication methods for four well-known methods, each keyed by its own well-known id (`StandardUserNamePasswordId` = `"UserNamePassword"`, `StandardPinId` = `"Pin"`, `StandardPrivateKeyFileId` = `"PrivateKeyFile"`, `StandardAnonymousId` = `"Anonymous"`) and, for the three that need one, its own registered `IPropertyObjectClass` (`UserNamePasswordCredentialSecretClass`, `PinCredentialSecretClass`, `PrivateKeyFileCredentialSecretClass` - all registered by `RegisterAuthenticationMethodTypes` alongside the struct types above) - so every module offering one of these methods produces the exact same authentication method and secret shape, rather than each inventing its own. `StandardAnonymousAuthenticationMethod` needs no `typeManager` argument, matching `NoneAuthenticationMethod` above.

```cpp
enum class CredentialFormat : EnumType
{
    None = 0,       // no secret(s) at all — typically anonymous access, nothing to supply or verify
    KeyValuePairs,  // N string pairs — e.g. UserName / Password
    String,         // one string — token, API key, PIN
    FilePath        // one string — path to a file containing the secret, e.g. a private key
};
```

---

### `IAuthenticationConfig`

Carries the authentication settings for a single connection attempt. Lives alongside the base add-component config, never serialized as part of it - though a component created with authentication may persist a reduced form of the config it was authenticated with alongside itself instead (see [Serialization](#serialization) below and [§7](#7-application-level-usage)).

`IAuthenticationConfig` extends `IPropertyObject` - the settings below are backed by ordinary properties, so besides the typed getters, they can equally be read and set through the generic `IPropertyObject` interface:

| Property | Description |
|---|---|
| `"AuthenticationMethod"` (Selection) | The authentication method id and its corresponding authentication method, bound together as one property so they can never be set out of sync: its selection value is the `IAuthenticationMethod` Struct itself, and the authentication method id is simply that Struct's own `getId()`. A config returned by `AuthenticationConfig(componentType)` (see [§2](#2-extensions-to-existing-interfaces)) is self-contained - every authentication method the component type supports is a selection candidate here, not just one - so the caller switches methods by changing this property's selection, without fetching a different config object. Changing this property may clear an incompatible `"SuppliedSecret"`. |
| `"SuppliedSecret"` (Object, optional) | A secret supplied directly by the caller, to be used instead of the registered credential provider obtaining it. Present only when one was actually supplied — check with `hasProperty("SuppliedSecret")`, or via `getSuppliedSecret()` below — since an Object-type property cannot itself hold `nullptr`. Every write is validated against the *currently selected* `"AuthenticationMethod"`: the object's property names must match `authentication method.createEmptySecret()`'s exactly (build from that template, fill it in, submit it - the blessed workflow), or the write is rejected. An `"AuthenticationMethod"` change that leaves an already-set `"SuppliedSecret"` incompatible with the new selection silently clears it. See [§5](#5-credential-provider-selection--supplied-secrets). |

| Member | Description |
|---|---|
| `getSelectedAuthenticationMethodId(IString**)` | Convenience getter for the selected `"AuthenticationMethod"` value's own id. The corresponding authentication method itself is obtained by looking this id up in `getSupportedAuthenticationMethods()` - there is no separate getter for it. |
| `getSuppliedSecret(IPropertyObject**)` | Convenience getter for the `"SuppliedSecret"` property's value, or `nullptr` if the property is absent entirely. |

There is no "additional config" on `IAuthenticationConfig` itself - settings like whether to hide secret input as it's typed travel via the component's own, generic config object instead (the same one `addDevice`/`addStreaming` always take), read by the module from its own `config` parameter alongside the authentication config. Nor is there any credential-provider selection on it at all - see [§5](#5-credential-provider-selection--supplied-secrets).

**Factory:** two overloads.
- `AuthenticationConfig(componentType)` — the recommended way to build a config for a live device/streaming type. Reads `authenticationMethods` straight off `componentType`'s own `getSupportedAuthenticationMethods()` (see [§2](#2-extensions-to-existing-interfaces)) - the type itself is the single source of truth for what it supports, set once via `IComponentTypeBuilder` when the module built it.
- `AuthenticationConfig(authenticationMethods)` — the lower-level overload, for building a config with no live `IComponentType` object at hand (e.g. deserialization). Each entry of `authenticationMethods` (a dict keyed by each authentication method's own id) becomes one candidate value of the resulting config's `"AuthenticationMethod"` Selection property, so a caller can later switch between methods just by changing that property's selection instead of needing a different config object per method; the dict's first entry, in iteration order, starts out selected.

`"SuppliedSecret"` is never set by either factory - set it afterward via that property directly, the same way regardless of which one built the config.

**Serialization:** relies on the generic `IPropertyObject` mechanism for `"AuthenticationMethod"` only - every candidate authentication method and the selected one round-trip through it like any other property. `"SuppliedSecret"` is excluded from it, never persisted at all. Deserializing can't use the generic property-object reconstruction pipeline as a black box (this class has no default constructor that adds `"AuthenticationMethod"` up front the way its real constructor does), so it builds a bare stub first, then runs that same generic pipeline on it to add `"AuthenticationMethod"` back from its own serialized definition and restore its saved selection, if one was saved.

---

### `ICredentialRequest`

Carries the non-secret details of a credential request, handed to `ICredentialProvider::requestCredentials`/`cacheCredentials`. Built via `ICredentialRequestBuilder`. Never carries actual secrets.

| Member | Description |
|---|---|
| `getComponentType(IComponentType**)` | The type of component the request is for. |
| `getConnectionString(IString**)` | The *canonical* connection string of this connection attempt - already resolved via the owning module's `onGetCanonicalConnectionString` (routing prefix trimmed, every parameter made explicit), not necessarily the raw string the caller originally supplied. |
| `getMetaData(IPropertyObject**)` | Additional metadata for the provider to present to the user. Optional - empty (no properties) if the caller added none. |
| `getManufacturer(IString**)` | The manufacturer of the device the connection is being established to or for - a request can be for a direct connection to that device, or for a streaming connection attached to it. Optional - unassigned if not known for this connection. |
| `getSerialNumber(IString**)` | The serial number of the device the connection is being established to or for - a request can be for a direct connection to that device, or for a streaming connection attached to it. Optional - unassigned if not known for this connection. |
| `getAuthenticationMethod(IAuthenticationMethod**)` | The authentication method the provider must provide a secret for, read from `IAuthenticationConfig` when the request was built. Its own id names the negotiated authentication method. |

**Why canonical?** A provider identifies which connection a request belongs to primarily by `(manufacturer, serialNumber)` - e.g. `CmdLineCredentialProvider`'s in-session `FilePath` caching (see [§5](#5-credential-provider-selection--supplied-secrets)). When neither is available, a provider falls back to the connection string itself as the identifying key instead - which only works reliably if it's canonical, so the same connection always identifies itself the same way no matter how the caller originally wrote it.

**Factory:** `CredentialRequestFromBuilder(builder)` — hidden factory, built from a `ICredentialRequestBuilder`.

---

### `ICredentialRequestBuilder`

Builds `ICredentialRequest` objects.

| Member | Description |
|---|---|
| `build(ICredentialRequest**)` | Builds and returns a `CredentialRequest` from the currently configured values. Fails if `componentType`, `connectionString`, or `authentication method` was never set. |
| `setComponentType` / `getComponentType` | The component type the request is being built for. Required. |
| `setConnectionString` / `getConnectionString` | The *canonical* connection string for this attempt - expected to already be resolved via `onGetCanonicalConnectionString` before being set here. Required. |
| `setManufacturer` / `getManufacturer` | The manufacturer of the device the connection is being established to or for - a request can be for a direct connection to that device, or for a streaming connection attached to it. Optional - leave unset if not known. |
| `setSerialNumber` / `getSerialNumber` | The serial number of the device the connection is being established to or for - a request can be for a direct connection to that device, or for a streaming connection attached to it. Optional - leave unset if not known. |
| `addMetaDataProperty(IProperty*)` | Adds a metadata property, for the provider to present to the user. Optional - the built request's metadata is simply empty if never called. |
| `getMetaData(IPropertyObject**)` | The accumulated metadata property object. |
| `setAuthenticationMethod` / `getAuthenticationMethod` | The authentication method the provider must supply a secret for - typically read from `IAuthenticationConfig` when the request is built. Its own id names the negotiated authentication method. Required. |

**Factory:** `CredentialRequestBuilder()`

---

### Credentials

There is no dedicated credential interface - a credential is simply an `IPropertyObject`, built from the authentication method's `createEmptySecret()` template and filled in with the actual secret value(s). For a `KeyValuePairs`-format method it has one String property per key (e.g. `"UserName"`, `"Password"`); for `String`/`FilePath` it has a single String property, named and described by the authentication method's own registered secret class (e.g. `"Pin"`, `"PrivateKeyFilePath"`). Both a credential provider's `requestCredentials` and a caller directly supplying a secret (`IAuthenticationConfig`'s `"SuppliedSecret"` property) produce/consume this exact same shape - see [§5](#5-credential-provider-selection--supplied-secrets).

A `None`-format method needs no credentials at all - no `createEmptySecret()` template, no `"SuppliedSecret"`, and no credential provider ever consulted for it (see [§8](#8-module-level-implementation)); selecting `"AuthenticationMethod"` to a `None`-format authentication method is the entire authentication step.

---

### `ICredentialProvider`

Supplies the secrets requested via an `ICredentialRequest` — by prompting the user, reading a file, or fetching from a secret store.

| Member | Description |
|---|---|
| `getDescription(IString**)` | A human-readable description of the provider - e.g. for identifying it in a log message or an error. Not an id - not expected to be unique, or stable across implementations. |
| `requestCredentials(ICredentialRequest*, IPropertyObject**)` | Requests credentials for the given request, in the format described by its authentication method — obtaining them interactively (prompting, reading a file, etc.) unless a cached value from an earlier `cacheCredentials`/`requestCredentials` call for the same context already covers it. Returns a property object built from the authentication method's `createEmptySecret` template, filled in with the obtained secret(s). |
| `cacheCredentials(ICredentialRequest*, IPropertyObject* secret)` | Accepts a secret already known in advance (see `IAuthenticationConfig`'s `"SuppliedSecret"` property), shaped like the request's authentication method's `createEmptySecret` template, so an implementation that would otherwise cache a value obtained interactively caches this one the same way — a later `requestCredentials` call for the same context then reuses it instead of prompting. Produces nothing itself; the caller already has the secret and uses it directly. Implementations for which caching doesn't apply (or doesn't apply to the request's format) may treat this as a no-op. |
| `getSupportedFormats(IList**)` | The list of `CredentialFormat` values this provider can supply — used for format-matching against a device type's supported formats. Should never include `None` - there is nothing for a provider to supply for it; `Module` resolves a `None`-format request entirely on its own, without ever consulting a provider (see [§8](#8-module-level-implementation)). |

**Factory:**
- `CmdLineCredentialProvider()` — the only credential provider implementation the SDK ships. Prompts the user for secrets via the command line, supporting every `CredentialFormat`: `KeyValuePairs`/`String` are read as plain (or hidden, per the authentication method's own `"Hidden"` parameter) input; `FilePath` is validated locally - retried up to 3 times if the entered path isn't accessible, then fails authentication - and cached in-memory for its own lifetime (i.e. for the active session), keyed by `(manufacturer, serialNumber)`, so a second interactive request for the same device reuses the path already entered (or supplied via `cacheCredentials`) instead of prompting again. `String`/`KeyValuePairs` secrets are never cached.

---

## 2. Extensions to Existing Interfaces

Component types (`IComponentType` and everything derived from it, including `IDeviceType`/`IStreamingType`) carry their supported authentication data as genuine Struct fields: `getSupportedAuthenticationMethods()` (the authentication methods it supports, keyed by their own id) and `getDefaultAuthenticationMethodId()`. A module declares these once, via `IComponentTypeBuilder::setSupportedAuthenticationMethods`/`setDefaultAuthenticationMethodId`, when it builds the device or streaming type - left unset, a type defaults to supporting only the standard `"Anonymous"` method. (Server and Function block types do not carry this data - only Device and Streaming types do.) To build the self-contained `IAuthenticationConfig` for a type, call `AuthenticationConfig(componentType)` directly (see [§1](#1-core-interfaces)) - there's no separate method on `IDevice`/`IModule` for this anymore.

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
| `getCredentialProvider(ICredentialProvider**)` | The credential provider set so far, or `nullptr` if none. |
| `setCredentialProvider(ICredentialProvider*)` | Sets the credential provider to register on the built instance's `Context`, replacing whatever was set before. At most one can ever be registered on an instance - there is no way to add more than one, and no way to register one on an already-built `Context` either. |

### `IContext`

Extended with an additional `credentialProvider` parameter (`ICredentialProvider*`) on the `Context` factory - whatever was set via `IInstanceBuilder::setCredentialProvider` flows in through it - and `getCredentialProvider(ICredentialProvider**)` to retrieve it, or `nullptr` if nothing was set. Construction is the only way in - a `Context`, once built, offers no way to change it.

---

## 3. Two Ways to Add a Device

The device can be added either **without authentication** (the plain, anonymous path) or **with authentication**. Both paths exist side by side — a device type that supports authentication doesn't lose its plain connection option, and the two are chosen simply by which method is called.

### The plain path (no authentication)

```cpp
auto device = instance.addDevice("daq://openDAQ_1234");
```

At the application level, this is a single call — no credential provider needs to be registered, and no authentication config is involved at all.

At the module level, this resolves to `Module::createDevice`. The device is constructed with `authenticated = false`, and the module's own secret-verification step is skipped entirely — no credential, no provider lookup, no challenge or comparison of any kind.

### The authenticated path

```cpp
auto device = instance.addAuthenticatedDevice("daq://openDAQ_1234", nullptr, authenticationConfig);
```

The same connection string is used, but an `IAuthenticationConfig` is supplied, and everything described in the rest of this document — credential negotiation, provider lookup, credential retrieval, verification — is triggered as a result.

A device type only exposes this path meaningfully when it declares at least one supported authentication method (see [§2](#2-extensions-to-existing-interfaces)) - `AuthenticationConfig(componentType)` still succeeds otherwise (every type defaults to `"Anonymous"`), but `addAuthenticatedDevice` against a type that only supports `"Anonymous"` gains nothing over the plain path.

---

## 4. Streaming Authentication

Attaching a streaming connection is authenticated **independently** of however the device itself got connected — a device authenticated via PIN can have a streaming source attached to it that goes through its own, entirely separate credential request. `IDevice::addStreaming`'s last parameter is the authentication config, exactly mirroring `addAuthenticatedDevice`: a `nullptr` config uses the plain, unauthenticated path; a supplied one triggers the same provider-lookup/credential-request machinery described for devices.

```cpp
// Plain, unauthenticated streaming attach:
device.addStreaming("daq.credential_demo_streaming://credential_demo_device");

// Authenticated streaming attach, with its own independent authentication config:
auto streamingAuthConfig = AuthenticationConfig(streamingType);
SelectAuthenticationMethod(streamingAuthConfig, "PrivateKeyFile"); // see §7 - switches the selection generically
device.addStreaming("daq.credential_demo_streaming://credential_demo_device", nullptr, streamingAuthConfig);
```

### Auto-attach is always unauthenticated

`StreamingSourceManager` (the implementation behind the `PrioritizedStreamingProtocols`/`AutomaticallyConnectStreaming` add-device config) auto-attaches streaming sources for a device once it's added, always via the plain, unauthenticated path — regardless of whether the device itself was authenticated (`addAuthenticatedDevice`) or not. A streaming source that needs authentication must be attached manually, via `addStreaming`'s own authentication config parameter, as shown above.

![Streaming authentication — manual attach (unauthenticated or independently authenticated), and unauthenticated auto-attach](credential_flow_diagram_streaming.png)
*Diagram 2 — manual `addStreaming` (unauthenticated or independently authenticated), and `StreamingSourceManager`'s always-unauthenticated auto-attach. The authenticated manual path hands off to the same credential resolution shown in Diagram 1 ([§6](#6-authentication-flow)).*

---

## 5. Credential Provider Selection & Supplied Secrets

There is no provider *selection* at all - at most one credential provider can ever be registered for an entire `Instance` (see [§2](#2-extensions-to-existing-interfaces)), and a module always uses that one, if any (see [§8](#8-module-level-implementation)). `IAuthenticationConfig` carries no provider id, and there is nothing to configure on it for this - only `"SuppliedSecret"` lets a caller take over part of the credential machinery that would otherwise happen automatically.

### Supplying the secret directly

Setting `"SuppliedSecret"` hands the module a secret it already has — a property object built from the config's authentication method's `createEmptySecret()` template and filled in with the actual value(s) — instead of having the registered provider obtain one interactively. The write is validated against the *currently selected* `"AuthenticationMethod"` (see [§1](#1-core-interfaces)), so select the method first:

```cpp
auto config = AuthenticationConfig(deviceType); // UserNamePassword is the default method

auto secret = config.getSupportedAuthenticationMethods().get(config.getSelectedAuthenticationMethodId()).createEmptySecret();
secret.setPropertyValue("UserName", "user");
secret.setPropertyValue("Password", "pass");
config.setPropertyValue("SuppliedSecret", secret);

auto device = instance.addAuthenticatedDevice("daq://openDAQ_1234", nullptr, config);
```

- **No credential provider registered:** the module uses the supplied secret directly and authenticates with it, with no prompt of any kind.
- **A provider is registered:** the module still uses the supplied secret directly for this connection, but first hands both it and the credential request to that provider via `ICredentialProvider::cacheCredentials`, so the provider can remember it the same way it would one obtained interactively.

The second case is what makes it possible for a *later*, ordinary `requestCredentials` call — e.g. authenticating a streaming connection attached to the same device — to be served from the provider's cache instead of prompting, provided the provider actually caches for that format (see `CmdLineCredentialProvider`'s `FilePath` caching) and the two requests share the same `(manufacturer, serialNumber)`:

```cpp
auto deviceConfig = AuthenticationConfig(deviceType);
SelectAuthenticationMethod(deviceConfig, "PrivateKeyFile");

auto deviceSecret = deviceConfig.getSupportedAuthenticationMethods().get(deviceConfig.getSelectedAuthenticationMethodId()).createEmptySecret();
deviceSecret.setPropertyValue("PrivateKeyFilePath", "/path/to/private_key.pem");
deviceConfig.setPropertyValue("SuppliedSecret", deviceSecret);

auto device = instance.addAuthenticatedDevice("daq://openDAQ_1234", nullptr, deviceConfig);

// Same device, same FilePath-format method - no path prompt this time; the registered provider serves
// it from the cache `cacheCredentials` populated above.
auto streamingConfig = AuthenticationConfig(streamingType);
SelectAuthenticationMethod(streamingConfig, "PrivateKeyFile");
device.addStreaming("daq.credential_demo_streaming://credential_demo_device", nullptr, streamingConfig);
```

For the credential-demo module specifically, the device's own manufacturer/serial number are what `MirroredDeviceBase::onAddStreaming` resolves (from `IDeviceInfo`) and forwards into the streaming connection attempt, so a manually-attached streaming connection for the same device naturally lands in the same cache bucket.

---

## 6. Authentication Flow

Both ways of adding a device converge on the same entry point at the application level — a connection string and a config object — and diverge based on whether an `IAuthenticationConfig` is supplied.

Without one, the call resolves straight down to the module's plain device-construction path: the module manager locates the appropriate device type from the connection string, and the module builds the device with no further involvement from the credential framework at all.

With one, the module manager first confirms the target device type actually supports authentication, then hands off to the module together with the authentication config. From there, the module works out which authentication method it needs, resolves the credentials for it — the single registered credential provider, if any (see [§5](#5-credential-provider-selection--supplied-secrets)) obtaining them interactively, or a directly-supplied secret used instead — and builds a fresh credential request for it, whether this is a first-time connection or a reload. The module then verifies the obtained credentials and constructs the device. The authentication config itself (not the credential request, which is never saved) is then stored on the device afterward, in reduced form - so a future reload can repeat this same resolution process rather than needing the original secrets to be saved anywhere.

![Adding a device — with vs without authentication, and credential resolution (API-level flow)](credential_flow_diagram_device.png)
*Diagram 1 — the plain vs. authenticated add-device paths, and the credential-resolution branching (the single registered credential provider, interactive `requestCredentials` vs. a directly-supplied secret). The same resolution applies verbatim to authenticating a streaming connection (Diagram 2, [§4](#4-streaming-authentication)) — only the surrounding API call differs. The pink steps (resolving the registered provider, secret-wrapping) are `Module`'s own shared base-class implementation (`requestCredentials`/`obtainCredentials`, see [§8](#8-module-level-implementation)) — not something the core interfaces prescribe, but also not something a module supporting authentication needs to reimplement itself.*

---

## 7. Application-Level Usage

### Registering the credential provider

```cpp
auto credentialProvider = CmdLineCredentialProvider();

auto instanceBuilder = InstanceBuilder();
instanceBuilder.setCredentialProvider(credentialProvider);
auto instance = instanceBuilder.build();
```

Provider registration is **never serialized** — a reloaded instance must register its own provider independently, since provider setup is platform-/host-specific. At most one credential provider can ever be registered, and only at instance-build time (`IInstanceBuilder::setCredentialProvider`) - there is no way to register one on an already-built `Context`, and no way to register more than one. `CmdLineCredentialProvider`, used above, supports every format, so a single instance of it is enough regardless of which authentication methods the connected devices use.

### Selecting an authentication method

`AuthenticationConfig(componentType)` returns one **self-contained** config listing every method the named device type supports as a candidate of its `"AuthenticationMethod"` selection property (see [§1](#1-core-interfaces)) - not a separate config per method. The application either accepts the type's default selection, or switches it to a different supported method by matching authentication method ids, entirely through plain property object calls:

```cpp
// Selects one of the config's supported methods by authentication method id - manipulating the "AuthenticationMethod"
// selection property directly. No `IAuthenticationMethod` cast needed for the id comparison itself,
// only to read `getId()` off each candidate.
void SelectAuthenticationMethod(const AuthenticationConfigPtr& authConfig, const StringPtr& authenticationMethodId)
{
    ListPtr<IStruct> candidates = authConfig.getProperty("AuthenticationMethod").getSelectionValues();
    for (const auto& candidate : candidates)
    {
        if (candidate.asPtr<IAuthenticationMethod>().getId() == authenticationMethodId)
        {
            authConfig.setPropertySelectionValue("AuthenticationMethod", candidate);
            return;
        }
    }
    throw std::runtime_error("Unknown authentication method id: " + authenticationMethodId.toStdString());
}

auto deviceType = instance.getAvailableDeviceTypes().get("CredentialDemoDevice");

// Use the type's default authentication method:
auto config = AuthenticationConfig(deviceType);
auto device = instance.addAuthenticatedDevice("daq://openDAQ_1234", nullptr, config);

// Or explicitly select a specific supported method instead (e.g. "Pin"), on a config of its own:
auto pinConfig = AuthenticationConfig(deviceType);
SelectAuthenticationMethod(pinConfig, "Pin");
auto device2 = instance.addAuthenticatedDevice("daq://openDAQ_1234", nullptr, pinConfig);
```

### Example: manipulating the config as a plain property object

`IAuthenticationConfig` extends `IPropertyObject` (see [§1](#1-core-interfaces)), so the application can equally configure and inspect it with plain property object calls instead of the typed getters used elsewhere in this section - `AuthenticationConfig(componentType)` already returns a fresh, independent, self-contained config, ready to tune directly:

```cpp
auto authConfig = AuthenticationConfig(deviceType);
SelectAuthenticationMethod(authConfig, "Pin");

StructPtr authenticationMethod = authConfig.getPropertySelectionValue("AuthenticationMethod");
std::cout << "Authentication method id: " << authenticationMethod.get("AuthenticationMethodId") << std::endl; // "Pin" - no IAuthenticationMethod cast needed

auto device = instance.addAuthenticatedDevice("daq://openDAQ_1234", nullptr, authConfig);
```

See `demoAuthenticationConfigAsPropertyObject` in the `credential_providers` example app for the full, runnable version of this.

### Example: private-key challenge authentication

```cpp
auto credentialProvider = CmdLineCredentialProvider();

auto instanceBuilder = InstanceBuilder();
instanceBuilder.setCredentialProvider(credentialProvider);
auto instance = instanceBuilder.build();

auto deviceType = instance.getAvailableDeviceTypes().get("CredentialDemoDevice");

// FilePath variant — module reads and parses the PEM file itself.
auto privateKeyFileConfig = AuthenticationConfig(deviceType);
SelectAuthenticationMethod(privateKeyFileConfig, "PrivateKeyFile");
auto device = instance.addAuthenticatedDevice("daq://openDAQ_1234", nullptr, privateKeyFileConfig);
```

See [§4](#4-streaming-authentication) for authenticating a streaming connection and [§5](#5-credential-provider-selection--supplied-secrets) for supplying a secret directly.

### Save and reload

Saving an instance persists the connected device's `AuthenticationConfig` as part of the tree - but only in its reduced, serialized form (see [Serialization](#serialization) in [§1](#1-core-interfaces)): every candidate authentication method and the selected method's id. No secret (supplied or obtained interactively) is ever saved. Reloading that saved configuration into a new instance rebuilds the config directly from the saved authentication methods and method id - it never re-consults the module/type registry. The device then re-authenticates from scratch through the same provider-resolution path a fresh connection would (see [§8](#8-module-level-implementation)): the new instance must have its own compatible credential provider registered, or the reload fails, and that provider is asked for credentials again exactly like it would for a first-time connection - a prompt-free reload only happens if that provider itself already has something cached for this context (see `cacheCredentials`/`requestCredentials` in [§1](#1-core-interfaces)), never because the saved config carried a secret.

See "Phase 4" of the sequence diagram in [§8](#8-module-level-implementation) (Diagram 3) for this end to end, alongside the rest of the device's lifecycle.

---

## 8. Module-Level Implementation

This section describes what happens internally when `Module::createAuthenticatedDevice`/`createStreaming` is called — none of this is visible to, or called directly by, application code.

Steps 1–3 and 5 below are handled entirely by the base `Module` class itself (`module_impl.h`), *before and after* the module's own overridable hook (`onCreateAuthenticatedDevice`/`onCreateStreaming`) is even invoked - the raw `IAuthenticationConfig` never reaches a module's own implementation at all. The hook receives only the already-resolved authentication method id and credentials (both left unassigned for an unauthenticated streaming connection); its own job is reduced to step 4 - constructing the component and verifying the credentials it was handed.

A `None`-format method skips steps 2–3 entirely: `Module::requestCredentials` resolves it directly, forming no `ICredentialRequest` and consulting no credential provider, since there is nothing to request. Step 4 still runs, with `credentials` left unassigned - the same shape as an unauthenticated streaming connection.

1. **Resolve the authentication method to use.** `Module::createAuthenticatedDevice`/`createStreaming` reads the authentication method id off the supplied `IAuthenticationConfig` (`getSelectedAuthenticationMethodId`) and looks its corresponding authentication method up in `getSupportedAuthenticationMethods()` - for streaming, only after first substituting the resolved streaming type's own default config (`resolveDefaultAuthenticationConfig`) if none was explicitly given and the type supports authentication.

2. **Build the credential request.** `Module::requestCredentials` builds a new `ICredentialRequest` via `ICredentialRequestBuilder`, populating connection string (canonicalized via the module's own `onGetCanonicalConnectionString` override), manufacturer/serial number (if resolved), the authentication method, and component type - the same way whether this is a fresh connection or a reload, since the request is always built fresh from whichever `IAuthenticationConfig` is in hand (a freshly-built one, or one reconstructed from its reduced saved form - see step 5 and [Serialization](#serialization) in [§1](#1-core-interfaces)). `addMetaDataProperty` (see [§1](#1-core-interfaces)) is left untouched here - the request's own `getComponentType()`/`getConnectionString()` already cover what a module building through `Module` would otherwise duplicate into it; a module bypassing `requestCredentials` to build its own request is still free to attach extra metadata.

3. **Resolve the credentials.** `Module::obtainCredentials` reads `context.getCredentialProvider()` and the authentication config's `getSuppliedSecret()`, and branches on whether the latter returned one:
   - **A secret is supplied:** no provider obtains anything — the supplied property object (already shaped like the authentication method's `createEmptySecret` template) is used directly as the credential. If a provider is registered, it is handed the secret via `provider.cacheCredentials(request, secret)`, so it can remember it the same way it would one obtained interactively — but the secret used for *this* connection is always the one the caller supplied, regardless of whether caching succeeds.
   - **No secret supplied:** the registered provider (failing immediately, with a message identifying the problem, if none is registered at all or the registered one doesn't support the required format) is asked via `provider.requestCredentials(request)`, obtaining credentials - a property object built from the authentication method's `createEmptySecret` template - interactively, or served from that provider's own cache if a matching entry exists (e.g. from an earlier `cacheCredentials` call, or an earlier interactive request for the same context).

> **Note:** retry/fallback behavior on failure is `Module`'s own policy, not prescribed by the core interfaces themselves - `requestCredentials`/`obtainCredentials`/`resolveDefaultAuthenticationConfig` are ordinary (non-virtual) `Module` methods a subclass can call directly for finer control, though `onCreateAuthenticatedDevice`/`onCreateStreaming` don't need to. Which provider gets used, however, is never `Module`'s own policy at all - there is only ever the one registered on `Context` (see [§5](#5-credential-provider-selection--supplied-secrets)), if any.

4. **Construct the component and authenticate.** The only step a module's own `onCreateAuthenticatedDevice`/`onCreateStreaming` override implements: construct the device or streaming object and run its authentication step — reading the credentials' property values (already resolved and handed in as the `authenticationMethodId`/`credentials` parameters) and verifying them against whatever the specific method requires (a fixed value, a signed challenge, etc.). A mismatch throws `AuthenticationFailedException`, and construction fails.

5. **Persist the authentication config for later reload (devices only).** Also handled by `Module::createAuthenticatedDevice` itself, after the hook returns a device: it stores the *original* authentication config it was given on the newly created device (`IComponentPrivate::setAuthenticationConfig`) — its custom serialization then reduces this, on save, to every candidate authentication method and the selected method id (see [Serialization](#serialization) in [§1](#1-core-interfaces)) — so a future reload can repeat this same resolution process (see [Save and reload](#save-and-reload)) rather than needing the credential itself, or how it was obtained, to be saved anywhere. The module implementation never sees this happen.

![Application / Module / Credential Provider — sequence view of the same steps](credential_flow_diagram_sequence.png)
*Diagram 3 — the same steps as above, as a sequence diagram across the three parties involved, now covering the full device lifecycle rather than just the module-level resolution step: the Application creates the self-contained default config (`AuthenticationConfig(componentType)`) and, if needed, tunes it - a different supported method, or a supplied secret; the Module then either resolves credentials through the registered Credential Provider (`requestCredentials`/`cacheCredentials`) or uses a supplied secret directly, matching the branching in Diagram 1; finally the config is saved and reloaded, re-authenticating the device from scratch through the same provider-resolution path - a prompt-free reload only happens if the provider itself already has something cached for this context, never because a secret was saved. Each phase's note names exactly which actors participate in it. Building the config is a single, direct call - `componentType` is already in the Application's hands (see [§2](#2-extensions-to-existing-interfaces)), no `ParentDevice`/`Instance` hop needed. As in Diagram 1, the pink/orange notes are `Module`'s own shared base-class code for consuming the single registered provider (steps 1-3 and 5, running around a module's own hook rather than inside it) - not core-mandated behavior, and not something a module implementation performs or even observes directly. Only the per-method verification (step 4) is truly specific to the credential-demo module.*
