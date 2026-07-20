# reference/netstack

Recovered network-stack reference material, centralized here from the (now-flushable) legacy ClayEngine
repos. **Reference only — not part of the build/solution.**

## Files

### `JAMServer-Overlapped.cpp` + `Utility.h` — finished IOCP data-pump pattern
A complete, self-contained IOCP echo server (TCP+UDP): completion-port loop, `AcceptEx`/`WSARecv`/
`WSASend`, per-connection buffer/socket objects, heavily commented.

- **Why it's kept:** the engine's IOCP server `ClayEngineLibrary/AsyncNetworkSystem.cpp` has a working
  overlapped-**accept** path but its post-accept **data pump is an explicit stub**
  (`AsyncNetworkSystem.cpp:212-219`). This file is the finished `WSARecv`/`WSASend` completion-loop
  echo/copy pattern to crib when building the data pump for the live-simulation service.
- **Provenance / originality:** **textbook-derived, NOT original IP** — adapted from Jones & Ohlund's
  *Network Programming for Microsoft Windows* `iocpserver` sample. Keep as a learning reference, not as
  authored engine code.
- **Build note:** references a `pch.h` that must supply the Winsock2 includes; `Utility.h` carries the
  `Console`/`Memory`/`Network`/`Random` helpers + `OverlappedSocket`/`OverlappedBuffer`. Not wired into
  `ClayEngineOSS.sln` — read it, don't compile it in place.

### `CATALOG.md` — the full reconciliation record
Inventory of all legacy repos, network-stack lineage (JAMSystem → GlitterGreed → Voxelmancy →
ClayEngineOSS), per-artifact keeper table, duplication map, originality tiers, and the Azure DevOps
flush checklist. This is the "why" behind what was kept and what was deleted.

## Related recovered file (elsewhere in this repo)
- `ClayEngineClient/GlitterGreed.h` — the GlitterGreed game-loop IP (gem-collection sim, standalone
  header). Original EMPS IP, **design credit: Yentlyn**.

## The two go-forward keeper modules (already in this repo)
- IOCP server/client: `ClayEngineLibrary/AsyncNetworkSystem.{cpp,h}`
- NBIO server/client: `ClayEngineLibrary/NetworkSystem.{cpp,h}`

Both were written against the retired thread-affinity `Services` API. Affinity call sites to strip when
they become the two purpose-built services (live-sim IOCP + batch-procedural): `NetworkSystem.cpp:171` &
`:372` (`GetService<…>(std::this_thread::get_id())` → `GetService<…>()`); `AsyncNetworkSystem.h:247` /
`.cpp:430-431` / field `.h:229` (drop the `AffinityData` ctor param + `m_affinity_data`).
