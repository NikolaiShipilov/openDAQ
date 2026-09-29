# Credential Provider Framework — Usage Scenarios

> **Stale pending a doc pass:** `addAuthenticatedDevice`, referenced throughout the scenarios below, has been
> removed - only the plain, config-only `addDevice`/`addStreaming` remain. A caller instead stashes the
> `IAuthenticationConfig` onto the plain `config` property object under the `"__AuthenticationConfig"` property
> name, temporarily, until `IAuthenticationConfig` becomes a real part of the add-device config schema. See
> `examples/applications/cpp/credential_providers/credential_providers.cpp`'s `WithAuthenticationConfig` helper
> for the current, correct pattern; every `addAuthenticatedDevice(...)` call below should be read as
> `addDevice(connectionString, WithAuthenticationConfig(authConfig))` instead.

A backlog of user stories and use-case scenarios for exercising `AuthenticationConfig(componentType)`, `addAuthenticatedDevice`/`addStreaming`, and the credential provider framework end to end - meant to drive example/test implementations that verify the API is actually usable, not just that individual methods return the right type. Each scenario names the concrete API calls and expected outcome (return value, exception, or observable side effect) so it can be implemented directly, ideally against `credential_demo_module` the way `credential_providers.cpp` already does.

Persona used throughout: **an application developer** integrating openDAQ's credential framework into a client app or tool - deciding whether/how to offer authenticated connections to their users.

At most one credential provider can ever be registered for an entire `Instance` (`IInstanceBuilder::setCredentialProvider`, before build) - there is no per-config provider selection anywhere in this API, so none of the scenarios below involve choosing among multiple registered providers.

---

## 1. Discovery & feasibility

Before ever prompting a user or attempting a connection, an application needs to know what's even possible.

### 1.1 — Does this type support authentication at all?

As an application developer, I want to check whether a given device/streaming type supports authentication, so I can decide whether to offer an "authenticated connect" option in my UI at all.

- **Given** a type id whose module declares supported authentication authentication methods and a default one for it (e.g. `"CredentialDemoDevice"`)
  **When** I call `AuthenticationConfig(componentType)`
  **Then** it succeeds and returns a config whose `"AuthenticationMethod"` selection has at least one candidate.
- **Given** a type whose module never called `setSupportedAuthenticationMethods`/`setDefaultAuthenticationMethodId`
  on its `IComponentTypeBuilder` (so it carries only the standard `"Anonymous"` method - no credentials required)
  **When** I call `AuthenticationConfig(componentType)`
  **Then** it still succeeds, returning a config whose only `"AuthenticationMethod"` candidate is `"Anonymous"` -
  there is no longer a way for a type to declare "no authentication support at all"; every type supports at
  least `"Anonymous"`. Distinguishing "genuinely richer auth support" from "Anonymous-only" is a matter of
  inspecting the candidates/format, not of the call failing (see `ModuleManagerImpl::createDeviceInternal`'s
  own such check for a worked example).
- **Given** a type id that names neither an available device type nor an available streaming type
  **When** I call `AuthenticationConfig(componentType)`
  **Then** it fails with `OPENDAQ_ERR_NOTFOUND`, distinguishable from the "not supported" case above (a developer needs to tell "unknown type" from "known type, no auth" apart - e.g. to detect a typo in `typeId` vs. correctly reporting "no auth available" to a user).
- **Given** the plain, anonymous path is what's wanted instead
  **When** I call `instance.addDevice(connectionString)` on a type that doesn't support authentication (or one that does, but I chose not to authenticate)
  **Then** it succeeds regardless - the two paths are fully independent (see the API reference, §3).

### 1.2 — Is authentication actually *usable* right now, given the registered provider?

As an application developer, I want to know not just whether a type *supports* authentication, but whether I can actually *complete* it with whatever's registered on my instance - these are different questions, and the API doesn't collapse them into one boolean. Unlike an earlier design, there is no per-config property to inspect for this - the only source of truth is the registered provider itself.

- **Given** a type supporting only a `String`-format method (e.g. `"Pin"`), and either no credential provider registered, or one registered that doesn't declare `String` among `getSupportedFormats()`
  **When** I call `instance.getContext().getCredentialProvider()` and inspect its `getSupportedFormats()` (or its absence) against the method's format, entirely client-side
  **Then** I can tell, without ever attempting a connection, that authentication would fail before ever calling `addAuthenticatedDevice`.
