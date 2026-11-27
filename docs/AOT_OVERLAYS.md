# Original-disc AOT overlays

The current Mega Man X5 release keeps original-disc overlays on the guarded
interpreter route because its trusted co-op function and instruction hooks
must observe every entry. It does not bundle an AOT bank or runtime compiler.
The resident executable, audited waits, loading provider and BIOS executive
use their selected native implementations. Unsupported BIOS services fail
explicitly; they never dispatch firmware.

The historical inventory below describes the original-disc extraction profile,
not native coverage shipped by this release.

The declarative [profile](../aot/overlays.json) consumes psxrecomp's reusable
`sector_extent_members` method. Descriptors are little-endian pairs of
archive-relative sector offset and byte size. Payloads are consecutive after
sector alignment. The shared implementation contains no title-ID branches.

## Original loader evidence

The initializer at `0x80015DDC` finds `ROCK_X5.BIN` and reads one header sector.
At `0x80015E78`, its loop adds each descriptor's first word to the file LBA and
copies its byte size; the loop covers 59 entries. Loader `0x80015EB4` selects
an entry by table index and preserves the supplied destination. Sector transfer
`0x80014464` copies payload verbatim, including its leading logical ID, and
rounds the final transfer to four bytes.

| Table indices (decimal) | Destination | Original evidence |
| --- | --- | --- |
| 0-29 | `0x800EE970` | Byte selector table `0x8006FD50`, destination word `0x80010000`, call `0x800136EC` |
| 30-37 | `0x800FA000` | Byte selector table `0x8006FC5C`, call `0x80013568` |
| 40-58 | `0x800FA000` | Byte selector table `0x8006FD80`, call `0x800136B4` |

Four selected members (13, 15, 17, 19) contain resident function-pointer data
and no established callable roots. They are explicitly excluded from code
generation. Members 38 and 39 contain code, but their loader selection and
destination were not established in this bounded pass; they are also explicitly
excluded. The remaining 53 members have static callable roots. A BIOS resident
helper brings the inventory to 54 independently compiled recipes.

The profile pins the original disc SHA-1 and checks the loader and selector
words. The parser validates descriptor count, termination, bounds, and complete
archive coverage after sector alignment. All 59 descriptors remain checked,
including the six exclusions. Original-disc discovery found no additional
supported code producers in `ROCK_X5.DAT`; that is not a proof that all
possible execution sources have been identified.

## Release checks and limits

The release configuration selects `aot.mode = "none"`, disables overlay-cache
execution and omits the overlay toolchain. Native save states remain specific
to their executable and current serializer. In-game memory-card progress is
persistent and separate from those snapshots.

The original image remains read-only. Valid mod plans can change guest RAM and
the loaded in-memory game image under expected-byte, identity, conflict and
transactional rollback guards.