- **Given** zero credential providers registered on the instance at all
  **When** I build the default config and call `addAuthenticatedDevice`
  **Then** it fails with `AuthenticationFailedException` ("no credential provider is registered") - a good negative-path example distinct from 1.1's "type doesn't support auth" case: here the *type* supports it, the *instance* just can't currently serve it.

---

## 2. Manual authenticated device — default config, unmodified

The user's original bullets, filled in.

### 2.1 — Default config + correct credentials, compatible provider registered

As an application developer, I want the simplest possible path to work: get the default config, hand it straight to `addAuthenticatedDevice` with no changes, and have it succeed when the user enters correct credentials.

- **Given** a `CmdLineCredentialProvider` (or equivalent, supporting the default method's format) registered on the instance
  **And** `authConfig = AuthenticationConfig(componentType)` used as-is (default `"AuthenticationMethod"` selection)
  **When** I call `instance.addAuthenticatedDevice(connectionString, nullptr, authConfig)` and supply correct credentials at the prompt
  **Then** the device is added, `device.getInfo()` is queryable, and `device.getAvailableDeviceTypes()`/other calls on it work normally.

### 2.2 — Default config + incorrect credentials

- **Given** the same setup as 2.1
  **When** incorrect credentials are supplied at the prompt
  **Then** `addAuthenticatedDevice` throws `AuthenticationFailedException`, the device is **not** added (verify `instance.getDevices()` doesn't contain it), and no partially-constructed device is left behind.

### 2.3 — Default config, no provider registered at all

- **Given** an instance built with no `setCredentialProvider` call
  **When** I call `addAuthenticatedDevice` with a default config
  **Then** it throws `AuthenticationFailedException` before ever attempting to prompt for anything (no hang waiting on stdin) - the "no credential provider is registered" message, not a null-pointer/crash.

---

## 3. Manual authenticated device — supplied credential

### 3.1 — Correct supplied credential, provider registered

- **Given** `authConfig = AuthenticationConfig(componentType)` with the desired `"AuthenticationMethod"` selected (`SelectAuthenticationMethod`) and `"SuppliedCredential"` set to a correctly-shaped, correct `credential`, with a format-compatible credential provider registered on the instance
  **When** I call `addAuthenticatedDevice`
  **Then** authentication succeeds with **no interactive prompt at all** (verify via a non-interactive/scripted run - if the harness would hang on stdin, that's a bug or a wrong assumption about this path), and the provider's `cacheCredentials(request, credential)` is invoked - confirmed indirectly by then making a *second*, separate connection attempt (same manufacturer/serial or canonical connection string) using only `requestCredentials` (no supplied credential) and observing it also completes without a prompt.

### 3.2 — Correct supplied credential, no provider registered

- **Given** the same as 3.1 but with no credential provider registered on the instance at all
  **When** I call `addAuthenticatedDevice`
  **Then** authentication still succeeds with no prompt (the supplied credential is used directly, no provider ever consulted per §8 step 3), **and** nothing is cached anywhere, since there is no provider to cache into.

### 3.3 — Incorrect supplied credential

- **Given** a supplied credential shaped correctly but with a wrong value (e.g. wrong password, wrong PIN (credential))
  **When** I call `addAuthenticatedDevice`
  **Then** it throws `AuthenticationFailedException` - verifying that a caller can't bypass the module's own verification step just by constructing the credential shape correctly; supplying a credential skips the *provider*, not the *authenticator*.

### 3.4 — Malformed supplied credential (robustness)

As an application developer, I want a clear failure, not a confusing one, if I get the supplied-credential shape wrong.

- **Given** a supplied credential property object missing an expected property (e.g. a `KeyValuePairs` credential missing `"Password"`), or with an extra/misnamed one
  **When** I call `addAuthenticatedDevice`
  **Then** document (via the example) exactly what happens today - does it throw a clear, actionable error, or fail cryptically deep inside verification? This is worth an explicit example even if the answer turns out to be "not very friendly today," since it tells us whether `createEmptyCredential()` really needs to be used as the template (as documented) rather than treated as optional guidance.

---

## 4. Tuning the default config before use

Between "use the unmodified default" (§2) and "supply a credential directly" (§3), there's the actual expected common path: get the default, change one or two things, use it. With no per-config provider selection, the only thing left to tune here is `"SuppliedCredential"`.

### 4.1 — An incompatible `"SuppliedCredential"` write is rejected

- **Given** a config with `"AuthenticationMethod"` selected to a `KeyValuePairs`-format method (e.g. `UserNamePassword`)
  **When** I call `setPropertyValue("SuppliedCredential", ...)` with an object shaped for a *different* format (e.g. a single `"Pin"` property, matching `String`/`FilePath` instead)
  **Then** the write throws and `"SuppliedCredential"` is not set - confirming validation happens against the object's actual property names/count, not just "any object goes."

### 4.2 — Changing the selected authentication method silently clears an incompatible `"SuppliedCredential"`

- **Given** a config with a valid `"SuppliedCredential"` set for the currently-selected `"AuthenticationMethod"`
  **When** I switch `"AuthenticationMethod"` to a different method whose `createEmptyCredential()` shape doesn't match the existing credential
  **Then** the selection change succeeds (no exception) and `hasProperty("SuppliedCredential")` becomes `false` afterward - the stale credential is silently cleared, not left in place mismatched with the new selection, and not blocking the method switch. Calling `addAuthenticatedDevice` afterward with no credential re-supplied falls through to the normal provider-based path (§2).

---

## 5. Credential provider caching

### 5.1 — Same context, same format → cache hit

- **Given** a device authenticated via a caching-capable provider and format (e.g. `CmdLineCredentialProvider` + `FilePath`)
  **When** a *second* connection sharing the same manufacturer/serial (or, absent those, the same canonical connection string - e.g. a streaming attach to the same device) is authenticated via the same format
  **Then** no prompt occurs the second time - this is the scenario `demoCachedFilePathCredentialAcrossDeviceAndStreaming` already demonstrates; worth keeping as a named, minimal example independent of the bigger demo.

### 5.2 — Different context → cache miss (isolation)

The inverse of 5.1, and just as important to verify explicitly.

- **Given** the same setup as 5.1
  **When** a connection with a *different* manufacturer/serial pair (or a different canonical connection string) is authenticated via the same format
  **Then** it prompts independently - confirming the cache key genuinely discriminates by connection identity and doesn't leak credentials across unrelated devices.

### 5.3 — Non-caching format

- **Given** a `KeyValuePairs`/`String` request against `CmdLineCredentialProvider` (only `FilePath` is cached)
  **When** the same context is authenticated twice
  **Then** it prompts both times - confirming caching is opt-in per format, not a blanket assumption an application can rely on for every combination.

---

## 6. Streaming authentication

### 6.1 — Streaming authenticated independently of its device

- **Given** a device connected via one method (or not authenticated at all)
  **When** a streaming connection to it is added via `device.addStreaming(connectionString, nullptr, streamingAuthConfig)` using a *different* method
  **Then** both succeed independently - the streaming leg's authentication outcome (success or failure) is unaffected by how the device itself was connected, and vice versa.

### 6.2 — Auto-attached streaming is always unauthenticated

As an application developer relying on `PrioritizedStreamingProtocols`/`AutomaticallyConnectStreaming`, I want to know this path never authenticates on my behalf, so I don't assume protection I don't have.

- **Given** a device added with auto-attach streaming config enabled, to a streaming type that *does* support authentication
  **When** the device connects
  **Then** the auto-attached streaming source is present and working, but was never asked for credentials of any kind - verified by registering *no* credential provider at all and confirming auto-attach still succeeds (where a manual `addStreaming` with an auth config would instead fail per §2.3).

---

## 7. Save & load

The custom persistence model: `AuthenticationConfigImpl`'s serialization writes every candidate authentication method and the selected method id, generically, as an ordinary property; `"SuppliedCredential"` is never written. There is no credential-provider information saved at all - a provider is registered once per `Instance`, not per config, so nothing about it needs to survive a save/reload round trip. Reload rebuilds the config directly from the saved authentication methods and method id - it never re-consults the module/type registry.

### 7.1 — Reload with a compatible provider registered succeeds

- **Given** an authenticated device saved via `instance.saveConfiguration()`
  **When** a new instance registers a format-compatible credential provider, then calls `loadConfiguration(savedConfiguration)`
  **Then** the device reconnects, re-authenticating from scratch through the normal `requestCredentials` path - prompting again unless that provider happens to have something cached for this context (§5.1).

### 7.2 — Reload with no compatible provider registered

- **Given** the same saved configuration
  **When** the new instance registers no provider at all, or one that doesn't support the persisted method's format
  **Then** `loadConfiguration` fails to reconnect the device (confirm via `reloadedInstance.getDevices()` being empty, matching `demoPinAuthenticationAndReload`'s own check) - not a silent, disconnected-but-present device.

### 7.3 — Reload with a stale saved method id

- **Given** a saved configuration whose method id is no longer among the type's *currently* supported authentication methods (e.g. the module was updated and dropped or renamed that method)
  **When** the new instance calls `loadConfiguration(savedConfiguration)`
  **Then** the reload fails hard (custom `Deserialize` throws) rather than silently substituting the type's current default method - confirming a reload never silently authenticates via a method the user didn't actually choose.

### 7.4 — A supplied-credential device does not skip the prompt on reload

- **Given** a device originally authenticated via a directly-supplied `"SuppliedCredential"` (§3.1/3.2)
  **When** it's saved and reloaded into a new instance with a compatible provider registered
  **Then** the reload prompts (or fails, if no provider is registered) exactly as a from-scratch connection would - confirming the supplied credential never survives serialization at all, only the candidate authentication methods and method id do.

---

## 8. Boundary & negative checks worth having somewhere, even briefly

- Calling `AuthenticationConfig(componentType)` with a component type of a sort that doesn't carry authentication data at all (Server/FunctionBlock types - only Device/Streaming types do) - confirm the current behavior: `getSupportedAuthenticationMethods()`/`getDefaultAuthenticationMethodId()` still return the internal `"Anonymous"`-only default for these (set once by `GenericComponentTypeImpl`, just never exposed as a Struct field), so the call succeeds with an Anonymous-only config rather than failing - worth confirming this is the intended behavior, not an oversight.
- Calling `addAuthenticatedDevice` with `authenticationConfig = nullptr` against a type that supports authentication - confirm this is rejected clearly (`AuthenticationFailedException`, from `Module::requestCredentials`'s own check, before a module's `onCreateAuthenticatedDevice` is ever invoked), distinct from silently falling back to the anonymous path.
- Two authentication attempts to the *same* connection string in quick succession without an intervening `removeDevice` (e.g. accidental double-click in a UI) - confirm the second attempt's failure/success mode is well-defined rather than racing the first.

---

## 9. Anonymous / `None`-format authentication

A `None`-format method needs no credentials at all - selecting it is the entire authentication step, with no provider and no supplied credential involved anywhere.

### 9.1 — Selecting a `None`-format method needs no registered provider

- **Given** a type whose module offers a `None`-format method (e.g. `"Anonymous"`) among its supported authentication methods, and *no* credential provider registered on the instance at all
  **When** I select that method (`SelectAuthenticationMethod(config, "Anonymous")`) and call `addAuthenticatedDevice`
  **Then** it succeeds - unlike §2.3, the absence of any registered provider doesn't matter here, since none is ever consulted for this method.

### 9.2 — A `"SuppliedCredential"` write is rejected while a `None`-format method is selected

- **Given** a config with a `None`-format method currently selected
  **When** I call `setPropertyValue("SuppliedCredential", credential)` for any property object at all, including an empty one
  **Then** the write throws - `None` has no `createEmptyCredential()` template to match against, so no object is ever a valid shape for it (compare §4.1, the analogous rejection for a real format).

### 9.3 — Switching *to* a `None`-format method clears an existing `"SuppliedCredential"`

- **Given** a config with a valid `"SuppliedCredential"` set for the currently-selected, non-`None` `"AuthenticationMethod"`
  **When** I switch the selection to a `None`-format method
  **Then** the selection change succeeds and `hasProperty("SuppliedCredential")` becomes `false` afterward - the same clearing behavior as §4.2, here covering the case where the new selection accepts no credential at all.

### 9.4 — Connecting via `None` behaves like the plain, unauthenticated path

- **Given** a type offering both a `None`-format method and the plain `addDevice` path
  **When** I connect once via `addDevice(connectionString)` and once via `addAuthenticatedDevice(connectionString, nullptr, anonymousConfig)`
  **Then** both succeed identically from the caller's perspective - no prompt, no provider interaction, the same resulting device. (`credential_demo_module` implements this literally: connecting via its `"Anonymous"` method takes the same construction path as its own unauthenticated `addDevice`.)
